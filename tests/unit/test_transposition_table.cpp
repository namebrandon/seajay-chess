/**
 * SeaJay Chess Engine - Stage 12: Transposition Tables
 * Transposition Table Unit Tests
 * 
 * Phase 0: Test Infrastructure Foundation
 * These tests will validate the TT implementation
 */

#include "../test_framework.h"
#include "core/board.h"
#include "core/types.h"
#include <iostream>
#include <random>
#include <vector>
#include <atomic>
#include <cstring>

using namespace seajay;

// Stub implementation for Move extension (needed for testing)
namespace seajay {
    inline std::string moveToString(Move m) {
        if (m == 0) return "none";
        std::string str;
        Square fromSq = moveFrom(m);
        Square toSq = moveTo(m);
        str += static_cast<char>('a' + fileOf(fromSq));
        str += static_cast<char>('1' + rankOf(fromSq));
        str += static_cast<char>('a' + fileOf(toSq));
        str += static_cast<char>('1' + rankOf(toSq));
        if (isPromotion(m)) {
            PieceType pt = promotionType(m);
            const char* pieces = "nbrq";
            str += pieces[pt - KNIGHT];
        }
        return str;
    }
}

// Forward declarations for future TT implementation
namespace seajay {

// Bound types for TT entries
enum class TTBound : uint8_t {
    NONE = 0,
    EXACT = 1,
    LOWER = 2,  // Beta cutoff, score is lower bound
    UPPER = 3   // Alpha failed, score is upper bound
};

/**
 * Transposition Table Entry Structure
 * 16 bytes, carefully packed for cache efficiency
 */
struct alignas(16) TTEntry {
    uint32_t key32;      // Upper 32 bits of zobrist key for validation
    uint16_t move;       // Best move from this position
    int16_t score;       // Evaluation score
    int16_t evalScore;   // Static evaluation (for future eval pruning)
    uint8_t depth;       // Search depth
    uint8_t genBound;    // Generation (6 bits) + Bound type (2 bits)
    
    // Helper methods
    uint8_t generation() const { return genBound >> 2; }
    TTBound bound() const { return static_cast<TTBound>(genBound & 0x03); }
    
    void save(uint32_t k, int16_t s, int16_t ev, uint8_t d, 
              uint16_t m, TTBound b, uint8_t gen) {
        key32 = k;
        score = s;
        evalScore = ev;
        depth = d;
        move = m;
        genBound = (gen << 2) | static_cast<uint8_t>(b);
    }
    
    bool isEmpty() const { return key32 == 0 && depth == 0; }
};

static_assert(sizeof(TTEntry) == 16, "TTEntry must be exactly 16 bytes");

/**
 * TT Cluster for improved collision handling
 * 64 bytes = 3 entries + padding for cache line alignment
 */
struct alignas(64) TTCluster {
    TTEntry entries[3];
    uint8_t padding[16];  // Ensure 64-byte alignment
};

static_assert(sizeof(TTCluster) == 64, "TTCluster must be exactly 64 bytes");

/**
 * Transposition Table Statistics
 */
struct TTStats {
    std::atomic<uint64_t> probes{0};
    std::atomic<uint64_t> hits{0};
    std::atomic<uint64_t> stores{0};
    std::atomic<uint64_t> collisions{0};
    std::atomic<uint64_t> overwrites{0};
    
    void reset() {
        probes = 0;
        hits = 0;
        stores = 0;
        collisions = 0;
        overwrites = 0;
    }
    
    double hitRate() const {
        uint64_t p = probes.load();
        return p > 0 ? (100.0 * hits.load() / p) : 0.0;
    }
    
    void print() const {
        std::cout << "TT Statistics:\n";
        std::cout << "  Probes:     " << probes.load() << "\n";
        std::cout << "  Hits:       " << hits.load() << "\n";
        std::cout << "  Hit Rate:   " << hitRate() << "%\n";
        std::cout << "  Stores:     " << stores.load() << "\n";
        std::cout << "  Collisions: " << collisions.load() << "\n";
        std::cout << "  Overwrites: " << overwrites.load() << "\n";
    }
};

/**
 * RAII wrapper for aligned memory allocation
 */
template<typename T, size_t Alignment>
class AlignedBuffer {
private:
    T* m_data = nullptr;
    size_t m_size = 0;
    
public:
    AlignedBuffer() = default;
    
