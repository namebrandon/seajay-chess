#pragma once

#include "types.h"
#include <cstddef>
#include <cstdint>
#include <memory>
#include <atomic>
#include <limits>

namespace seajay {

// Sentinel value for "no static eval stored" in TT
// Using INT16_MIN (-32768) which is extremely unlikely as a real eval
constexpr int16_t TT_EVAL_NONE = std::numeric_limits<int16_t>::min();

// Bound types for transposition table entries
enum class Bound : uint8_t {
    NONE = 0,
    EXACT = 1,
    LOWER = 2,  // Alpha bound (fail-high)
    UPPER = 3   // Beta bound (fail-low)
};

enum class TTEntryFlags : uint8_t {
    None = 0,
    Exclusion = 1 << 0,
};

constexpr inline uint8_t toMask(TTEntryFlags flag) {
    return static_cast<uint8_t>(flag);
}

// Transposition table entry - exactly 16 bytes, carefully packed
struct alignas(16) TTEntry {
    uint32_t key32;      // Upper 32 bits of zobrist key (4 bytes)
    uint16_t move;       // Best move from this position (2 bytes)
    int16_t score;       // Evaluation score (2 bytes)
    int16_t evalScore;   // Static evaluation (2 bytes)
    uint8_t depth;       // Search depth (1 byte)
    uint8_t genBound;    // Generation (6 bits) + bound type (2 bits) (1 byte)
    uint8_t flags;       // Entry flags (verification, etc.)
    uint8_t padding[3];  // Padding to reach 16 bytes alignment
    
    // Helper methods for genBound field
    uint8_t generation() const { return genBound >> 2; }
    Bound bound() const { return static_cast<Bound>(genBound & 0x03); }
    
    void save(uint32_t k, Move m, int16_t s, int16_t e, uint8_t d, Bound b, uint8_t gen, uint8_t newFlags = 0) {
        key32 = k;
        move = m;
        score = s;
        evalScore = e;
        depth = d;
        genBound = (gen << 2) | static_cast<uint8_t>(b);
        flags = newFlags;
        padding[0] = padding[1] = padding[2] = 0;
    }
    
    bool isEmpty() const {
        // Use genBound == 0 as the emptiness indicator
        // This is more robust than checking key32 and move
        // since stored entries always have a non-zero bound
        return genBound == 0;
    }

    bool hasFlag(TTEntryFlags flag) const {
        return (flags & toMask(flag)) != 0;
    }

    void clearFlags() { flags = 0; }
};

static_assert(sizeof(TTEntry) == 16, "TTEntry must be exactly 16 bytes");
static_assert(alignof(TTEntry) == 16, "TTEntry must be 16-byte aligned");

// Enable TT stats only in debug builds or when explicitly requested
#if defined(DEBUG) || defined(ENABLE_TT_STATS)
#define TT_STATS_ENABLED
#endif

// Statistics for transposition table operations
struct TTStats {
#ifdef TT_STATS_ENABLED
    // Use relaxed memory ordering for performance
    std::atomic<uint64_t> probes{0};
    std::atomic<uint64_t> hits{0};
    std::atomic<uint64_t> stores{0};
    std::atomic<uint64_t> collisions{0};
    
    // Probe-side collision tracking for better diagnostics
    std::atomic<uint64_t> probeEmpties{0};      // Probed an empty slot
    std::atomic<uint64_t> probeMismatches{0};   // Probed non-empty with wrong key (real collision)
    
    // Clustered mode specific stats
    std::atomic<uint64_t> clusterScans{0};      // Total cluster scans
    std::atomic<uint64_t> totalScanLength{0};   // Sum of scan lengths
    std::atomic<uint64_t> replacedEmpty{0};     // Replaced empty slot
    std::atomic<uint64_t> replacedOldGen{0};    // Replaced old generation
    std::atomic<uint64_t> replacedShallower{0}; // Replaced shallower depth
    std::atomic<uint64_t> replacedNonExact{0};  // Replaced non-EXACT bound
    std::atomic<uint64_t> replacedNoMove{0};    // Replaced NO_MOVE entry
    std::atomic<uint64_t> replacedOldest{0};    // Replaced oldest (round-robin)
#else
    // Dummy fields for release builds
    uint64_t probes = 0;
    uint64_t hits = 0;
    uint64_t stores = 0;
    uint64_t collisions = 0;
    uint64_t probeEmpties = 0;
    uint64_t probeMismatches = 0;
    uint64_t clusterScans = 0;
    uint64_t totalScanLength = 0;
    uint64_t replacedEmpty = 0;
    uint64_t replacedOldGen = 0;
    uint64_t replacedShallower = 0;
    uint64_t replacedNonExact = 0;
    uint64_t replacedNoMove = 0;
    uint64_t replacedOldest = 0;
#endif
    
    void reset() {
        probes = 0;
        hits = 0;
        stores = 0;
        collisions = 0;
        probeEmpties = 0;
        probeMismatches = 0;
        clusterScans = 0;
        totalScanLength = 0;
        replacedEmpty = 0;
        replacedOldGen = 0;
        replacedShallower = 0;
        replacedNonExact = 0;
        replacedNoMove = 0;
        replacedOldest = 0;
    }
    
