// Stage 13, Deliverable 2.1c: Test soft/hard time limits

#include <iostream>
#include <cassert>
#include <cmath>
#include "../src/search/time_management.h"

using namespace seajay;
using namespace seajay::search;

void testSoftLimitCalculation() {
    std::cout << "Testing soft limit calculation..." << std::endl;
    
    // Test various optimum times
    TimeMs optimum1 = 1000;
    TimeMs soft1 = calculateSoftLimit(optimum1);
    assert(soft1 == static_cast<TimeMs>(1000 * TimeConstants::SOFT_LIMIT_RATIO));
    std::cout << "  Optimum 1000ms -> soft " << soft1 << "ms" << std::endl;
    
    TimeMs optimum2 = 5000;
    TimeMs soft2 = calculateSoftLimit(optimum2);
    assert(soft2 == static_cast<TimeMs>(5000 * TimeConstants::SOFT_LIMIT_RATIO));
    std::cout << "  Optimum 5000ms -> soft " << soft2 << "ms" << std::endl;
    
    std::cout << "  ✓ Soft limit calculation correct" << std::endl;
}

void testHardLimitNormalTime() {
    std::cout << "Testing hard limit with normal time..." << std::endl;
    
    TimeInfo info;
    info.whiteTime = 60000;  // 1 minute
    
    TimeMs optimum = 2000;  // 2 seconds optimum
    TimeMs hard = calculateHardLimit(optimum, info, WHITE);
    
    // Should be 3x optimum (6 seconds) since we have plenty of time
    TimeMs expected = static_cast<TimeMs>(2000 * TimeConstants::HARD_LIMIT_RATIO);
    assert(hard == expected);
    
    std::cout << "  60s available, 2s optimum -> hard " << hard << "ms" << std::endl;
    std::cout << "  ✓ Normal time hard limit correct" << std::endl;
}

void testHardLimitLowTime() {
    std::cout << "Testing hard limit with low time..." << std::endl;
    
    TimeInfo info;
    info.blackTime = 500;  // Only 500ms left!
    
    TimeMs optimum = 200;  // Would want 200ms
    TimeMs hard = calculateHardLimit(optimum, info, BLACK);
    
    // Should be capped by available time minus reserve
    TimeMs maxUsable = 500 - TimeConstants::MIN_TIME_RESERVE;
    assert(hard == maxUsable);
    
    std::cout << "  500ms available, 200ms optimum -> hard " << hard << "ms (capped)" << std::endl;
    std::cout << "  ✓ Low time hard limit correct" << std::endl;
}

void testHardLimitCriticalTime() {
    std::cout << "Testing hard limit with critical time..." << std::endl;
    
    TimeInfo info;
    info.whiteTime = 30;  // Only 30ms left! Less than reserve
    
    TimeMs optimum = 10;
    TimeMs hard = calculateHardLimit(optimum, info, WHITE);
    
    // In critical situation, use half of remaining
    assert(hard == 15);  // 30/2 = 15
    
    std::cout << "  30ms critical -> hard " << hard << "ms (half remaining)" << std::endl;
    std::cout << "  ✓ Critical time hard limit correct" << std::endl;
}

void testHardLimitFixedTime() {
    std::cout << "Testing hard limit with fixed move time..." << std::endl;
    
    TimeInfo info;
    info.moveTime = 1000;  // 1 second fixed
    info.whiteTime = 60000;  // Also have clock time
    
    TimeMs optimum = 950;  // calculateOptimumTime would return this
    TimeMs hard = calculateHardLimit(optimum, info, WHITE);
    
    // Should be capped by moveTime - 10ms buffer
    assert(hard == 990);
    
    std::cout << "  1s fixed time -> hard " << hard << "ms (fixed - buffer)" << std::endl;
    std::cout << "  ✓ Fixed time hard limit correct" << std::endl;
}

void testHardLimitMinimum() {
    std::cout << "Testing hard limit minimum..." << std::endl;
    
    TimeInfo info;
    info.blackTime = 100;
    
    TimeMs optimum = 1;  // Very small optimum
    TimeMs hard = calculateHardLimit(optimum, info, BLACK);
    
    // Hard limit should be at least soft limit
    TimeMs soft = calculateSoftLimit(optimum);
    assert(hard >= soft);
    assert(hard >= 1);
    
    std::cout << "  1ms optimum -> hard " << hard << "ms (>= soft)" << std::endl;
    std::cout << "  ✓ Hard limit minimum correct" << std::endl;
}

void testCalculateAllLimits() {
    std::cout << "Testing calculate all limits..." << std::endl;
    
    TimeInfo info;
    info.whiteTime = 180000;  // 3 minutes
    info.whiteInc = 2000;     // 2 second increment
    info.movesToGo = 0;        // Sudden death
    
    calculateTimeLimits(info, WHITE);
    
    // Check all limits were set
    assert(info.optimumTime > 0);
    assert(info.softLimit > 0);
    assert(info.hardLimit > 0);
    assert(info.maximumTime == info.hardLimit);
    
    // Check relationships
    assert(info.softLimit <= info.hardLimit);
    assert(info.optimumTime <= info.softLimit);  // With current ratio of 1.0
    
    std::cout << "  3min+2s: optimum=" << info.optimumTime 
              << "ms, soft=" << info.softLimit
              << "ms, hard=" << info.hardLimit << "ms" << std::endl;
    std::cout << "  ✓ All limits calculated correctly" << std::endl;
}

