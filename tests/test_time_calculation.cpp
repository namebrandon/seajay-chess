// Stage 13, Deliverable 2.1b: Test basic time calculation

#include <iostream>
#include <cassert>
#include <cmath>
#include "../src/search/time_management.h"

using namespace seajay;
using namespace seajay::search;

void testFixedMoveTime() {
    std::cout << "Testing fixed move time..." << std::endl;
    
    TimeInfo info;
    info.moveTime = 1000;  // 1 second fixed time
    
    TimeMs optimum = calculateOptimumTime(info, WHITE);
    
    // Should be moveTime minus reserve
    TimeMs expected = 1000 - TimeConstants::MIN_TIME_RESERVE;
    assert(optimum == expected);
    
    std::cout << "  Fixed time 1000ms -> optimum " << optimum << "ms" << std::endl;
    std::cout << "  ✓ Fixed move time calculation correct" << std::endl;
}

void testMovesToGo() {
    std::cout << "Testing moves-to-go time control..." << std::endl;
    
    TimeInfo info;
    info.whiteTime = 60000;  // 1 minute
    info.movesToGo = 40;      // 40 moves to time control
    
    TimeMs optimum = calculateOptimumTime(info, WHITE);
    
    // Should be roughly (60000 - reserve) * 0.8 / 40
    TimeMs available = 60000 - TimeConstants::MIN_TIME_RESERVE;
    TimeMs expected = static_cast<TimeMs>(available * TimeConstants::MOVES_TO_GO_FACTOR / 40);
    
    // Allow small difference due to rounding
    assert(std::abs(optimum - expected) <= 1);
    
    std::cout << "  60s for 40 moves -> optimum " << optimum << "ms per move" << std::endl;
    std::cout << "  ✓ Moves-to-go calculation correct" << std::endl;
}

void testSuddenDeath() {
    std::cout << "Testing sudden death time control..." << std::endl;
    
    TimeInfo info;
    info.blackTime = 120000;  // 2 minutes
    info.movesToGo = 0;        // Sudden death
    
    TimeMs optimum = calculateOptimumTime(info, BLACK);
    
    // Should be roughly 4% of remaining time
    TimeMs available = 120000 - TimeConstants::MIN_TIME_RESERVE;
    TimeMs expected = static_cast<TimeMs>(available * TimeConstants::SUDDEN_DEATH_FACTOR);
    
    assert(std::abs(optimum - expected) <= 1);
    
    std::cout << "  120s sudden death -> optimum " << optimum << "ms per move" << std::endl;
    std::cout << "  ✓ Sudden death calculation correct" << std::endl;
}

void testWithIncrement() {
    std::cout << "Testing time with increment..." << std::endl;
    
    TimeInfo info;
    info.whiteTime = 30000;   // 30 seconds
    info.whiteInc = 1000;     // 1 second increment
    info.movesToGo = 0;        // Sudden death
    
    TimeMs optimum = calculateOptimumTime(info, WHITE);
    
    // Should include base time plus part of increment
    TimeMs available = 30000 - TimeConstants::MIN_TIME_RESERVE;
    TimeMs baseTime = static_cast<TimeMs>(available * TimeConstants::SUDDEN_DEATH_FACTOR);
    TimeMs incrementBonus = static_cast<TimeMs>(1000 * TimeConstants::INCREMENT_FACTOR);
    TimeMs expected = baseTime + incrementBonus;
    
    // But capped by MAX_TIME_FACTOR
    TimeMs maxAllowed = static_cast<TimeMs>(available * TimeConstants::MAX_TIME_FACTOR);
    expected = std::min(expected, maxAllowed);
    
    assert(std::abs(optimum - expected) <= 1);
    
    std::cout << "  30s + 1s inc -> optimum " << optimum << "ms" << std::endl;
    std::cout << "  ✓ Increment calculation correct" << std::endl;
}

