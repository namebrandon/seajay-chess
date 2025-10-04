#include "transposition_table.h"
#include <cstring>
#include <cstdlib>
#include <algorithm>
#include <bit>
#include <cassert>
#include <climits>

// Platform-specific aligned memory allocation
#ifdef _WIN32
    #include <malloc.h>  // For _aligned_malloc/_aligned_free on Windows
    #define ALIGNED_ALLOC(alignment, size) _aligned_malloc(size, alignment)
    #define ALIGNED_FREE(ptr) _aligned_free(ptr)
#else
    // Unix-like systems (Linux, macOS) use std::aligned_alloc
    #define ALIGNED_ALLOC(alignment, size) std::aligned_alloc(alignment, size)
    #define ALIGNED_FREE(ptr) std::free(ptr)
#endif

namespace seajay {

namespace {
thread_local TranspositionTable::StorePolicy g_storePolicy = TranspositionTable::StorePolicy::Primary;
}

TranspositionTable::StorePolicyGuard::StorePolicyGuard(StorePolicy policy)
    : m_previous(g_storePolicy) {
    g_storePolicy = policy;
}

TranspositionTable::StorePolicyGuard::~StorePolicyGuard() {
    g_storePolicy = m_previous;
}

// AlignedBuffer implementation
AlignedBuffer::AlignedBuffer(size_t size) : m_size(size) {
    if (size > 0) {
        // Allocate 64-byte aligned memory for cache line optimization
        // Note: On Unix, size must be multiple of alignment; Windows doesn't require this
        size_t alignedSize = (size + 63) & ~size_t(63);  // Round up to multiple of 64
        m_data = ALIGNED_ALLOC(64, alignedSize);
        if (!m_data) {
            throw std::bad_alloc();
        }
        // Zero-initialize for consistency (use original size, not aligned size)
        std::memset(m_data, 0, size);
    }
}

AlignedBuffer::~AlignedBuffer() {
    free();
}

AlignedBuffer::AlignedBuffer(AlignedBuffer&& other) noexcept
    : m_data(other.m_data), m_size(other.m_size) {
    other.m_data = nullptr;
    other.m_size = 0;
}

AlignedBuffer& AlignedBuffer::operator=(AlignedBuffer&& other) noexcept {
    if (this != &other) {
        free();
        m_data = other.m_data;
        m_size = other.m_size;
        other.m_data = nullptr;
        other.m_size = 0;
    }
    return *this;
}

void AlignedBuffer::resize(size_t newSize) {
    if (newSize != m_size) {
        free();
        m_size = newSize;
        if (newSize > 0) {
            // Note: On Unix, size must be multiple of alignment; Windows doesn't require this
            size_t alignedSize = (newSize + 63) & ~size_t(63);  // Round up to multiple of 64
            m_data = ALIGNED_ALLOC(64, alignedSize);
            if (!m_data) {
                m_size = 0;
                throw std::bad_alloc();
            }
            std::memset(m_data, 0, newSize);
        }
    }
}

void AlignedBuffer::clear() {
    if (m_data && m_size > 0) {
        std::memset(m_data, 0, m_size);
    }
}

void AlignedBuffer::free() {
    if (m_data) {
        ALIGNED_FREE(m_data);
        m_data = nullptr;
        m_size = 0;
    }
}

// TranspositionTable implementation
TranspositionTable::TranspositionTable() 
    : TranspositionTable(
#ifdef NDEBUG
        DEFAULT_SIZE_MB_RELEASE
#else
        DEFAULT_SIZE_MB_DEBUG
#endif
    ) {}

TranspositionTable::TranspositionTable(size_t sizeInMB)
    : m_generation(0), m_enabled(true), m_clustered(true) {
    resize(sizeInMB);
}

void TranspositionTable::resize(size_t sizeInMB) {
    // Calculate number of entries (must be power of 2)
    m_numEntries = calculateNumEntries(sizeInMB);
    
    // For clustered mode, ensure entries is multiple of CLUSTER_SIZE
    if (m_clustered && (m_numEntries % CLUSTER_SIZE) != 0) {
        // Round down to nearest multiple of CLUSTER_SIZE
        m_numEntries = (m_numEntries / CLUSTER_SIZE) * CLUSTER_SIZE;
        if (m_numEntries == 0) {
            m_numEntries = CLUSTER_SIZE;  // Minimum one cluster
        }
    }
    
    m_mask = m_numEntries - 1;
    
    // Allocate buffer
    size_t bufferSize = m_numEntries * sizeof(TTEntry);
    m_buffer.resize(bufferSize);
    m_entries = static_cast<TTEntry*>(m_buffer.data());
    
    // Reset statistics
    m_stats.reset();
    
    // Validate alignment in debug builds
#ifndef NDEBUG
    assert(reinterpret_cast<uintptr_t>(m_entries) % 64 == 0);
    assert(std::has_single_bit(m_numEntries));  // Power of 2 check
#endif
}

void TranspositionTable::store(Hash key, Move move, int16_t score,
                               int16_t evalScore, uint8_t depth, Bound bound,
                               int ply, CoverageKind coverageKind) {
    if (!m_enabled) return;

    const StorePolicy policy = g_storePolicy;

    auto recordStore = [&]() {
#ifdef TT_STATS_ENABLED
        m_stats.stores++;
        if (policy == StorePolicy::Verification) {
            m_stats.verificationStores++;
        }
#endif
        m_stats.recordStore(ply, coverageKind);
    };

    auto recordVerificationSkip = [&]() {
#ifdef TT_STATS_ENABLED
        if (policy == StorePolicy::Verification) {
            m_stats.verificationSkips++;
        }
#endif
    };

    enum class StoreSkipReason {
        ProtectFreshMove,
        DepthNotImproved,
        CollisionNoMove,
        Other,
    };

    auto recordSkip = [&](StoreSkipReason reason) {
#ifdef TT_STATS_ENABLED
        switch (reason) {
            case StoreSkipReason::ProtectFreshMove:
                m_stats.storeSkipsProtectMove.fetch_add(1, std::memory_order_relaxed);
                break;
            case StoreSkipReason::DepthNotImproved:
                m_stats.storeSkipsDepth.fetch_add(1, std::memory_order_relaxed);
                break;
            case StoreSkipReason::CollisionNoMove:
                m_stats.storeSkipsCollisionNoMove.fetch_add(1, std::memory_order_relaxed);
                break;
            case StoreSkipReason::Other:
            default:
                m_stats.storeSkipsOther.fetch_add(1, std::memory_order_relaxed);
                break;
        }
#endif
    };

#ifdef DEBUG
    auto ensureVerificationSlot = [&](const TTEntry& entry) {
        if (policy == StorePolicy::Verification) {
            assert((entry.isEmpty() || entry.hasFlag(TTEntryFlags::Exclusion)) &&
                   "Verification store attempted to overwrite primary TT entry");
        }
    };
#else
    auto ensureVerificationSlot = [&](const TTEntry&) {};
#endif

    uint32_t key32 = static_cast<uint32_t>(key >> 32);
    
    if (m_clustered) {
        // Clustered mode: single-pass victim selection
        const size_t clusterIdx = clusterStart(key);  // Compute once
        TTEntry* cluster = &m_entries[clusterIdx];

        if (policy == StorePolicy::Verification) {
            for (size_t i = 0; i < CLUSTER_SIZE; ++i) {
                TTEntry& entry = cluster[i];
                if (entry.isEmpty() || entry.hasFlag(TTEntryFlags::Exclusion)) {
                    ensureVerificationSlot(entry);
                    entry.save(key32, move, score, evalScore, depth, bound, m_generation,
                               toMask(TTEntryFlags::Exclusion));
                    recordStore();
                    return;
                }
            }
            recordVerificationSkip();
            return;  // No suitable slot; skip to avoid polluting primary entries
        }

        // Single-pass victim selection with inline scoring
        size_t bestVictim = 0;
        int bestScore = INT32_MAX;
        
        // Unrolled evaluation of all 4 entries
        for (size_t i = 0; i < CLUSTER_SIZE; ++i) {
            const TTEntry& entry = cluster[i];
            
            // Immediate return cases
            if (entry.isEmpty()) {
#ifdef TT_STATS_ENABLED
                m_stats.replacedEmpty++;
#endif
                ensureVerificationSlot(cluster[i]);
                cluster[i].save(key32, move, score, evalScore, depth, bound, m_generation);
                recordStore();
                return;
            }
            
            if (entry.key32 == key32) {
                // Same position - update if appropriate
                const bool entryIsFresh = entry.generation() == m_generation;
                if (move == NO_MOVE && entry.move != NO_MOVE && entryIsFresh && depth <= entry.depth &&
                    !entry.hasFlag(TTEntryFlags::Exclusion)) {
                    return;  // Don't overwrite move with NO_MOVE
                }
                ensureVerificationSlot(cluster[i]);
                cluster[i].save(key32, move, score, evalScore, depth, bound, m_generation);
                recordStore();
                return;
            }
            
            // Score this entry as victim candidate
            // Simple scoring: oldGen(-10000) + depth*100 + nonExact(-50) + noMove(-25)
            int victimScore = entry.depth * 100;
            if (entry.generation() != m_generation) victimScore -= 10000;
            if (entry.bound() != Bound::EXACT) victimScore -= 50;
            if (entry.move == NO_MOVE) victimScore -= 25;
            if (entry.hasFlag(TTEntryFlags::Exclusion)) victimScore -= 20000;
            
            // Penalty for evicting deep entries with moves for shallow NO_MOVE
            if (move == NO_MOVE && entry.move != NO_MOVE) {
                victimScore += 200;
            }
            
            if (victimScore < bestScore) {
                bestScore = victimScore;
                bestVictim = i;
            }
        }
        
        // Store in best victim slot
        TTEntry* victim = &cluster[bestVictim];

        // Stage 6e: Preserve valuable move-carrying entries when the incoming
        // data is a shallow heuristic (NO_MOVE) in the current generation.
        const bool victimIsFresh = victim->generation() == m_generation;
        if (move == NO_MOVE && victim->move != NO_MOVE && victimIsFresh && depth < victim->depth) {
            recordSkip(StoreSkipReason::ProtectFreshMove);
            return;
        }

#ifdef TT_STATS_ENABLED
        if (!victim->isEmpty() && victim->key32 != key32) {
            m_stats.collisions++;
        }
        // Track replacement reason once we've confirmed we will overwrite
        if (victim->generation() != m_generation) {
            m_stats.replacedOldGen++;
        } else if (victim->depth < depth) {
            m_stats.replacedShallower++;
        } else if (victim->bound() != Bound::EXACT) {
            m_stats.replacedNonExact++;
        } else if (victim->move == NO_MOVE) {
            m_stats.replacedNoMove++;
        } else {
            m_stats.replacedOldest++;
        }
#endif
        ensureVerificationSlot(*victim);
        victim->save(key32, move, score, evalScore, depth, bound, m_generation);
        recordStore();
        return;
    } else {
        // Regular mode: single entry replacement
        size_t idx = index(key);
        TTEntry* entry = &m_entries[idx];

        if (policy == StorePolicy::Verification) {
            if (entry->isEmpty() || entry->hasFlag(TTEntryFlags::Exclusion)) {
                ensureVerificationSlot(*entry);
                entry->save(key32, move, score, evalScore, depth, bound, m_generation,
                            toMask(TTEntryFlags::Exclusion));
                recordStore();
            }
            else {
                recordVerificationSkip();
            }
            return;
        }
        
#ifdef TT_STATS_ENABLED
        // Check for collision BEFORE checking isEmpty()
        // A collision is when we have a non-empty entry with a different key
        if (!entry->isEmpty() && entry->key32 != key32) {
            m_stats.collisions++;
        }
#endif
        
        // Track replacement decision for diagnostics
        bool canReplace = false;
        const bool entryIsVerification = entry->hasFlag(TTEntryFlags::Exclusion);
        
        if (entry->isEmpty()) {
            // Case 1: Entry is empty - always replace
            canReplace = true;
            // Note: We'll track this in negamax.cpp where we have access to SearchData
        } else if (entry->key32 == key32) {
            // Same position - update based on depth and generation
            
            // TT pollution fix: Protect entries with moves from shallow NO_MOVE overwrites
            const bool entryIsFresh = entry->generation() == m_generation;
            if (!entryIsVerification && move == NO_MOVE && entry->move != NO_MOVE && entryIsFresh && depth <= entry->depth) {
                recordSkip(StoreSkipReason::ProtectFreshMove);
                // Don't replace a valuable move-carrying entry with a NO_MOVE heuristic
                canReplace = false;
                return;
            } else if (entryIsVerification) {
                canReplace = true;
            } else if (entry->generation() != m_generation) {
                // Case 2: Old generation - consider depth before replacing
                // IMPROVED: Only replace old gen if new search is at least as deep
                if (depth >= entry->depth - 2) {  // Allow 2 ply grace for old entries
                    canReplace = true;
                }
                else {
                    recordSkip(StoreSkipReason::DepthNotImproved);
                    return;
                }
            } else {
                // Case 3: Same generation - replace if deeper
                if (depth >= entry->depth) {
                    canReplace = true;
                } else {
                    recordSkip(StoreSkipReason::DepthNotImproved);
                    return;
                }
            }
        } else {
            // Different position (collision) - use depth-preferred replacement

            // TT pollution fix: Be more conservative with NO_MOVE heuristic entries
            if (entryIsVerification) {
                canReplace = true;
            } else if (move == NO_MOVE && depth <= entry->depth + 2) {
                // Don't displace entries for shallow heuristics
                canReplace = false;
                recordSkip(StoreSkipReason::CollisionNoMove);
                return;
            } else if (depth > entry->depth + 2) {
                // Replace if new entry is significantly deeper
                canReplace = true;
            } else if (entry->generation() != m_generation && depth >= entry->depth) {
                // Or if it's old and at least as deep
                canReplace = true;
            } else if (entry->generation() != m_generation) {
                // CRITICAL FIX: Always allow replacing very old entries to prevent TT lockup
                // Even if shallower, old entries must be replaceable
                canReplace = true;
            }
            else {
                recordSkip(StoreSkipReason::DepthNotImproved);
                return;
            }
            // Note: If none of above, canReplace stays false (same gen, shallower, with move)
        }

        if (canReplace) {
            ensureVerificationSlot(*entry);
            entry->save(key32, move, score, evalScore, depth, bound, m_generation);
            recordStore();
        }
        else {
            recordSkip(StoreSkipReason::Other);
        }
    }
}

TTEntry* TranspositionTable::probe(Hash key, int ply, CoverageKind coverageKind) {
    if (!m_enabled) return nullptr;

#ifdef TT_STATS_ENABLED
    m_stats.probes++;
#endif
    uint32_t key32 = static_cast<uint32_t>(key >> 32);
    
    if (m_clustered) {
        // Clustered mode: unrolled scan of 4 entries
#ifdef TT_STATS_ENABLED
        m_stats.clusterScans++;
#endif
        const size_t clusterIdx = clusterStart(key);  // Compute once
        TTEntry* cluster = &m_entries[clusterIdx];
        
        // Unrolled scan - minimize branches
        // Check all 4 entries without early return for better pipelining
        TTEntry* e0 = &cluster[0];
        TTEntry* e1 = &cluster[1];
        TTEntry* e2 = &cluster[2];
        TTEntry* e3 = &cluster[3];
        
        // Check for matches (most likely case first)
        if (e0->key32 == key32 && !e0->isEmpty()) {
#ifdef TT_STATS_ENABLED
            m_stats.hits++;
            m_stats.totalScanLength += 1;
#endif
            m_stats.recordProbe(ply, coverageKind, true);
            return e0;
        }
        if (e1->key32 == key32 && !e1->isEmpty()) {
#ifdef TT_STATS_ENABLED
            m_stats.hits++;
            m_stats.totalScanLength += 2;
#endif
            m_stats.recordProbe(ply, coverageKind, true);
            return e1;
        }
        if (e2->key32 == key32 && !e2->isEmpty()) {
#ifdef TT_STATS_ENABLED
            m_stats.hits++;
            m_stats.totalScanLength += 3;
#endif
            m_stats.recordProbe(ply, coverageKind, true);
            return e2;
        }
        if (e3->key32 == key32 && !e3->isEmpty()) {
#ifdef TT_STATS_ENABLED
            m_stats.hits++;
            m_stats.totalScanLength += 4;
#endif
            m_stats.recordProbe(ply, coverageKind, true);
            return e3;
        }
        
#ifdef TT_STATS_ENABLED
        // Count empties and mismatches for diagnostics
        int empties = 0;
        if (e0->isEmpty()) empties++;
        if (e1->isEmpty()) empties++;
        if (e2->isEmpty()) empties++;
        if (e3->isEmpty()) empties++;
        m_stats.probeEmpties += empties;
        m_stats.probeMismatches += (4 - empties);
        m_stats.totalScanLength += 4;
#endif
        
        m_stats.recordProbe(ply, coverageKind, false);
        return nullptr;
    } else {
        // Regular mode: single entry
        size_t idx = index(key);
        TTEntry* entry = &m_entries[idx];

#ifdef TT_STATS_ENABLED
        // Track probe-side collisions for diagnostics
        if (entry->isEmpty()) {
            m_stats.probeEmpties++;
        } else {
            if (entry->key32 == key32) {
                m_stats.hits++;
                m_stats.recordProbe(ply, coverageKind, true);
                return entry;
            } else {
                // Non-empty slot with wrong key = collision
                m_stats.probeMismatches++;
            }
        }
#else
        // Fast path without stats
        if (!entry->isEmpty() && entry->key32 == key32) {
            m_stats.recordProbe(ply, coverageKind, true);
            return entry;
        }
#endif
        m_stats.recordProbe(ply, coverageKind, false);
        return nullptr;
    }
}

void TranspositionTable::clear() {
    m_buffer.clear();
    m_stats.reset();
    m_generation = 0;
    m_roundRobin = 0;
}

void TranspositionTable::setClustered(bool clustered) {
    if (m_clustered != clustered) {
        m_clustered = clustered;
        // Note: Caller should call resize() after this to rebuild the table
    }
}

double TranspositionTable::fillRate() const {
    if (m_numEntries == 0) return 0.0;
    
    if (m_clustered) {
        // Sample clusters for performance
        constexpr size_t SAMPLE_CLUSTERS = 250;  // 250 clusters = 1000 entries
        size_t numClusters = m_numEntries / CLUSTER_SIZE;
        size_t sampleClusters = std::min(SAMPLE_CLUSTERS, numClusters);
        size_t used = 0;
        size_t total = 0;
        
        for (size_t i = 0; i < sampleClusters; ++i) {
            size_t clusterIdx = (i * numClusters) / sampleClusters * CLUSTER_SIZE;
            for (size_t j = 0; j < CLUSTER_SIZE; ++j) {
                if (!m_entries[clusterIdx + j].isEmpty()) {
                    used++;
                }
                total++;
            }
        }
        
        return total > 0 ? 100.0 * used / total : 0.0;
    } else {
        // Sample a subset of entries for performance
        constexpr size_t SAMPLE_SIZE = 1000;
        size_t sampleSize = std::min(SAMPLE_SIZE, m_numEntries);
        size_t used = 0;
        
        for (size_t i = 0; i < sampleSize; ++i) {
            size_t idx = (i * m_numEntries) / sampleSize;
            if (!m_entries[idx].isEmpty()) {
                used++;
            }
        }
        
        return 100.0 * used / sampleSize;
    }
}

size_t TranspositionTable::hashfull() const {
    if (m_numEntries == 0) return 0;
    
    if (m_clustered) {
        // Sample clusters: 250 clusters = 1000 entries
        constexpr size_t SAMPLE_CLUSTERS = 250;
        size_t numClusters = m_numEntries / CLUSTER_SIZE;
        size_t sampleClusters = std::min(SAMPLE_CLUSTERS, numClusters);
        size_t used = 0;
        size_t total = 0;
        
        for (size_t i = 0; i < sampleClusters; ++i) {
            size_t clusterIdx = (i * numClusters) / sampleClusters * CLUSTER_SIZE;
            for (size_t j = 0; j < CLUSTER_SIZE; ++j) {
                if (!m_entries[clusterIdx + j].isEmpty() && 
                    m_entries[clusterIdx + j].generation() == m_generation) {
                    used++;
                }
                total++;
            }
        }
        
        // Scale to 1000 if we sampled fewer entries
        if (total < 1000 && total > 0) {
            used = (used * 1000) / total;
        }
        
        return used;  // Returns value 0-1000
    } else {
        // Standard UCI hashfull: sample 1000 entries
        constexpr size_t SAMPLE_SIZE = 1000;
        size_t used = 0;
        
        for (size_t i = 0; i < SAMPLE_SIZE; ++i) {
            size_t idx = (i * m_numEntries) / SAMPLE_SIZE;
            if (!m_entries[idx].isEmpty() && 
                m_entries[idx].generation() == m_generation) {
                used++;
            }
        }
        
        return used;  // Returns value 0-1000
    }
}

size_t TranspositionTable::calculateNumEntries(size_t sizeInMB) {
    size_t sizeInBytes = sizeInMB * 1024 * 1024;
    size_t numEntries = sizeInBytes / sizeof(TTEntry);
    
    // Round down to nearest power of 2 for fast indexing
    if (numEntries == 0) return 1;
    
    // Find the highest bit set
    size_t powerOf2 = 1;
    while (powerOf2 <= numEntries / 2) {
        powerOf2 <<= 1;
    }
    
    return powerOf2;
}

} // namespace seajay
