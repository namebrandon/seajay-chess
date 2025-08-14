// Test that searchIterativeTest produces identical results to search
// Part of Stage 13, Deliverable 1.2a

#include "../src/core/board.h"
#include "../src/search/negamax.h"
#include "../src/core/transposition_table.h"
#include <iostream>
#include <cassert>
#include <vector>

using namespace seajay;

struct TestPosition {
    const char* fen;
    const char* name;
    int depth;
};

void testIdenticalResults() {
    // Test positions
    std::vector<TestPosition> positions = {
        {"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", "Starting position", 4},
        {"r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1", "Complex middlegame", 3},
        {"8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1", "Endgame", 4},
        {"rnbqkb1r/pp1ppppp/5n2/2p5/4P3/5N2/PPPP1PPP/RNBQKB1R w KQkq - 0 1", "Sicilian", 3}
    };
    
    TranspositionTable tt(16);  // 16MB TT
    
    for (const auto& pos : positions) {
        std::cout << "Testing: " << pos.name << std::endl;
        
        // Set up position
        Board board1, board2;
        board1.fromFEN(pos.fen);
        board2.fromFEN(pos.fen);
        
        search::SearchLimits limits;
        limits.maxDepth = pos.depth;
        
        // Clear TT between tests for consistency
        tt.clear();
        
        // Run original search
        Move originalMove = search::search(board1, limits, &tt);
        
        // Clear TT again
        tt.clear();
        
        // Run test wrapper
        Move testMove = search::searchIterativeTest(board2, limits, &tt);
        
        // Verify identical results
        if (originalMove != testMove) {
            std::cerr << "FAILED: Different moves!" << std::endl;
            std::cerr << "Original: from=" << moveFrom(originalMove) 
                     << " to=" << moveTo(originalMove) << std::endl;
            std::cerr << "Test: from=" << moveFrom(testMove) 
                     << " to=" << moveTo(testMove) << std::endl;
            assert(false);
        }
        
        std::cout << "  PASSED - Both found same move" << std::endl;
    }
    
    std::cout << "\nAll positions produced identical results!" << std::endl;
}

void testTimeManagement() {
    std::cout << "\nTesting time management..." << std::endl;
    
    Board board;
    board.setStartingPosition();
    
    TranspositionTable tt(16);
    
    search::SearchLimits limits;
    limits.movetime = std::chrono::milliseconds(100);  // 100ms time limit
    
    auto start = std::chrono::steady_clock::now();
    Move move = search::searchIterativeTest(board, limits, &tt);
    auto elapsed = std::chrono::steady_clock::now() - start;
    
    auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed);
    
    std::cout << "Time allocated: 100ms" << std::endl;
    std::cout << "Time used: " << elapsed_ms.count() << "ms" << std::endl;
    
    // Should respect time limit (with some tolerance)
    assert(elapsed_ms.count() < 150);
    assert(move != NO_MOVE);
    
    std::cout << "Time management PASSED" << std::endl;
}

int main() {
    std::cout << "Testing searchIterativeTest wrapper..." << std::endl;
    std::cout << "======================================" << std::endl;
    
    testIdenticalResults();
    testTimeManagement();
    
    std::cout << "\nAll tests PASSED!" << std::endl;
    return 0;
}