    explicit AlignedBuffer(size_t size) : m_size(size) {
        allocate(size);
    }
    
    ~AlignedBuffer() {
        deallocate();
    }
    
    // Delete copy operations
    AlignedBuffer(const AlignedBuffer&) = delete;
    AlignedBuffer& operator=(const AlignedBuffer&) = delete;
    
    // Move operations
    AlignedBuffer(AlignedBuffer&& other) noexcept 
        : m_data(other.m_data), m_size(other.m_size) {
        other.m_data = nullptr;
        other.m_size = 0;
    }
    
    AlignedBuffer& operator=(AlignedBuffer&& other) noexcept {
        if (this != &other) {
            deallocate();
            m_data = other.m_data;
            m_size = other.m_size;
            other.m_data = nullptr;
            other.m_size = 0;
        }
        return *this;
    }
    
    void allocate(size_t size) {
        deallocate();
        m_size = size;
        size_t bytes = size * sizeof(T);
        
        #ifdef _WIN32
            m_data = static_cast<T*>(_aligned_malloc(bytes, Alignment));
        #else
            m_data = static_cast<T*>(std::aligned_alloc(Alignment, bytes));
        #endif
        
        if (m_data) {
            std::memset(m_data, 0, bytes);  // Zero-initialize
        }
    }
    
    void deallocate() {
        if (m_data) {
            #ifdef _WIN32
                _aligned_free(m_data);
            #else
                std::free(m_data);
            #endif
            m_data = nullptr;
            m_size = 0;
        }
    }
    
    T* data() { return m_data; }
    const T* data() const { return m_data; }
    size_t size() const { return m_size; }
    
    T& operator[](size_t idx) { return m_data[idx]; }
    const T& operator[](size_t idx) const { return m_data[idx]; }
    
    bool isAligned() const {
        return (reinterpret_cast<uintptr_t>(m_data) % Alignment) == 0;
    }
};

/**
 * Basic Transposition Table Implementation
 * This will be enhanced throughout the phases
 */
class TranspositionTable {
private:
    AlignedBuffer<TTEntry, 16> m_entries;
    size_t m_mask;
    bool m_enabled;
    uint8_t m_generation;
    TTStats m_stats;
    
    // Debug/validation modes
    #ifdef TT_DEBUG
    bool m_validateOnStore = true;
    bool m_validateOnProbe = true;
    #endif
    
public:
    TranspositionTable() : m_enabled(true), m_generation(0) {}
    
    void resize(size_t mbSize) {
        size_t entryCount = (mbSize * 1024 * 1024) / sizeof(TTEntry);
        
        // Round down to power of 2 for efficient masking
        size_t size = 1;
        while (size * 2 <= entryCount) {
            size *= 2;
        }
        
        m_entries.allocate(size);
        m_mask = size - 1;
        clear();
        
        std::cout << "TT resized to " << mbSize << " MB (" 
                  << size << " entries)\n";
    }
    
    void clear() {
        if (m_entries.data()) {
            std::memset(m_entries.data(), 0, 
                       m_entries.size() * sizeof(TTEntry));
        }
        m_stats.reset();
    }
    
    void newSearch() {
        m_generation = (m_generation + 1) & 0x3F;  // 6-bit generation
    }
    
    TTEntry* probe(uint64_t key) {
        if (!m_enabled || !m_entries.data()) return nullptr;
        
        m_stats.probes++;
        
        size_t idx = key & m_mask;
        TTEntry* entry = &m_entries[idx];
        
        #ifdef TT_DEBUG
        if (m_validateOnProbe) {
            validateEntry(entry, key);
        }
        #endif
        
        // Check if key matches (upper 32 bits)
        if (entry->key32 == static_cast<uint32_t>(key >> 32)) {
            m_stats.hits++;
            return entry;
        }
        
        return nullptr;
    }
    
