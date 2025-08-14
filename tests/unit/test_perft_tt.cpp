/**
 * SeaJay Chess Engine - Stage 12: Transposition Tables
 * Phase 3: Perft TT Integration Test
 * 
 * Validates that perft with TT produces identical results to perft without TT.
 * Also measures TT effectiveness and collision rates.
 */

#include "../test_framework.h"
#include "core/board.h"
#include "core/perft.h"
#include "core/transposition_table.h"
#include <iostream>
#include <vector>

using namespace seajay;

TEST_CASE(PerftTT_Correctness) {
    // Test positions with known perft values
    struct TestCase {
        std::string fen;
        int depth;
        uint64_t expected;
    };
    
    std::vector<TestCase> tests = {
        {"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", 4, 197281},
        {"r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1", 3, 97862},
        {"8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1", 4, 43238},
        {"rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8", 3, 62379},
    };
    
    TranspositionTable tt(16);  // 16MB for testing
    
    for (const auto& test : tests) {
        Board board;
        REQUIRE(board.fromFEN(test.fen));
        
        SECTION("Position: " + test.fen.substr(0, 30) + "... depth " + std::to_string(test.depth)) {
            // Run without TT
            uint64_t nodesWithoutTT = Perft::perft(board, test.depth);
            REQUIRE(nodesWithoutTT == test.expected);
            
            // Clear and reset TT
            tt.clear();
            tt.resetStats();
            
            // Run with TT (cold cache)
            uint64_t nodesWithTT = Perft::perftWithTT(board, test.depth, tt);
            REQUIRE(nodesWithTT == test.expected);
            
            // Get stats from cold run
            const auto& coldStats = tt.stats();
            uint64_t coldStores = coldStats.stores.load();
            uint64_t coldHits = coldStats.hits.load();
            
            // Run with TT again (warm cache)
            uint64_t nodesWarmTT = Perft::perftWithTT(board, test.depth, tt);
            REQUIRE(nodesWarmTT == test.expected);
            
            // Get stats from warm run
            const auto& warmStats = tt.stats();
            uint64_t totalHits = warmStats.hits.load();
            uint64_t warmHits = totalHits - coldHits;
            
            // Warm cache should have more hits
            REQUIRE(warmHits > 0);
            
            // Check collision rate is reasonable
            if (coldStores > 0) {
                double collisionRate = 100.0 * coldStats.collisions.load() / coldStores;
                REQUIRE(collisionRate < 5.0);  // Less than 5% collisions
            }
        }
    }
}

TEST_CASE(PerftTT_Performance) {
    TranspositionTable tt(32);  // 32MB for performance test
    
    SECTION("TT provides speedup") {
        Board board;
        board.fromFEN("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1");
        
        // Measure without TT
        auto result1 = Perft::runPerft(board, 4, false, nullptr);
        
        // Clear TT for fair comparison
        tt.clear();
        tt.resetStats();
        
        // Measure with TT (cold)
        auto result2 = Perft::runPerft(board, 4, true, &tt);
        
        // Verify correctness
        REQUIRE(result1.nodes == result2.nodes);
        
        // TT should provide some speedup even on cold cache
        // due to transpositions within the search
        double speedup = result1.timeSeconds / result2.timeSeconds;
        REQUIRE(speedup > 1.0);
        
        // Measure with TT (warm)
        auto result3 = Perft::runPerft(board, 4, true, &tt);
        
        // Warm cache should be much faster
        double warmSpeedup = result1.timeSeconds / result3.timeSeconds;
        REQUIRE(warmSpeedup > 10.0);  // At least 10x speedup on warm cache
    }
}

TEST_CASE(PerftTT_EncodingLimits) {
    SECTION("Small values encode correctly") {
        for (uint64_t i = 0; i <= 32767; i += 1000) {
            int16_t encoded = Perft::encodeNodeCount(i);
            REQUIRE(encoded == static_cast<int16_t>(i));
            uint64_t decoded = Perft::decodeNodeCount(encoded);
            REQUIRE(decoded == i);
        }
    }
    
    SECTION("Large values are marked as uncacheable") {
        uint64_t largeValue = 1000000;
        int16_t encoded = Perft::encodeNodeCount(largeValue);
        REQUIRE(encoded == -1);  // Special marker for "too large"
    }
}

TEST_CASE(PerftTT_DivideCorrectness) {
    TranspositionTable tt(8);  // Small TT
    
    Board board;
    board.fromFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    
    // Get divide results with and without TT
    auto divideNoTT = Perft::perftDivide(board, 3);
    auto divideWithTT = Perft::perftDivideWithTT(board, 3, tt);
    
    // Total nodes should match
    REQUIRE(divideNoTT.totalNodes == divideWithTT.totalNodes);
    REQUIRE(divideNoTT.totalNodes == 8902);  // Known value for startpos depth 3
    
    // Each move should have same node count
    REQUIRE(divideNoTT.moveNodes.size() == divideWithTT.moveNodes.size());
    for (const auto& pair : divideNoTT.moveNodes) {
        REQUIRE(divideWithTT.moveNodes.count(pair.first) == 1);
        REQUIRE(divideWithTT.moveNodes.at(pair.first) == pair.second);
    }
}

int main(int argc, char* argv[]) {
    std::cout << "SeaJay Stage 12 Phase 3: Perft TT Integration Tests\n";
    std::cout << "===================================================\n\n";
    
    return Catch::Session().run(argc, argv);
}