#include <iostream>
#include <iomanip>
#include <chrono>
#include "../../src/core/board.h"
#include "../../src/search/negamax.h"
#include "../../src/search/aspiration_window.h"
#include "../../src/evaluation/types.h"
#include "../../src/core/transposition_table.h"

using namespace seajay;
using namespace seajay::search;
using namespace seajay::eval;

// Test that re-search has a hard limit of 5 attempts
void testResearchLimits() {
    std::cout << "Testing Re-search Limits (5 attempts max)...\n";
    
    // Test window that will fail multiple times
    Score previousScore(0);
    AspirationWindow window = calculateInitialWindow(previousScore, 6);
    
    // Simulate extreme score that will keep failing
    Score extremeScore(5000);  // Way outside any reasonable window
    
    int attempts = 0;
    while (!window.exceedsMaxAttempts() && attempts < 10) {  // Safety limit
        window = widenWindow(window, extremeScore, true);
        attempts++;
        
        std::cout << "Attempt " << window.attempts << ": ";
        if (window.isInfinite()) {
            std::cout << "INFINITE WINDOW (search will terminate)\n";
            break;
        } else {
            std::cout << "[" << window.alpha.value() << ", " << window.beta.value() 
                     << "] delta=" << window.delta << "\n";
        }
    }
    
    if (window.attempts == 5 && window.isInfinite()) {
        std::cout << "✓ Correctly limited to 5 attempts and switched to infinite\n";
    } else {
        std::cout << "✗ Should limit to 5 attempts\n";
    }
}

// Test pathological position that might cause many re-searches
void testPathologicalPosition() {
    std::cout << "\nTesting Pathological Position (should not hang)...\n";
    
    // Complex tactical position with many forcing moves
    // This position has wild score swings between depths
    Board board;
    board.fromFEN("r1b1kb1r/pp1n1ppp/2p1pn2/q7/2BP4/2N2N2/PPP2PPP/R1BQK2R w KQkq - 0 9");
    
    SearchLimits limits;
    limits.maxDepth = 6;  // Deep enough to trigger aspiration
    limits.movetime = std::chrono::milliseconds(2000);  // 2 second limit
    
    TranspositionTable tt(16);  // 16MB table
    
    // Start timing
    auto start = std::chrono::steady_clock::now();
    
    // Run search - should not hang despite pathological score behavior
    Move bestMove = searchIterativeTest(board, limits, &tt);
    
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - start);
    
    std::cout << "Search completed in " << elapsed.count() << "ms\n";
    std::cout << "Best move: " << SafeMoveExecutor::moveToString(bestMove) << "\n";
    
    // Verify search didn't hang (should complete within time limit + margin)
    if (elapsed.count() < 2500) {  // 500ms margin
        std::cout << "✓ Search terminated properly without hanging\n";
    } else {
        std::cout << "✗ Search took too long (possible hang)\n";
    }
}

// Full regression test with various positions
void regressionTest() {
    std::cout << "\nRunning Full Regression Test...\n";
    
    struct TestPosition {
        const char* fen;
        const char* description;
        int expectedDepth;  // Minimum depth we should reach
    };
    
    TestPosition positions[] = {
        {"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", 
         "Starting position", 6},
        {"r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1",
         "Complex middlegame", 5},
        {"8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1",
         "Rook endgame", 6},
        {"r1bqkb1r/pppp1ppp/2n2n2/4p3/2B1P3/5N2/PPPP1PPP/RNBQK2R w KQkq - 0 1",
         "Italian opening", 6},
        {"rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8",
         "Promotion position", 5}
    };
    
    TranspositionTable tt(16);
    int passed = 0;
    int total = sizeof(positions) / sizeof(positions[0]);
    
    for (const auto& pos : positions) {
        std::cout << "\nTesting: " << pos.description << "\n";
        Board board;
        board.fromFEN(pos.fen);
        
        SearchLimits limits;
        limits.maxDepth = 8;  // Allow deep search
        limits.movetime = std::chrono::milliseconds(500);  // 500ms per position
        
        auto start = std::chrono::steady_clock::now();
        Move bestMove = searchIterativeTest(board, limits, &tt);
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - start);
        
        if (bestMove != NO_MOVE) {
            std::cout << "  Found: " << SafeMoveExecutor::moveToString(bestMove) 
                     << " in " << elapsed.count() << "ms\n";
            passed++;
        } else {
            std::cout << "  FAILED: No move found\n";
        }
    }
    
    std::cout << "\n=== Regression Results: " << passed << "/" << total 
              << " positions passed ===\n";
    
    if (passed == total) {
        std::cout << "✓ All positions passed regression test\n";
    } else {
        std::cout << "✗ Some positions failed\n";
    }
}

int main() {
    std::cout << "=== Stage 13, Deliverable 3.2e: Re-search Limits Test ===\n\n";
    
    testResearchLimits();
    testPathologicalPosition();
    regressionTest();
    
    std::cout << "\n=== Tests Complete ===\n";
    return 0;
}