    void store(uint64_t key, int score, int evalScore, int depth, 
               uint16_t move, TTBound bound) {
        if (!m_enabled || !m_entries.data()) return;
        
        m_stats.stores++;
        
        size_t idx = key & m_mask;
        TTEntry* entry = &m_entries[idx];
        
        // Check if we're overwriting
        if (!entry->isEmpty()) {
            m_stats.overwrites++;
            if (entry->key32 != static_cast<uint32_t>(key >> 32)) {
                m_stats.collisions++;
            }
        }
        
        #ifdef TT_DEBUG
        if (m_validateOnStore) {
            validateStore(key, score, depth, bound);
        }
        #endif
        
        // Always replace for now (Phase 2)
        entry->save(static_cast<uint32_t>(key >> 32),
                   static_cast<int16_t>(score),
                   static_cast<int16_t>(evalScore),
                   static_cast<uint8_t>(depth),
                   move, bound, m_generation);
    }
    
    void setEnabled(bool enable) { m_enabled = enable; }
    bool isEnabled() const { return m_enabled; }
    
    const TTStats& stats() const { return m_stats; }
    void resetStats() { m_stats.reset(); }
    
    // Validation helpers for debug mode
    #ifdef TT_DEBUG
    void validateEntry(TTEntry* entry, uint64_t key) {
        if (!entry) return;
        
        // Check alignment
        if (reinterpret_cast<uintptr_t>(entry) % 16 != 0) {
            std::cerr << "TT Entry not aligned!\n";
        }
        
        // Check bound type
        if (entry->bound() > TTBound::UPPER) {
            std::cerr << "Invalid bound type: " 
                     << static_cast<int>(entry->bound()) << "\n";
        }
    }
    
    void validateStore(uint64_t key, int score, int depth, TTBound bound) {
        // Check score bounds
        constexpr int MATE_SCORE = 30000;
        if (std::abs(score) > MATE_SCORE + 100) {
            std::cerr << "Score out of bounds: " << score << "\n";
        }
        
        // Check depth
        if (depth < 0 || depth > 100) {
            std::cerr << "Invalid depth: " << depth << "\n";
        }
        
        // Check bound
        if (bound == TTBound::NONE) {
            std::cerr << "Storing NONE bound type\n";
        }
    }
    #endif
    
    // Prefetch for future SMP support
    void prefetch(uint64_t key) {
        #ifdef __GNUC__
        size_t idx = key & m_mask;
        __builtin_prefetch(&m_entries[idx], 0, 1);
        #endif
    }
    
    // Test interface for validation
    size_t capacity() const { return m_mask + 1; }
    bool verify() const { return m_entries.isAligned(); }
};

/**
 * Three-Entry Cluster Implementation (Phase 6)
 */
class ClusteredTranspositionTable {
private:
    AlignedBuffer<TTCluster, 64> m_clusters;
    size_t m_clusterCount;
    bool m_enabled;
    uint8_t m_generation;
    TTStats m_stats;
    
public:
    void resize(size_t mbSize) {
        size_t byteSize = mbSize * 1024 * 1024;
        m_clusterCount = byteSize / sizeof(TTCluster);
        
        // Round down to power of 2
        size_t size = 1;
        while (size * 2 <= m_clusterCount) {
            size *= 2;
        }
        m_clusterCount = size;
        
        m_clusters.allocate(m_clusterCount);
        clear();
    }
    
    TTEntry* probe(uint64_t key) {
        if (!m_enabled || !m_clusters.data()) return nullptr;
        
        m_stats.probes++;
        
        size_t idx = (key % m_clusterCount);
        TTCluster& cluster = m_clusters[idx];
        
        uint32_t key32 = static_cast<uint32_t>(key >> 32);
        
        // Check all 3 entries in cluster
        for (int i = 0; i < 3; i++) {
            if (cluster.entries[i].key32 == key32) {
                m_stats.hits++;
                return &cluster.entries[i];
            }
        }
        
        return nullptr;
    }
    
