/**
 * SeaJay Chess Engine - Stage 12: Transposition Tables
 * Transposition Table Unit Tests
 * 
 * Phase 2: Basic TT Structure Implementation Tests
 */

#include "../test_framework.h"
#include "core/board.h"
#include "core/types.h"
#include "core/transposition_table.h"
#include <iostream>
#include <random>
#include <vector>
#include <atomic>
#include <cstring>
#include <chrono>

using namespace seajay;


// Note: Using the actual implementation from core/transposition_table.h
// The test file includes stub implementations for future phases (clustering)

// ============================================================================
// Test Suite
// ============================================================================

TEST_CASE(TT_MemoryAlignment) {
    SECTION("TTEntry is 16 bytes") {
        REQUIRE(sizeof(TTEntry) == 16);
    }
    
    SECTION("TTEntry is properly aligned") {
        REQUIRE(alignof(TTEntry) == 16);
    }
    
    SECTION("AlignedBuffer allocates correctly") {
        AlignedBuffer buffer(1024 * 1024);  // 1MB
        REQUIRE(buffer.data() != nullptr);
        REQUIRE(buffer.size() == 1024 * 1024);
        // Check alignment
        REQUIRE(reinterpret_cast<uintptr_t>(buffer.data()) % 64 == 0);
    }
}

TEST_CASE(TT_BasicOperations) {
    TranspositionTable tt(1);  // 1 MB for testing
    
    SECTION("Store and retrieve") {
        uint64_t key = 0x123456789ABCDEF0ULL;
        int score = 100;
        int evalScore = 50;
        int depth = 10;
        Move move = makeMove(E2, E4, NORMAL);
        
        tt.store(key, move, score, evalScore, depth, Bound::EXACT);
        
        TTEntry* entry = tt.probe(key);
        REQUIRE(entry != nullptr);
        REQUIRE(entry->score == score);
        REQUIRE(entry->evalScore == evalScore);
        REQUIRE(entry->depth == depth);
        REQUIRE(entry->move == move);
        REQUIRE(entry->bound() == Bound::EXACT);
    }
    
    SECTION("Key validation") {
        uint64_t key1 = 0x123456789ABCDEF0ULL;
        uint64_t key2 = 0x123456789ABCDEF1ULL;  // Different lower bits
        uint64_t key3 = 0x223456789ABCDEF0ULL;  // Different upper bits
        
        Move move = makeMove(E2, E4, NORMAL);
        tt.store(key1, move, 100, 50, 10, Bound::EXACT);
        
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
        
        Move move1 = makeMove(E2, E4, NORMAL);
        Move move2 = makeMove(D2, D4, NORMAL);
        
        tt.store(key, move1, 100, 50, 10, Bound::EXACT);
        tt.store(key, move2, 200, 60, 12, Bound::LOWER);
        
        TTEntry* entry = tt.probe(key);
        REQUIRE(entry != nullptr);
        REQUIRE(entry->score == 200);
        REQUIRE(entry->depth == 12);
        REQUIRE(entry->move == move2);
        REQUIRE(entry->bound() == Bound::LOWER);
    }
}

TEST_CASE(TT_Statistics) {
    TranspositionTable tt(1);
    
    SECTION("Hit rate calculation") {
        tt.resetStats();
        
        Move nullMove = makeMove(0, 0, NORMAL);
        
        // Store some entries
        for (uint64_t i = 0; i < 100; i++) {
            tt.store(i, nullMove, static_cast<int>(i), 0, 5, Bound::EXACT);
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
    TranspositionTable tt(1);
    
    SECTION("Disabled TT returns nullptr") {
        uint64_t key = 0x123456789ABCDEF0ULL;
        Move move = makeMove(E2, E4, NORMAL);
        
        tt.store(key, move, 100, 50, 10, Bound::EXACT);
        REQUIRE(tt.probe(key) != nullptr);
        
        tt.setEnabled(false);
        REQUIRE(tt.probe(key) == nullptr);
        
        tt.setEnabled(true);
        REQUIRE(tt.probe(key) != nullptr);
    }
}

TEST_CASE(TT_GenerationManagement) {
    TranspositionTable tt(1);
    
    SECTION("Generation increments correctly") {
        uint64_t key = 0x123456789ABCDEF0ULL;
        Move move1 = makeMove(E2, E4, NORMAL);
        Move move2 = makeMove(D2, D4, NORMAL);
        
        tt.store(key, move1, 100, 50, 10, Bound::EXACT);
        TTEntry* entry = tt.probe(key);
        REQUIRE(entry != nullptr);
        uint8_t gen1 = entry->generation();
        
        tt.newSearch();
        tt.store(key, move2, 200, 60, 12, Bound::LOWER);
        entry = tt.probe(key);
        REQUIRE(entry != nullptr);
        uint8_t gen2 = entry->generation();
        
        REQUIRE(gen2 == ((gen1 + 1) & 0x3F));
    }
}

// Clustered implementation tests will be added in Phase 6

TEST_CASE(TT_CollisionHandling) {
    TranspositionTable tt(1);  // Small table to force collisions
    
    SECTION("Collision detection") {
        tt.resetStats();
        
        Move move = makeMove(E2, E4, NORMAL);
        
        // Create keys that will collide (same index, different upper bits)
        std::vector<uint64_t> keys;
        uint64_t base = 0x1000;
        
        // These will likely map to same index with small table
        for (int i = 0; i < 10; i++) {
            keys.push_back(base + (static_cast<uint64_t>(i) << 32));
        }
        
        // Store all keys
        for (auto key : keys) {
            tt.store(key, move, 100, 50, 10, Bound::EXACT);
        }
        
        // Check collision count
        auto& stats = tt.stats();
        REQUIRE(stats.collisions > 0);  // Should have some collisions
    }
}

TEST_CASE(TT_ClearOperation) {
    TranspositionTable tt(1);
    
    SECTION("Clear removes all entries") {
        Move nullMove = makeMove(0, 0, NORMAL);
        
        // Store some entries
        for (uint64_t i = 0; i < 100; i++) {
            tt.store(i, nullMove, static_cast<int>(i), 0, 5, Bound::EXACT);
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
    TranspositionTable tt(16);  // 16 MB for stress test
    
    std::mt19937_64 rng(12345);
    std::uniform_int_distribution<uint64_t> keyDist;
    std::uniform_int_distribution<int> scoreDist(-1000, 1000);
    std::uniform_int_distribution<int> depthDist(1, 20);
    std::uniform_int_distribution<Square> squareDist(0, 63);
    
    std::cout << "Running TT stress test with " << iterations 
              << " operations...\n";
    
    auto start = std::chrono::high_resolution_clock::now();
    
    for (size_t i = 0; i < iterations; i++) {
        uint64_t key = keyDist(rng);
        
        // 70% store, 30% probe
        if (rng() % 10 < 7) {
            Move move = makeMove(squareDist(rng), squareDist(rng), NORMAL);
            tt.store(key, move, scoreDist(rng), 0, depthDist(rng), 
                    Bound::EXACT);
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
    
    // Print stats
    auto& stats = tt.stats();
    std::cout << "TT Statistics:\n";
    std::cout << "  Probes:     " << stats.probes.load() << "\n";
    std::cout << "  Hits:       " << stats.hits.load() << "\n";
    std::cout << "  Hit Rate:   " << stats.hitRate() << "%\n";
    std::cout << "  Stores:     " << stats.stores.load() << "\n";
    std::cout << "  Collisions: " << stats.collisions.load() << "\n";
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