    double hitRate() const {
#ifdef TT_STATS_ENABLED
        uint64_t p = probes.load(std::memory_order_relaxed);
        return p > 0 ? (100.0 * hits.load(std::memory_order_relaxed) / p) : 0.0;
#else
        return probes > 0 ? (100.0 * hits / probes) : 0.0;
#endif
    }
    
    double collisionRate() const {
#ifdef TT_STATS_ENABLED
        uint64_t p = probes.load(std::memory_order_relaxed);
        return p > 0 ? (100.0 * probeMismatches.load(std::memory_order_relaxed) / p) : 0.0;
#else
        return probes > 0 ? (100.0 * probeMismatches / probes) : 0.0;
#endif
    }
    
    double avgScanLength() const {
#ifdef TT_STATS_ENABLED
        uint64_t scans = clusterScans.load(std::memory_order_relaxed);
        return scans > 0 ? (double)totalScanLength.load(std::memory_order_relaxed) / scans : 0.0;
#else
        return clusterScans > 0 ? (double)totalScanLength / clusterScans : 0.0;
#endif
    }
};

// Aligned memory buffer for transposition table
class AlignedBuffer {
public:
    AlignedBuffer() = default;
    explicit AlignedBuffer(size_t size);
    ~AlignedBuffer();
    
    // Disable copy, enable move
    AlignedBuffer(const AlignedBuffer&) = delete;
    AlignedBuffer& operator=(const AlignedBuffer&) = delete;
    AlignedBuffer(AlignedBuffer&& other) noexcept;
    AlignedBuffer& operator=(AlignedBuffer&& other) noexcept;
    
    void* data() const { return m_data; }
    size_t size() const { return m_size; }
    void resize(size_t newSize);
    void clear();
    
private:
    void* m_data = nullptr;
    size_t m_size = 0;
    
    void free();
};

// Main transposition table class
class TranspositionTable {
public:
    enum class StorePolicy : uint8_t {
        Primary,
        Verification,
    };

    class StorePolicyGuard {
    public:
        explicit StorePolicyGuard(StorePolicy policy);
        ~StorePolicyGuard();

        StorePolicyGuard(const StorePolicyGuard&) = delete;
        StorePolicyGuard& operator=(const StorePolicyGuard&) = delete;

    private:
        StorePolicy m_previous;
    };

    // Default sizes
    static constexpr size_t DEFAULT_SIZE_MB_DEBUG = 16;
    static constexpr size_t DEFAULT_SIZE_MB_RELEASE = 16;
    
    // Clustering configuration
    static constexpr size_t CLUSTER_SIZE = 4;
    
    TranspositionTable();
    explicit TranspositionTable(size_t sizeInMB);
    ~TranspositionTable() = default;
    
    // Core operations
    void store(Hash key, Move move, int16_t score, int16_t evalScore, 
               uint8_t depth, Bound bound);
    TTEntry* probe(Hash key);
    void clear();
    
    // Configuration
    void resize(size_t sizeInMB);
    void setEnabled(bool enabled) { m_enabled = enabled; }
    bool isEnabled() const { return m_enabled; }
    void newSearch() { m_generation = (m_generation + 1) & 0x3F; } // 6-bit generation
    
    // Clustering support
    void setClustered(bool clustered);
    bool isClustered() const { return m_clustered; }
    
    // Statistics
    const TTStats& stats() const { return m_stats; }
    void resetStats() { m_stats.reset(); }
    size_t size() const { return m_numEntries; }
    size_t sizeInBytes() const { return m_numEntries * sizeof(TTEntry); }
    size_t sizeInMB() const { return sizeInBytes() / (1024 * 1024); }
    double fillRate() const;
    
    // Utility
    size_t hashfull() const;  // Returns permille (0-1000) of entries used
    
    // Prefetch hint for upcoming probe
    void prefetch(Hash key) const {
        if (m_enabled && m_entries) {
            size_t idx = clusterStart(key);
            __builtin_prefetch(&m_entries[idx], 0, 1);
        }
    }
    
private:
    AlignedBuffer m_buffer;
    TTEntry* m_entries;
    size_t m_numEntries;
    size_t m_mask;  // For fast modulo operation (size must be power of 2)
    uint8_t m_generation;
    bool m_enabled;
    bool m_clustered;  // Use 4-way clustering
    mutable TTStats m_stats;
    mutable uint8_t m_roundRobin{0};  // For round-robin replacement in clusters
    
    size_t index(Hash key) const {
        // Mix the key for better distribution and apply mask
        return (key * 0x9E3779B97F4A7C15ULL) & m_mask;
    }
    
    size_t clusterStart(Hash key) const {
        size_t raw = index(key);
        return m_clustered ? (raw & ~(CLUSTER_SIZE - 1)) : raw;
    }
    
    // Select victim entry within a cluster for replacement
    
    static size_t calculateNumEntries(size_t sizeInMB);
};

} // namespace seajay
