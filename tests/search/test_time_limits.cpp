// Stage 13, Deliverable 2.2b: Test that new time management respects limits

#include "../../src/search/negamax.h"
#include "../../src/search/types.h"
#include "../../src/core/board.h"
#include "../../src/core/transposition_table.h"
#include <iostream>
#include <iomanip>
#include <chrono>

using namespace seajay;
using namespace seajay::search;

bool testFixedMoveTime() {
    std::cout << "\n=== Testing Fixed Move Time ===" << std::endl;
    
    Board board;
    board.setStartingPosition();
    
    SearchLimits limits;
    limits.maxDepth = 10;  // High depth to ensure time runs out first
    limits.movetime = std::chrono::milliseconds(500);  // 500ms fixed time
    
    TranspositionTable tt(16);
    
    auto start = std::chrono::steady_clock::now();
    Move bestMove = searchIterativeTest(board, limits, &tt);
    auto end = std::chrono::steady_clock::now();
    
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    std::cout << "Fixed time limit: 500ms" << std::endl;
    std::cout << "Actual time used: " << elapsed.count() << "ms" << std::endl;
    std::cout << "Best move: " << SafeMoveExecutor::moveToString(bestMove) << std::endl;
    
    // Allow some overhead (up to 100ms)
    if (elapsed.count() > 600) {
        std::cerr << "❌ FAILED: Exceeded fixed move time by too much!" << std::endl;
        return false;
    }
    
    std::cout << "✅ PASSED: Respected fixed move time" << std::endl;
    return true;
}

bool testSuddenDeath() {
    std::cout << "\n=== Testing Sudden Death Time Control ===" << std::endl;
    
    Board board;
    board.setStartingPosition();
    
    SearchLimits limits;
    limits.maxDepth = 10;
    limits.time[WHITE] = std::chrono::milliseconds(5000);  // 5 seconds remaining
    limits.time[BLACK] = std::chrono::milliseconds(5000);
    
    TranspositionTable tt(16);
    
    auto start = std::chrono::steady_clock::now();
    Move bestMove = searchIterativeTest(board, limits, &tt);
    auto end = std::chrono::steady_clock::now();
    
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    std::cout << "Time remaining: 5000ms" << std::endl;
    std::cout << "Actual time used: " << elapsed.count() << "ms" << std::endl;
    std::cout << "Best move: " << SafeMoveExecutor::moveToString(bestMove) << std::endl;
    
    // Should use much less than all remaining time (safety)
    if (elapsed.count() > 2500) {  // Should not use more than 50%
        std::cerr << "❌ FAILED: Used too much of remaining time!" << std::endl;
        return false;
    }
    
    if (elapsed.count() < 50) {  // Should use at least some time
        std::cerr << "❌ FAILED: Used too little time!" << std::endl;
        return false;
    }
    
    std::cout << "✅ PASSED: Conservative time usage" << std::endl;
    return true;
}

bool testIncrementTime() {
    std::cout << "\n=== Testing Increment Time Control ===" << std::endl;
    
    Board board;
    board.setStartingPosition();
    
    SearchLimits limits;
    limits.maxDepth = 10;
    limits.time[WHITE] = std::chrono::milliseconds(3000);  // 3 seconds
    limits.time[BLACK] = std::chrono::milliseconds(3000);
    limits.inc[WHITE] = std::chrono::milliseconds(1000);   // 1 second increment
    limits.inc[BLACK] = std::chrono::milliseconds(1000);
    
    TranspositionTable tt(16);
    
    auto start = std::chrono::steady_clock::now();
    Move bestMove = searchIterativeTest(board, limits, &tt);
    auto end = std::chrono::steady_clock::now();
    
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    std::cout << "Time remaining: 3000ms + 1000ms inc" << std::endl;
    std::cout << "Actual time used: " << elapsed.count() << "ms" << std::endl;
    std::cout << "Best move: " << SafeMoveExecutor::moveToString(bestMove) << std::endl;
    
    // With increment, can use a bit more time
    if (elapsed.count() > 2000) {  // Still shouldn't use most of base time
        std::cerr << "❌ FAILED: Used too much time despite increment!" << std::endl;
        return false;
    }
    
    std::cout << "✅ PASSED: Reasonable time usage with increment" << std::endl;
    return true;
}

bool testTimePressure() {
    std::cout << "\n=== Testing Time Pressure ===" << std::endl;
    
    Board board;
    board.setStartingPosition();
    
    SearchLimits limits;
    limits.maxDepth = 10;
    limits.time[WHITE] = std::chrono::milliseconds(200);  // Only 200ms left!
    limits.time[BLACK] = std::chrono::milliseconds(200);
    
    TranspositionTable tt(16);
    
    auto start = std::chrono::steady_clock::now();
    Move bestMove = searchIterativeTest(board, limits, &tt);
    auto end = std::chrono::steady_clock::now();
    
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    std::cout << "Time remaining: 200ms (critical!)" << std::endl;
    std::cout << "Actual time used: " << elapsed.count() << "ms" << std::endl;
    std::cout << "Best move: " << SafeMoveExecutor::moveToString(bestMove) << std::endl;
    
    // Should be very conservative
    if (elapsed.count() > 100) {  // Should keep at least 100ms
        std::cerr << "❌ FAILED: Used too much time under pressure!" << std::endl;
        return false;
    }
    
    if (bestMove == NO_MOVE) {
        std::cerr << "❌ FAILED: No move returned!" << std::endl;
        return false;
    }
    
    std::cout << "✅ PASSED: Very conservative under time pressure" << std::endl;
    return true;
}

int main() {
    std::cout << "=== Stage 13, Deliverable 2.2b: Time Limit Respect Test ===" << std::endl;
    std::cout << "Testing that new time management respects various time controls..." << std::endl;
    
    bool allPassed = true;
    
    allPassed &= testFixedMoveTime();
    allPassed &= testSuddenDeath();
    allPassed &= testIncrementTime();
    allPassed &= testTimePressure();
    
    std::cout << "\n=== Final Result ===" << std::endl;
    if (allPassed) {
        std::cout << "✅ All time limit tests PASSED!" << std::endl;
        std::cout << "New time management correctly respects all time controls." << std::endl;
        return 0;
    } else {
        std::cerr << "❌ Some tests FAILED!" << std::endl;
        return 1;
    }
}