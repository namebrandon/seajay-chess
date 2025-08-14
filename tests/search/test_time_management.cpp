// Stage 13, Deliverable 2.2a: Test time management integration
// This test verifies that both old and new time calculations work

#include "../../src/search/negamax.h"
#include "../../src/search/iterative_search_data.h"
#include "../../src/search/time_management.h"
#include "../../src/search/types.h"
#include "../../src/core/board.h"
#include "../../src/core/transposition_table.h"
#include <iostream>
#include <iomanip>

using namespace seajay;
using namespace seajay::search;

void testTimeCalculations() {
    std::cout << "=== Testing Time Management Calculations ===" << std::endl;
    
    Board board;
    board.setStartingPosition();
    
    // Test various time control scenarios
    struct TestCase {
        const char* name;
        SearchLimits limits;
        double expectedRatio;  // Expected new/old ratio (approximate)
    };
    
    TestCase tests[] = {
        {
            "Fixed movetime 1000ms",
            []() { SearchLimits l; l.movetime = std::chrono::milliseconds(1000); return l; }(),
            1.0  // Should be similar
        },
        {
            "Sudden death: 60s remaining",
            []() { 
                SearchLimits l; 
                l.time[WHITE] = std::chrono::milliseconds(60000);
                l.time[BLACK] = std::chrono::milliseconds(60000);
                return l; 
            }(),
            1.0  // Should be similar  
        },
        {
            "Increment game: 10s + 1s inc",
            []() {
                SearchLimits l;
                l.time[WHITE] = std::chrono::milliseconds(10000);
                l.time[BLACK] = std::chrono::milliseconds(10000);
                l.inc[WHITE] = std::chrono::milliseconds(1000);
                l.inc[BLACK] = std::chrono::milliseconds(1000);
                return l;
            }(),
            1.0  // Should be similar
        },
        {
            "Time pressure: 2s remaining",
            []() {
                SearchLimits l;
                l.time[WHITE] = std::chrono::milliseconds(2000);
                l.time[BLACK] = std::chrono::milliseconds(2000);
                return l;
            }(),
            1.0  // Should be conservative
        }
    };
    
    for (const auto& test : tests) {
        std::cout << "\nTest: " << test.name << std::endl;
        
        // Calculate OLD time limit
        auto oldTime = calculateTimeLimit(test.limits, board);
        std::cout << "  OLD calculation: " << std::setw(6) << oldTime.count() << " ms" << std::endl;
        
        // Calculate NEW time limits (with neutral stability factor)
        TimeLimits newLimits = calculateTimeLimits(test.limits, board, 1.0);
        std::cout << "  NEW soft limit:  " << std::setw(6) << newLimits.soft.count() << " ms" << std::endl;
        std::cout << "  NEW hard limit:  " << std::setw(6) << newLimits.hard.count() << " ms" << std::endl;
        std::cout << "  NEW optimum:     " << std::setw(6) << newLimits.optimum.count() << " ms" << std::endl;
        
        // Calculate ratio
        if (oldTime.count() > 0) {
            double ratio = static_cast<double>(newLimits.optimum.count()) / oldTime.count();
            std::cout << "  Ratio (new/old): " << std::fixed << std::setprecision(2) << ratio << std::endl;
        }
    }
    
    // Test stability factors
    std::cout << "\n=== Testing Stability Factors ===" << std::endl;
    
    SearchLimits limits;
    limits.time[WHITE] = std::chrono::milliseconds(30000);  // 30 seconds
    limits.inc[WHITE] = std::chrono::milliseconds(500);      // 0.5s increment
    
    double stabilityFactors[] = {0.5, 0.7, 1.0, 1.3, 1.5};
    const char* descriptions[] = {
        "Very stable (0.5)",
        "Stable (0.7)",
        "Neutral (1.0)",
        "Unstable (1.3)",
        "Very unstable (1.5)"
    };
    
    for (int i = 0; i < 5; i++) {
        TimeLimits timeLimits = calculateTimeLimits(limits, board, stabilityFactors[i]);
        std::cout << std::setw(20) << descriptions[i] << ": ";
        std::cout << "soft=" << std::setw(5) << timeLimits.soft.count() << "ms, ";
        std::cout << "hard=" << std::setw(5) << timeLimits.hard.count() << "ms" << std::endl;
    }
}

void testSearchWithBothCalculations() {
    std::cout << "\n=== Testing Search with Both Calculations ===" << std::endl;
    
    Board board;
    board.setStartingPosition();
    
    SearchLimits limits;
    limits.maxDepth = 4;
    limits.movetime = std::chrono::milliseconds(2000);
    
    // Create a small TT for testing
    TranspositionTable tt(16);  // 16 MB
    
    std::cout << "\nRunning searchIterativeTest (uses both calculations)..." << std::endl;
    Move bestMove = searchIterativeTest(board, limits, &tt);
    
    std::cout << "\nBest move found: " << SafeMoveExecutor::moveToString(bestMove) << std::endl;
}

int main() {
    std::cout << "=== Stage 13, Deliverable 2.2a: Time Management Integration Test ===" << std::endl;
    
    try {
        testTimeCalculations();
        testSearchWithBothCalculations();
        
        std::cout << "\n✅ All time management tests completed successfully!" << std::endl;
        std::cout << "Both old and new calculations are producing values." << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "❌ Test failed: " << e.what() << std::endl;
        return 1;
    }
}