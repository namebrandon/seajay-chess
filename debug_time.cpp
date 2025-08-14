// Debug program to check time calculations for 10+0.1 time control
#include <iostream>
#include <chrono>
#include <algorithm>

using namespace std;
using namespace std::chrono;

struct SearchLimits {
    milliseconds time[2] = {milliseconds(0), milliseconds(0)};
    milliseconds inc[2] = {milliseconds(0), milliseconds(0)};
    milliseconds movetime = milliseconds(0);
    int maxDepth = 64;
    bool infinite = false;
};

// Simulate the time calculation logic
milliseconds calculateEnhancedTimeLimit(const SearchLimits& limits, int moveNumber, int color, double stabilityFactor) {
    
    // Fixed move time takes priority
    if (limits.movetime > milliseconds(0)) {
        return limits.movetime;
    }
    
    // Infinite analysis mode
    if (limits.infinite) {
        return milliseconds::max();
    }
    
    auto remaining = limits.time[color];
    auto increment = limits.inc[color];
    
    // If no time specified, use a default
    if (remaining == milliseconds(0)) {
        return milliseconds(5000);
    }
    
    // Estimate moves remaining
    int estimatedMovesRemaining;
    
    if (moveNumber < 15) {
        estimatedMovesRemaining = 40;
    } else if (moveNumber < 40) {
        estimatedMovesRemaining = 35 - (moveNumber - 15) / 2;
    } else {
        estimatedMovesRemaining = max(15, 60 - moveNumber);
    }
    
    // Calculate base time allocation
    auto baseTime = remaining / estimatedMovesRemaining;
    
    // Add increment consideration (80% of increment)
    auto incrementBonus = increment * 4 / 5;
    
    // Apply stability factor
    auto adjustedTime = milliseconds(static_cast<int64_t>(
        (baseTime.count() + incrementBonus.count()) * stabilityFactor
    ));
    
    // Apply safety bounds
    adjustedTime = max(adjustedTime, milliseconds(10));
    
    // Never use more than 30% of remaining time
    auto maxTime = remaining * 3 / 10;
    adjustedTime = min(adjustedTime, maxTime);
    
    // Keep at least 100ms buffer for lag
    if (remaining > milliseconds(200)) {
        adjustedTime = min(adjustedTime, remaining - milliseconds(100));
    }
    
    return adjustedTime;
}

int main() {
    SearchLimits limits;
    
    // Test case: 10+0.1 time control
    // This is 10 seconds + 0.1 second increment
    limits.time[0] = milliseconds(10000);  // 10 seconds for white
    limits.time[1] = milliseconds(10000);  // 10 seconds for black
    limits.inc[0] = milliseconds(100);     // 0.1 second increment
    limits.inc[1] = milliseconds(100);
    
    cout << "Testing 10+0.1 time control (10000ms + 100ms increment)\n";
    cout << "=========================================\n\n";
    
    // Test at different move numbers
    int moveNumbers[] = {1, 10, 20, 30, 40, 50};
    double stabilityFactors[] = {1.0, 0.5, 1.5};
    
    for (int moveNum : moveNumbers) {
        cout << "Move " << moveNum << ":\n";
        for (double stability : stabilityFactors) {
            auto time = calculateEnhancedTimeLimit(limits, moveNum, 0, stability);
            cout << "  Stability " << stability << ": " << time.count() << "ms";
            
            // Calculate soft and hard limits
            auto soft = time;
            auto hard = milliseconds(time.count() * 3);
            auto maxHard = limits.time[0] / 2;
            hard = min(hard, maxHard);
            
            cout << " (soft=" << soft.count() << "ms, hard=" << hard.count() << "ms)\n";
        }
        cout << "\n";
    }
    
    // Special case: very low time remaining
    cout << "Low time remaining tests:\n";
    cout << "========================\n";
    
    limits.time[0] = milliseconds(100);  // Only 100ms remaining
    auto time = calculateEnhancedTimeLimit(limits, 1, 0, 1.0);
    cout << "100ms remaining: allocated=" << time.count() << "ms\n";
    
    limits.time[0] = milliseconds(50);   // Only 50ms remaining  
    time = calculateEnhancedTimeLimit(limits, 1, 0, 1.0);
    cout << "50ms remaining: allocated=" << time.count() << "ms\n";
    
    limits.time[0] = milliseconds(10);   // Only 10ms remaining
    time = calculateEnhancedTimeLimit(limits, 1, 0, 1.0);
    cout << "10ms remaining: allocated=" << time.count() << "ms\n";
    
    return 0;
}