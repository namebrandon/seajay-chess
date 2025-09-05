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

// Transposition table entry - exactly 16 bytes, carefully packed
struct alignas(16) TTEntry {
    uint32_t key32;      // Upper 32 bits of zobrist key (4 bytes)
    uint16_t move;       // Best move from this position (2 bytes)
    int16_t score;       // Evaluation score (2 bytes)
    int16_t evalScore;   // Static evaluation (2 bytes)
    uint8_t depth;       // Search depth (1 byte)
    uint8_t genBound;    // Generation (6 bits) + bound type (2 bits) (1 byte)
    uint8_t padding[4];  // Padding to reach 16 bytes alignment
    
    // Helper methods for genBound field
    uint8_t generation() const { return genBound >> 2; }
    Bound bound() const { return static_cast<Bound>(genBound & 0x03); }
    
    void save(uint32_t k, Move m, int16_t s, int16_t e, uint8_t d, Bound b, uint8_t gen) {
        key32 = k;
        move = m;
        score = s;
        evalScore = e;
        depth = d;
        genBound = (gen << 2) | static_cast<uint8_t>(b);
    }
    
    bool isEmpty() const {
        // Use genBound == 0 as the emptiness indicator
        // This is more robust than checking key32 and move
        // since stored entries always have a non-zero bound
        return genBound == 0;
    }
};

static_assert(sizeof(TTEntry) == 16, "TTEntry must be exactly 16 bytes");
static_assert(alignof(TTEntry) == 16, "TTEntry must be 16-byte aligned");

// Statistics for transposition table operations
struct TTStats {
    std::atomic<uint64_t> probes{0};
    std::atomic<uint64_t> hits{0};
    std::atomic<uint64_t> stores{0};
    std::atomic<uint64_t> collisions{0};
    
    // Probe-side collision tracking for better diagnostics
    std::atomic<uint64_t> probeEmpties{0};      // Probed an empty slot
    std::atomic<uint64_t> probeMismatches{0};   // Probed non-empty with wrong key (real collision)
    
    void reset() {
        probes = 0;
        hits = 0;
        stores = 0;
        collisions = 0;
        probeEmpties = 0;
        probeMismatches = 0;
    }
    
    double hitRate() const {
        uint64_t p = probes.load();
        return p > 0 ? (100.0 * hits.load() / p) : 0.0;
    }
    
    double collisionRate() const {
        uint64_t p = probes.load();
        return p > 0 ? (100.0 * probeMismatches.load() / p) : 0.0;
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
    // Default sizes
    static constexpr size_t DEFAULT_SIZE_MB_DEBUG = 16;
    static constexpr size_t DEFAULT_SIZE_MB_RELEASE = 16;
    
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
            size_t idx = index(key);
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
    mutable TTStats m_stats;
    
    size_t index(Hash key) const {
        // Mix the key for better distribution and apply mask
        return (key * 0x9E3779B97F4A7C15ULL) & m_mask;
    }
    
    static size_t calculateNumEntries(size_t sizeInMB);
};

} // namespace seajay