    void store(uint64_t key, int score, int evalScore, int depth,
              uint16_t move, TTBound bound) {
        if (!m_enabled || !m_clusters.data()) return;
        
        m_stats.stores++;
        
        size_t idx = (key % m_clusterCount);
        TTCluster& cluster = m_clusters[idx];
        
        uint32_t key32 = static_cast<uint32_t>(key >> 32);
        
        // Find matching or empty entry
        TTEntry* replace = nullptr;
        for (int i = 0; i < 3; i++) {
            if (cluster.entries[i].key32 == key32 || 
                cluster.entries[i].isEmpty()) {
                replace = &cluster.entries[i];
                break;
            }
        }
        
        // If no match/empty, use replacement strategy
        if (!replace) {
            replace = selectReplacement(cluster, depth);
            m_stats.collisions++;
        }
        
        if (replace && !replace->isEmpty()) {
            m_stats.overwrites++;
        }
        
        if (replace) {
            replace->save(key32, static_cast<int16_t>(score),
                         static_cast<int16_t>(evalScore),
                         static_cast<uint8_t>(depth),
                         move, bound, m_generation);
        }
    }
    
    void clear() {
        if (m_clusters.data()) {
            std::memset(m_clusters.data(), 0,
                       m_clusterCount * sizeof(TTCluster));
        }
        m_stats.reset();
    }
    
    void setEnabled(bool enable) { m_enabled = enable; }
    const TTStats& stats() const { return m_stats; }
    
private:
    TTEntry* selectReplacement(TTCluster& cluster, int depth) {
        // Simple strategy: replace lowest depth from old generation
        TTEntry* candidate = &cluster.entries[0];
        
        for (int i = 1; i < 3; i++) {
            TTEntry& entry = cluster.entries[i];
            
            // Prefer replacing old generation entries
            if (entry.generation() != m_generation &&
                candidate->generation() == m_generation) {
                candidate = &entry;
            }
            // Within same generation, replace lower depth
            else if (entry.generation() == candidate->generation() &&
                    entry.depth < candidate->depth) {
                candidate = &entry;
            }
        }
        
        return candidate;
    }
};

} // namespace seajay

// ============================================================================
// Test Suite
// ============================================================================

TEST_CASE(TT_MemoryAlignment) {
    SECTION("TTEntry is 16 bytes") {
        REQUIRE(sizeof(TTEntry) == 16);
    }
    
    SECTION("TTCluster is 64 bytes") {
        REQUIRE(sizeof(TTCluster) == 64);
    }
    
    SECTION("AlignedBuffer allocates correctly") {
        AlignedBuffer<TTEntry, 16> buffer(1024);
        REQUIRE(buffer.isAligned());
        REQUIRE(buffer.size() == 1024);
    }
}

TEST_CASE(TT_BasicOperations) {
    TranspositionTable tt;
    tt.resize(1);  // 1 MB for testing
    
    SECTION("Store and retrieve") {
        uint64_t key = 0x123456789ABCDEF0ULL;
        int score = 100;
        int evalScore = 50;
        int depth = 10;
        uint16_t move = 0x1234;
        
        tt.store(key, score, evalScore, depth, move, TTBound::EXACT);
        
        TTEntry* entry = tt.probe(key);
        REQUIRE(entry != nullptr);
        REQUIRE(entry->score == score);
        REQUIRE(entry->evalScore == evalScore);
        REQUIRE(entry->depth == depth);
        REQUIRE(entry->move == move);
        REQUIRE(entry->bound() == TTBound::EXACT);
    }
    
    SECTION("Key validation") {
        uint64_t key1 = 0x123456789ABCDEF0ULL;
        uint64_t key2 = 0x123456789ABCDEF1ULL;  // Different lower bits
        uint64_t key3 = 0x223456789ABCDEF0ULL;  // Different upper bits
        
        tt.store(key1, 100, 50, 10, 0x1234, TTBound::EXACT);
        
        // Same key should hit
        REQUIRE(tt.probe(key1) != nullptr);
        
        // Different lower bits, same index, same upper 32 - should hit
        TTEntry* entry2 = tt.probe(key2);
        // This depends on masking, might or might not hit
        
        // Different upper 32 bits - should not hit
        TTEntry* entry3 = tt.probe(key3);
        if (entry3) {
            // If we got an entry, it shouldn't match our key
            REQUIRE(entry3->key32 != (key3 >> 32));
        }
    }
    
    SECTION("Overwrite behavior") {
        uint64_t key = 0x123456789ABCDEF0ULL;
        
        tt.store(key, 100, 50, 10, 0x1234, TTBound::EXACT);
        tt.store(key, 200, 60, 12, 0x5678, TTBound::LOWER);
        
        TTEntry* entry = tt.probe(key);
        REQUIRE(entry != nullptr);
        REQUIRE(entry->score == 200);
        REQUIRE(entry->depth == 12);
        REQUIRE(entry->move == 0x5678);
        REQUIRE(entry->bound() == TTBound::LOWER);
    }
}