void testEdgeCases() {
    std::cout << "Testing edge cases..." << std::endl;
    
    // Edge case 1: Zero time
    {
        TimeInfo info;
        info.whiteTime = 0;
        
        calculateTimeLimits(info, WHITE);
        
        // Should handle gracefully
        assert(info.optimumTime == 0);  // Infinite time
        assert(info.softLimit == 0);
        assert(info.hardLimit >= 0);
        
        std::cout << "  Zero time handled gracefully" << std::endl;
    }
    
    // Edge case 2: Very high time
    {
        TimeInfo info;
        info.blackTime = 36000000;  // 10 hours!
        info.movesToGo = 1;
        
        calculateTimeLimits(info, BLACK);
        
        // Should be capped by MAX_TIME_FACTOR
        TimeMs available = 36000000 - TimeConstants::MIN_TIME_RESERVE;
        TimeMs maxAllowed = static_cast<TimeMs>(available * TimeConstants::MAX_TIME_FACTOR);
        assert(info.optimumTime == maxAllowed);
        
        std::cout << "  Very high time capped correctly" << std::endl;
    }
    
    // Edge case 3: Negative increment (shouldn't happen but be safe)
    {
        TimeInfo info;
        info.whiteTime = 10000;
        info.whiteInc = -1000;  // Invalid but test robustness
        
        calculateTimeLimits(info, WHITE);
        
        // Should still work (increment treated as 0)
        assert(info.optimumTime > 0);
        assert(info.hardLimit > 0);
        
        std::cout << "  Negative increment handled safely" << std::endl;
    }
    
    std::cout << "  ✓ All edge cases handled correctly" << std::endl;
}

void testTimeScenarios() {
    std::cout << "Testing realistic time scenarios..." << std::endl;
    
    // Scenario 1: Bullet 1+0
    {
        TimeInfo info;
        info.whiteTime = 60000;  // 1 minute
        info.whiteInc = 0;
        info.movesToGo = 0;
        
        calculateTimeLimits(info, WHITE);
        
        std::cout << "  Bullet 1+0: opt=" << info.optimumTime 
                  << ", soft=" << info.softLimit
                  << ", hard=" << info.hardLimit << "ms" << std::endl;
        
        // Should be aggressive but safe
        assert(info.optimumTime >= 2000 && info.optimumTime <= 3000);
        assert(info.hardLimit <= 15000);  // Never more than 25% of time
    }
    
    // Scenario 2: Blitz 5+3
    {
        TimeInfo info;
        info.blackTime = 300000;  // 5 minutes
        info.blackInc = 3000;     // 3 seconds
        info.movesToGo = 0;
        
        calculateTimeLimits(info, BLACK);
        
        std::cout << "  Blitz 5+3: opt=" << info.optimumTime
                  << ", soft=" << info.softLimit  
                  << ", hard=" << info.hardLimit << "ms" << std::endl;
        
        // Should use increment well
        assert(info.optimumTime >= 10000 && info.optimumTime <= 15000);
        assert(info.hardLimit >= info.optimumTime * 2);  // Plenty of buffer
    }
    
    // Scenario 3: Time pressure
    {
        TimeInfo info;
        info.whiteTime = 5000;  // 5 seconds left
        info.whiteInc = 0;
        info.movesToGo = 10;     // 10 moves to control
        
        calculateTimeLimits(info, WHITE);
        
        std::cout << "  Time pressure (5s/10moves): opt=" << info.optimumTime
                  << ", soft=" << info.softLimit
                  << ", hard=" << info.hardLimit << "ms" << std::endl;
        
        // Should be very conservative
        assert(info.optimumTime <= 500);  // ~400ms per move
        assert(info.hardLimit <= 1250);   // Max 25% of remaining
    }
    
    std::cout << "  ✓ All scenarios produce reasonable limits" << std::endl;
}

int main() {
    std::cout << "\n=== Stage 13, Deliverable 2.1c: Soft/Hard Limits Test ===" << std::endl;
    
    try {
        testSoftLimitCalculation();
        testHardLimitNormalTime();
        testHardLimitLowTime();
        testHardLimitCriticalTime();
        testHardLimitFixedTime();
        testHardLimitMinimum();
        testCalculateAllLimits();
        testEdgeCases();
        testTimeScenarios();
        
        std::cout << "\n✓ All tests passed!" << std::endl;
        std::cout << "Deliverable 2.1c COMPLETE: Soft/hard limits implemented correctly" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Test failed with exception: " << e.what() << std::endl;
        return 1;
    }
}