#include "transposition_table.h"
#include <cstring>
#include <cstdlib>
#include <algorithm>
#include <bit>
#include <cassert>
#include <climits>

namespace seajay {

// AlignedBuffer implementation
AlignedBuffer::AlignedBuffer(size_t size) : m_size(size) {
    if (size > 0) {
        // Allocate 64-byte aligned memory for cache line optimization
        // C standard requires size to be a multiple of alignment
        size_t alignedSize = (size + 63) & ~size_t(63);  // Round up to multiple of 64
        m_data = std::aligned_alloc(64, alignedSize);
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
            // C standard requires size to be a multiple of alignment
            size_t alignedSize = (newSize + 63) & ~size_t(63);  // Round up to multiple of 64
            m_data = std::aligned_alloc(64, alignedSize);
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
        std::free(m_data);
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
    : m_generation(0), m_enabled(true), m_clustered(false) {
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
                               int16_t evalScore, uint8_t depth, Bound bound) {
    if (!m_enabled) return;
    
    m_stats.stores++;
    uint32_t key32 = static_cast<uint32_t>(key >> 32);
    
    if (m_clustered) {
        // Clustered mode: select victim within cluster
        size_t clusterIdx = clusterStart(key);
        TTEntry* cluster = &m_entries[clusterIdx];
        
        // Select victim within cluster
        int victim = selectVictim(cluster, key, move, depth);
        
        if (victim >= 0) {
            TTEntry* entry = &cluster[victim];
            
            // Track collision if replacing a different position
            if (!entry->isEmpty() && entry->key32 != key32) {
                m_stats.collisions++;
            }
            
            // Store the new entry
            entry->save(key32, move, score, evalScore, depth, bound, m_generation);
        }
    } else {
        // Regular mode: single entry replacement
        size_t idx = index(key);
        TTEntry* entry = &m_entries[idx];
        
        // FIX: Check for collision BEFORE checking isEmpty()
        // A collision is when we have a non-empty entry with a different key
        if (!entry->isEmpty() && entry->key32 != key32) {
            m_stats.collisions++;
        }
        
        // Track replacement decision for diagnostics
        bool canReplace = false;
        
        if (entry->isEmpty()) {
            // Case 1: Entry is empty - always replace
            canReplace = true;
            // Note: We'll track this in negamax.cpp where we have access to SearchData
        } else if (entry->key32 == key32) {
            // Same position - update based on depth and generation
            
            // TT pollution fix: Protect entries with moves from NO_MOVE overwrites
            if (move == NO_MOVE && entry->move != NO_MOVE && depth <= entry->depth) {
                // Don't replace a valuable move-carrying entry with a NO_MOVE heuristic
                canReplace = false;
            } else if (entry->generation() != m_generation) {
                // Case 2: Old generation - consider depth before replacing
                // IMPROVED: Only replace old gen if new search is at least as deep
                if (depth >= entry->depth - 2) {  // Allow 2 ply grace for old entries
                    canReplace = true;
                }
            } else {
                // Case 3: Same generation - replace if deeper
                if (depth >= entry->depth) {
                    canReplace = true;
                }
            }
        } else {
            // Different position (collision) - use depth-preferred replacement
            
            // TT pollution fix: Be more conservative with NO_MOVE heuristic entries
            if (move == NO_MOVE && depth <= entry->depth + 2) {
                // Don't displace entries for shallow heuristics
                canReplace = false;
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
            // Note: If none of above, canReplace stays false (same gen, shallower, with move)
        }
        
        if (canReplace) {
            entry->save(key32, move, score, evalScore, depth, bound, m_generation);
        }
    }
}

TTEntry* TranspositionTable::probe(Hash key) {
    if (!m_enabled) return nullptr;
    
    m_stats.probes++;
    uint32_t key32 = static_cast<uint32_t>(key >> 32);
    
    if (m_clustered) {
        // Clustered mode: scan 4 entries
        m_stats.clusterScans++;
        size_t clusterIdx = clusterStart(key);
        int scanLength = 0;
        
        for (size_t i = 0; i < CLUSTER_SIZE; ++i) {
            scanLength++;
            TTEntry* entry = &m_entries[clusterIdx + i];
            
            if (entry->isEmpty()) {
                m_stats.probeEmpties++;
                continue;
            }
            
            if (entry->key32 == key32) {
                m_stats.hits++;
                m_stats.totalScanLength += scanLength;
                return entry;
            }
            
            // Non-empty with wrong key = mismatch
            m_stats.probeMismatches++;
        }
        
        // No match found in cluster
        m_stats.totalScanLength += scanLength;
        return nullptr;
    } else {
        // Regular mode: single entry
        size_t idx = index(key);
        TTEntry* entry = &m_entries[idx];
        
        // Track probe-side collisions for diagnostics
        if (entry->isEmpty()) {
            m_stats.probeEmpties++;
        } else {
            if (entry->key32 == key32) {
                m_stats.hits++;
                return entry;
            } else {
                // Non-empty slot with wrong key = collision
                m_stats.probeMismatches++;
            }
        }
        
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

int TranspositionTable::selectVictim(const TTEntry* cluster, Hash key, Move move, uint8_t depth) const {
    uint32_t key32 = static_cast<uint32_t>(key >> 32);
    
    // Replacement policy priority (from design doc):
    // 1) Empty slot
    // 2) Old generation entries  
    // 3) Shallower depth entries
    // 4) Non-EXACT bound entries
    // 5) NO_MOVE entries
    // 6) Oldest (round-robin)
    
    int bestVictim = -1;
    int bestScore = INT32_MAX;  // Lower score = better victim
    
    for (size_t i = 0; i < CLUSTER_SIZE; ++i) {
        const TTEntry& entry = cluster[i];
        int score = 0;
        
        // Priority 1: Empty slot (immediate return)
        if (entry.isEmpty()) {
            m_stats.replacedEmpty++;
            return i;
        }
        
        // Same position - always prefer to update it
        if (entry.key32 == key32) {
            // Special case: protect move-carrying entries from NO_MOVE overwrites
            if (move == NO_MOVE && entry.move != NO_MOVE && depth <= entry.depth) {
                score = INT32_MAX;  // Never replace
            } else {
                return i;  // Always update same position if allowed
            }
        } else {
            // Different position - apply replacement policy
            
            // Priority 2: Old generation (significant weight)
            if (entry.generation() != m_generation) {
                score -= 10000;
            }
            
            // Priority 3: Depth (lower depth = better victim)
            score += entry.depth * 100;
            
            // Priority 4: Bound type (non-EXACT is preferred victim)
            if (entry.bound() != Bound::EXACT) {
                score -= 50;
            }
            
            // Priority 5: NO_MOVE entries are preferred victims
            if (entry.move == NO_MOVE) {
                score -= 25;
            }
            
            // Special penalty: Don't evict deep entries with moves for shallow NO_MOVE
            if (move == NO_MOVE && entry.move != NO_MOVE) {
                score += 200;  // Make it less likely to be victim
            }
        }
        
        if (score < bestScore) {
            bestScore = score;
            bestVictim = i;
        }
    }
    
    // Track replacement reason (approximate based on score)
    if (bestVictim >= 0) {
        const TTEntry& victim = cluster[bestVictim];
        if (victim.generation() != m_generation) {
            m_stats.replacedOldGen++;
        } else if (victim.depth < depth) {
            m_stats.replacedShallower++;
        } else if (victim.bound() != Bound::EXACT) {
            m_stats.replacedNonExact++;
        } else if (victim.move == NO_MOVE) {
            m_stats.replacedNoMove++;
        } else {
            m_stats.replacedOldest++;
        }
    }
    
    // Fallback: round-robin if somehow no victim selected (shouldn't happen)
    if (bestVictim < 0) {
        bestVictim = (m_roundRobin++) % CLUSTER_SIZE;
        m_stats.replacedOldest++;
    }
    
    return bestVictim;
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