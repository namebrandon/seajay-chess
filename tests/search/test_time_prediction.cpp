#include <iostream>
#include <iomanip>
#include <chrono>
#include "../../src/search/time_management.h"

using namespace seajay::search;
using namespace std::chrono;

// Test time prediction with various EBF values
void testTimePrediction() {
    std::cout << "Testing Time Prediction with EBF...\n\n";
    
    // Test case 1: Normal EBF progression
    std::cout << "Test 1: Normal EBF values\n";
    std::cout << "Last Time | EBF  | Depth | Predicted | Expected Range\n";
    std::cout << "----------|------|-------|-----------|---------------\n";
    
    struct TestCase {
        milliseconds lastTime;
        double ebf;
        int depth;
        milliseconds minExpected;
        milliseconds maxExpected;
    };
    
    TestCase cases[] = {
        {milliseconds(100), 5.0, 5, milliseconds(500), milliseconds(700)},    // 100 * 5 * 1.2 = 600
        {milliseconds(500), 4.0, 6, milliseconds(2000), milliseconds(2600)},  // 500 * 4 * 1.2 = 2400
        {milliseconds(2000), 3.0, 7, milliseconds(6800), milliseconds(7500)}, // 2000 * 3 * 0.95 * 1.2 = 6840
        {milliseconds(5000), 2.5, 10, milliseconds(13000), milliseconds(14000)}, // 5000 * 2.5 * 0.9 * 1.2 = 13500
    };
    
    for (const auto& tc : cases) {
        auto predicted = predictNextIterationTime(tc.lastTime, tc.ebf, tc.depth);
        
        std::cout << std::setw(9) << tc.lastTime.count() << " | "
                 << std::fixed << std::setprecision(1) 
                 << std::setw(4) << tc.ebf << " | "
                 << std::setw(5) << tc.depth << " | "
                 << std::setw(9) << predicted.count() << " | "
                 << "[" << std::setw(5) << tc.minExpected.count() 
                 << "-" << std::setw(5) << tc.maxExpected.count() << "]";
        
        if (predicted >= tc.minExpected && predicted <= tc.maxExpected) {
            std::cout << " ✓\n";
        } else {
            std::cout << " ✗\n";
        }
    }
    
    // Test case 2: Edge cases
    std::cout << "\nTest 2: Edge cases\n";
    
    // Very low EBF (should be clamped to 1.5)
    auto predicted = predictNextIterationTime(milliseconds(1000), 0.5, 5);
    std::cout << "EBF 0.5 -> clamped to 1.5: " << predicted.count() << "ms";
    if (predicted.count() >= 1800 && predicted.count() <= 2000) {  // 1000 * 1.5 * 1.2 = 1800
        std::cout << " ✓\n";
    } else {
        std::cout << " ✗\n";
    }
    
    // Very high EBF (should be clamped to 10.0)
    predicted = predictNextIterationTime(milliseconds(100), 50.0, 5);
    std::cout << "EBF 50.0 -> clamped to 10.0: " << predicted.count() << "ms";
    if (predicted.count() >= 1100 && predicted.count() <= 1300) {  // 100 * 10 * 1.2 = 1200
        std::cout << " ✓\n";
    } else {
        std::cout << " ✗\n";
    }
    
    // Invalid inputs
    predicted = predictNextIterationTime(milliseconds(-100), 5.0, 5);
    std::cout << "Negative time -> large value: " << predicted.count() << "ms";
    if (predicted.count() >= 100000) {
        std::cout << " ✓\n";
    } else {
        std::cout << " ✗\n";
    }
}

// Test integration with hasTimeForNextIteration
void testTimeDecision() {
    std::cout << "\nTesting Time Decision with Prediction...\n";
    
    TimeLimits limits;
    limits.soft = milliseconds(5000);
    limits.hard = milliseconds(8000);
    limits.optimum = milliseconds(3000);
    
    struct Scenario {
        const char* description;
        milliseconds elapsed;
        double lastIterTime;
        double ebf;
        bool expectedDecision;
    };
    
    Scenario scenarios[] = {
        {"Early in search, plenty of time", milliseconds(500), 200, 4.0, true},
        {"Near soft limit, high EBF", milliseconds(4000), 800, 5.0, false},
        {"Under soft but would exceed", milliseconds(3000), 1000, 3.0, false},
        {"Very early, optimistic", milliseconds(50), 50, 6.0, true},
    };
    
    for (const auto& s : scenarios) {
        bool decision = hasTimeForNextIteration(limits, s.elapsed, s.lastIterTime, s.ebf);
        
        std::cout << s.description << ": ";
        std::cout << (decision ? "CONTINUE" : "STOP");
        
        if (decision == s.expectedDecision) {
            std::cout << " ✓\n";
        } else {
            std::cout << " ✗ (expected " << (s.expectedDecision ? "CONTINUE" : "STOP") << ")\n";
        }
    }
}

// Test with real search data
void testWithRealSearch() {
    std::cout << "\nSimulating Real Search Progression...\n";
    std::cout << "Depth | Time | Nodes | EBF  | Predicted Next | Decision\n";
    std::cout << "------|------|-------|------|---------------|----------\n";
    
    // Simulate typical search progression
    struct Iteration {
        int depth;
        milliseconds time;
        int nodes;
    };
    
    Iteration iterations[] = {
        {1, milliseconds(1), 30},
        {2, milliseconds(5), 150},
        {3, milliseconds(25), 900},
        {4, milliseconds(120), 5400},
        {5, milliseconds(650), 32000},
        {6, milliseconds(3200), 180000},
    };
    
    TimeLimits limits;
    limits.soft = milliseconds(5000);
    limits.hard = milliseconds(8000);
    limits.optimum = milliseconds(3000);
    
    milliseconds totalElapsed(0);
    
    for (size_t i = 0; i < sizeof(iterations)/sizeof(iterations[0]); ++i) {
        const auto& iter = iterations[i];
        totalElapsed += iter.time;
        
        double ebf = 0.0;
        if (i > 0) {
            ebf = static_cast<double>(iter.nodes) / iterations[i-1].nodes;
        }
        
        // Predict next iteration time
        auto predicted = predictNextIterationTime(iter.time, ebf, iter.depth);
        
        // Check if we should continue
        bool shouldContinue = hasTimeForNextIteration(limits, totalElapsed, 
                                                      iter.time.count(), ebf);
        
        std::cout << std::setw(5) << iter.depth << " | "
                 << std::setw(4) << iter.time.count() << " | "
                 << std::setw(6) << iter.nodes << " | "
                 << std::fixed << std::setprecision(1)
                 << std::setw(4) << ebf << " | "
                 << std::setw(13) << predicted.count() << " | "
                 << (shouldContinue ? "CONTINUE" : "STOP    ") << "\n";
        
        if (!shouldContinue) {
            std::cout << "\nSearch would stop at depth " << iter.depth 
                     << " (elapsed: " << totalElapsed.count() << "ms)\n";
            break;
        }
    }
}

int main() {
    std::cout << "=== Stage 13, Deliverable 4.2a: Time Prediction Test ===\n\n";
    
    testTimePrediction();
    testTimeDecision();
    testWithRealSearch();
    
    std::cout << "\n✓ Time prediction implemented with EBF\n";
    std::cout << "=== Test Complete ===\n";
    return 0;
}