TEST_CASE(TT_Statistics) {
    TranspositionTable tt;
    tt.resize(1);
    
    SECTION("Hit rate calculation") {
        tt.resetStats();
        
        // Store some entries
        for (uint64_t i = 0; i < 100; i++) {
            tt.store(i, static_cast<int>(i), 0, 5, 0, TTBound::EXACT);
        }
        
        // Probe them back
        int hits = 0;
        for (uint64_t i = 0; i < 100; i++) {
            if (tt.probe(i)) hits++;
        }
        
        // Probe some that don't exist
        for (uint64_t i = 100; i < 200; i++) {
            tt.probe(i);
        }
        
        auto& stats = tt.stats();
        REQUIRE(stats.probes == 200);
        REQUIRE(stats.hits == hits);
        REQUIRE(stats.stores == 100);
        
        double hitRate = stats.hitRate();
        REQUIRE(hitRate == Approx(50.0).margin(10.0));
    }
}

TEST_CASE(TT_EnableDisable) {
    TranspositionTable tt;
    tt.resize(1);
    
    SECTION("Disabled TT returns nullptr") {
        uint64_t key = 0x123456789ABCDEF0ULL;
        
        tt.store(key, 100, 50, 10, 0x1234, TTBound::EXACT);
        REQUIRE(tt.probe(key) != nullptr);
        
        tt.setEnabled(false);
        REQUIRE(tt.probe(key) == nullptr);
        
        tt.setEnabled(true);
        REQUIRE(tt.probe(key) != nullptr);
    }
}

TEST_CASE(TT_GenerationManagement) {
    TranspositionTable tt;
    tt.resize(1);
    
    SECTION("Generation increments correctly") {
        uint64_t key = 0x123456789ABCDEF0ULL;
        
        tt.store(key, 100, 50, 10, 0x1234, TTBound::EXACT);
        TTEntry* entry = tt.probe(key);
        REQUIRE(entry != nullptr);
        uint8_t gen1 = entry->generation();
        
        tt.newSearch();
        tt.store(key, 200, 60, 12, 0x5678, TTBound::LOWER);
        entry = tt.probe(key);
        REQUIRE(entry != nullptr);
        uint8_t gen2 = entry->generation();
        
        REQUIRE(gen2 == ((gen1 + 1) & 0x3F));
    }
}

TEST_CASE(TT_ClusteredImplementation) {
    ClusteredTranspositionTable tt;
    tt.resize(1);
    
    SECTION("Store and retrieve in cluster") {
        // Store 3 entries that hash to same cluster
        uint64_t base = 0x123456789ABCDEF0ULL;
        
        for (int i = 0; i < 3; i++) {
            uint64_t key = base + (i << 32);  // Different upper bits
            tt.store(key, 100 + i, 50, 10, 0x1234 + i, TTBound::EXACT);
        }
        
        // All 3 should be retrievable
        for (int i = 0; i < 3; i++) {
            uint64_t key = base + (i << 32);
            TTEntry* entry = tt.probe(key);
            REQUIRE(entry != nullptr);
            REQUIRE(entry->score == 100 + i);
        }
    }
    
    SECTION("Replacement in full cluster") {
        ClusteredTranspositionTable tt;
        tt.resize(1);
        
        uint64_t base = 0x123456789ABCDEF0ULL;
        
        // Fill cluster
        for (int i = 0; i < 3; i++) {
            uint64_t key = base + (i << 32);
            tt.store(key, 100, 50, 10, 0x1234, TTBound::EXACT);
        }
        
        // Store 4th entry - should replace one
        uint64_t key4 = base + (3ULL << 32);
        tt.store(key4, 400, 50, 5, 0x9999, TTBound::EXACT);
        
        // New entry should be retrievable
        REQUIRE(tt.probe(key4) != nullptr);
        
        // At least 2 of the original 3 should still be there
        int found = 0;
        for (int i = 0; i < 3; i++) {
            uint64_t key = base + (i << 32);
            if (tt.probe(key)) found++;
        }
        REQUIRE(found >= 2);
    }
}

