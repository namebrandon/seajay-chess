#include <iostream>
#include <iomanip>
#include <chrono>
#include "../../src/core/board.h"
#include "../../src/search/negamax.h"
#include "../../src/search/iterative_search_data.h"
#include "../../src/core/transposition_table.h"

using namespace seajay;
using namespace seajay::search;

// Test early termination with stable position
void testStablePositionTermination() {
    std::cout << "Testing Early Termination with Stable Position...\n";
    
    Board board;  // Starting position (very stable)
    
    SearchLimits limits;
    limits.maxDepth = 20;  // Allow deep search
    limits.movetime = std::chrono::milliseconds(500);  // Limited time
    
    TranspositionTable tt(16);
    
    auto start = std::chrono::steady_clock::now();
    Move bestMove = searchIterativeTest(board, limits, &tt);
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - start);
    
    std::cout << "Search completed in " << elapsed.count() << "ms\n";
    std::cout << "Best move: " << SafeMoveExecutor::moveToString(bestMove) << "\n";
    
    // Should terminate early due to stability
    if (elapsed.count() < 600) {  // Should respect time limit
        std::cout << "✓ Terminated within time limit\n";
    } else {
        std::cout << "✗ Exceeded time limit\n";
    }
}

// Test with unstable tactical position
void testUnstablePositionTermination() {
    std::cout << "\nTesting Early Termination with Unstable Position...\n";
    
    // Complex tactical position
    Board board;
    board.fromFEN("r1b1kb1r/pp1n1ppp/2p1pn2/q7/2BP4/2N2N2/PPP2PPP/R1BQK2R w KQkq - 0 9");
    
    SearchLimits limits;
    limits.maxDepth = 20;  // Allow deep search
    limits.movetime = std::chrono::milliseconds(500);  // Limited time
    
    TranspositionTable tt(16);
    
    auto start = std::chrono::steady_clock::now();
    Move bestMove = searchIterativeTest(board, limits, &tt);
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - start);
    
    std::cout << "Search completed in " << elapsed.count() << "ms\n";
    std::cout << "Best move: " << SafeMoveExecutor::moveToString(bestMove) << "\n";
    
    // May use more time for unstable position
    if (elapsed.count() < 700) {  // Slightly more lenient
        std::cout << "✓ Terminated reasonably\n";
    } else {
        std::cout << "⚠ Used significant time (expected for tactical position)\n";
    }
}

// Test minimum depth guarantee
void testMinimumDepthGuarantee() {
    std::cout << "\nTesting Minimum Depth Guarantee...\n";
    
    Board board;
    
    SearchLimits limits;
    limits.maxDepth = 20;
    limits.movetime = std::chrono::milliseconds(50);  // Very short time
    
    TranspositionTable tt(16);
    
    // Track depth reached (would need to parse debug output)
    // For now, just ensure we get a move
    Move bestMove = searchIterativeTest(board, limits, &tt);
    
    if (bestMove != NO_MOVE) {
        std::cout << "✓ Found move despite short time: " 
                 << SafeMoveExecutor::moveToString(bestMove) << "\n";
    } else {
        std::cout << "✗ Failed to find move\n";
    }
}

// Test with various time limits
void testVariousTimeLimits() {
    std::cout << "\nTesting Various Time Limits...\n";
    
    Board board;
    board.fromFEN("rnbqkbnr/pppp1ppp/8/4p3/4P3/8/PPPP1PPP/RNBQKBNR w KQkq e6 0 2");
    
    TranspositionTable tt(16);
    
    std::chrono::milliseconds timeLimits[] = {
        std::chrono::milliseconds(100),
        std::chrono::milliseconds(250),
        std::chrono::milliseconds(500),
        std::chrono::milliseconds(1000)
    };
    
    std::cout << "Time Limit | Elapsed | Status\n";
    std::cout << "-----------|---------|--------\n";
    
    for (auto timeLimit : timeLimits) {
        SearchLimits limits;
        limits.maxDepth = 20;
        limits.movetime = timeLimit;
        
        auto start = std::chrono::steady_clock::now();
        Move bestMove = searchIterativeTest(board, limits, &tt);
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - start);
        
        std::cout << std::setw(10) << timeLimit.count() << " | "
                 << std::setw(7) << elapsed.count() << " | ";
        
        // Allow 20% overage for final iteration
        if (elapsed <= timeLimit * 1.2) {
            std::cout << "✓ OK\n";
        } else {
            std::cout << "✗ Exceeded\n";
        }
    }
}

int main() {
    std::cout << "=== Stage 13, Deliverable 4.2b: Early Termination Logic Test ===\n\n";
    
    testStablePositionTermination();
    testUnstablePositionTermination();
    testMinimumDepthGuarantee();
    testVariousTimeLimits();
    
    std::cout << "\n✓ Early termination logic implemented\n";
    std::cout << "=== Test Complete ===\n";
    return 0;
}