void testLowTime() {
    std::cout << "Testing low time situation..." << std::endl;
    
    TimeInfo info;
    info.whiteTime = 100;  // Only 100ms left!
    info.movesToGo = 0;
    
    TimeMs optimum = calculateOptimumTime(info, WHITE);
    
    // Should reserve minimum time but still return something
    assert(optimum >= 1);
    assert(optimum <= 100 - TimeConstants::MIN_TIME_RESERVE);
    
    std::cout << "  100ms remaining -> optimum " << optimum << "ms" << std::endl;
    std::cout << "  ✓ Low time handled correctly" << std::endl;
}

void testNoTimeControl() {
    std::cout << "Testing no time control (infinite time)..." << std::endl;
    
    TimeInfo info;
    // All times are 0 (default)
    
    TimeMs optimum = calculateOptimumTime(info, WHITE);
    
    // Should return 0 for infinite time
    assert(optimum == 0);
    
    std::cout << "  No time control -> optimum 0 (infinite)" << std::endl;
    std::cout << "  ✓ Infinite time handled correctly" << std::endl;
}

void testMaxTimeCap() {
    std::cout << "Testing maximum time cap..." << std::endl;
    
    TimeInfo info;
    info.blackTime = 10000;  // 10 seconds
    info.movesToGo = 1;      // Only 1 move to go (would use all time)
    
    TimeMs optimum = calculateOptimumTime(info, BLACK);
    
    // Should be capped by MAX_TIME_FACTOR (25% of remaining)
    TimeMs available = 10000 - TimeConstants::MIN_TIME_RESERVE;
    TimeMs maxAllowed = static_cast<TimeMs>(available * TimeConstants::MAX_TIME_FACTOR);
    
    assert(optimum == maxAllowed);
    
    std::cout << "  10s for 1 move -> capped at " << optimum << "ms (25% max)" << std::endl;
    std::cout << "  ✓ Maximum time cap working" << std::endl;
}

void testKnownScenarios() {
    std::cout << "Testing known time control scenarios..." << std::endl;
    
    // Scenario 1: Blitz 3+0
    {
        TimeInfo info;
        info.whiteTime = 180000;  // 3 minutes
        info.whiteInc = 0;
        info.movesToGo = 0;
        
        TimeMs optimum = calculateOptimumTime(info, WHITE);
        
        // Should be around 7 seconds (4% of 180s)
        assert(optimum >= 7000 && optimum <= 7500);
        std::cout << "  Blitz 3+0 start: " << optimum << "ms per move" << std::endl;
    }
    
    // Scenario 2: Rapid 10+5
    {
        TimeInfo info;
        info.blackTime = 600000;  // 10 minutes
        info.blackInc = 5000;     // 5 seconds
        info.movesToGo = 0;
        
        TimeMs optimum = calculateOptimumTime(info, BLACK);
        
        // Should be base (4% of 600s = 24s) plus increment bonus (75% of 5s = 3.75s)
        // But might be capped by 25% rule
        assert(optimum >= 20000 && optimum <= 30000);
        std::cout << "  Rapid 10+5 start: " << optimum << "ms per move" << std::endl;
    }
    
    // Scenario 3: Tournament 40/90
    {
        TimeInfo info;
        info.whiteTime = 5400000;  // 90 minutes
        info.movesToGo = 40;        // 40 moves
        
        TimeMs optimum = calculateOptimumTime(info, WHITE);
        
        // Should be roughly 90min * 0.8 / 40 = 108 seconds per move
        assert(optimum >= 100000 && optimum <= 110000);
        std::cout << "  Tournament 40/90: " << optimum << "ms per move" << std::endl;
    }
    
    std::cout << "  ✓ All scenarios produce reasonable times" << std::endl;
}

int main() {
    std::cout << "\n=== Stage 13, Deliverable 2.1b: Basic Time Calculation Test ===" << std::endl;
    
    try {
        testFixedMoveTime();
        testMovesToGo();
        testSuddenDeath();
        testWithIncrement();
        testLowTime();
        testNoTimeControl();
        testMaxTimeCap();
        testKnownScenarios();
        
        std::cout << "\n✓ All tests passed!" << std::endl;
        std::cout << "Deliverable 2.1b COMPLETE: Basic time calculation implemented correctly" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Test failed with exception: " << e.what() << std::endl;
        return 1;
    }
}