TEST_CASE(TT_CollisionHandling) {
    TranspositionTable tt;
    tt.resize(1);  // Small table to force collisions
    
    SECTION("Collision detection") {
        tt.resetStats();
        
        // Create keys that will collide (same index, different upper bits)
        std::vector<uint64_t> keys;
        uint64_t base = 0x1000;
        
        // These will likely map to same index with small table
        for (int i = 0; i < 10; i++) {
            keys.push_back(base + (static_cast<uint64_t>(i) << 32));
        }
        
        // Store all keys
        for (auto key : keys) {
            tt.store(key, 100, 50, 10, 0x1234, TTBound::EXACT);
        }
        
        // Check collision count
        auto& stats = tt.stats();
        REQUIRE(stats.collisions > 0);  // Should have some collisions
    }
}

TEST_CASE(TT_ClearOperation) {
    TranspositionTable tt;
    tt.resize(1);
    
    SECTION("Clear removes all entries") {
        // Store some entries
        for (uint64_t i = 0; i < 100; i++) {
            tt.store(i, static_cast<int>(i), 0, 5, 0, TTBound::EXACT);
        }
        
        // Verify some are there
        REQUIRE(tt.probe(0) != nullptr);
        REQUIRE(tt.probe(50) != nullptr);
        
        // Clear
        tt.clear();
        
        // Verify all gone
        for (uint64_t i = 0; i < 100; i++) {
            REQUIRE(tt.probe(i) == nullptr);
        }
        
        // Stats should be reset
        auto& stats = tt.stats();
        REQUIRE(stats.probes == 100);  // From the probes above
        REQUIRE(stats.hits == 0);
        REQUIRE(stats.stores == 0);
    }
}

// ============================================================================
// Stress Testing Helpers
// ============================================================================

void stressTestTT(size_t iterations) {
    TranspositionTable tt;
    tt.resize(16);  // 16 MB for stress test
    
    std::mt19937_64 rng(12345);
    std::uniform_int_distribution<uint64_t> keyDist;
    std::uniform_int_distribution<int> scoreDist(-1000, 1000);
    std::uniform_int_distribution<int> depthDist(1, 20);
    
    std::cout << "Running TT stress test with " << iterations 
              << " operations...\n";
    
    auto start = std::chrono::high_resolution_clock::now();
    
    for (size_t i = 0; i < iterations; i++) {
        uint64_t key = keyDist(rng);
        
        // 70% store, 30% probe
        if (rng() % 10 < 7) {
            tt.store(key, scoreDist(rng), 0, depthDist(rng), 
                    0, TTBound::EXACT);
        } else {
            tt.probe(key);
        }
        
        // Occasionally clear
        if (i % 10000 == 0) {
            tt.newSearch();
        }
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    std::cout << "Completed in " << duration.count() << "ms\n";
    tt.stats().print();
}

// Main test runner
int main(int argc, char* argv[]) {
    std::cout << "SeaJay Stage 12: Transposition Table Unit Tests\n";
    std::cout << "===============================================\n\n";
    
    // Run stress test if requested
    if (argc > 1 && std::string(argv[1]) == "--stress") {
        size_t iterations = 1000000;
        if (argc > 2) {
            iterations = std::stoull(argv[2]);
        }
        stressTestTT(iterations);
        return 0;
    }
    
    // Run catch2 tests
    return Catch::Session().run(argc, argv);
}