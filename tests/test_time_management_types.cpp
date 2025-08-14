// Stage 13, Deliverable 2.1a: Test time management types compilation

#include <iostream>
#include <cassert>
#include "../src/search/time_management.h"

using namespace seajay;
using namespace seajay::search;

void testTimeInfoStructure() {
    std::cout << "Testing TimeInfo structure..." << std::endl;
    
    // Create default TimeInfo
    TimeInfo info;
    
    // Check default values
    assert(info.whiteTime == 0);
    assert(info.blackTime == 0);
    assert(info.whiteInc == 0);
    assert(info.blackInc == 0);
    assert(info.moveTime == 0);
    assert(info.movesToGo == 0);
    assert(info.optimumTime == 0);
    assert(info.maximumTime == 0);
    assert(info.softLimit == 0);
    assert(info.hardLimit == 0);
    
    std::cout << "  ✓ Default values correct" << std::endl;
    
    // Test hasTimeControl
    assert(!info.hasTimeControl());
    info.whiteTime = 60000;  // 1 minute
    assert(info.hasTimeControl());
    
    std::cout << "  ✓ hasTimeControl() works" << std::endl;
    
    // Test getTimeForSide
    info.whiteTime = 120000;  // 2 minutes
    info.blackTime = 90000;   // 1.5 minutes
    assert(info.getTimeForSide(WHITE) == 120000);
    assert(info.getTimeForSide(BLACK) == 90000);
    
    std::cout << "  ✓ getTimeForSide() works" << std::endl;
    
    // Test getIncrementForSide
    info.whiteInc = 1000;  // 1 second
    info.blackInc = 2000;  // 2 seconds
    assert(info.getIncrementForSide(WHITE) == 1000);
    assert(info.getIncrementForSide(BLACK) == 2000);
    
    std::cout << "  ✓ getIncrementForSide() works" << std::endl;
    
    std::cout << "  Test passed!" << std::endl;
}

void testTimeConstants() {
    std::cout << "Testing time management constants..." << std::endl;
    
    // Just verify they exist and have reasonable values
    assert(TimeConstants::MIN_TIME_RESERVE > 0);
    assert(TimeConstants::MIN_TIME_RESERVE <= 100);
    
    assert(TimeConstants::MOVES_TO_GO_FACTOR > 0.0);
    assert(TimeConstants::MOVES_TO_GO_FACTOR <= 1.0);
    
    assert(TimeConstants::SUDDEN_DEATH_FACTOR > 0.0);
    assert(TimeConstants::SUDDEN_DEATH_FACTOR <= 0.1);
    
    assert(TimeConstants::INCREMENT_FACTOR > 0.0);
    assert(TimeConstants::INCREMENT_FACTOR <= 1.0);
    
    assert(TimeConstants::STABLE_POSITION_FACTOR > 0.0);
    assert(TimeConstants::STABLE_POSITION_FACTOR <= 1.0);
    
    assert(TimeConstants::UNSTABLE_POSITION_FACTOR >= 1.0);
    assert(TimeConstants::UNSTABLE_POSITION_FACTOR <= 3.0);
    
    assert(TimeConstants::SOFT_LIMIT_RATIO >= 0.5);
    assert(TimeConstants::SOFT_LIMIT_RATIO <= 2.0);
    
    assert(TimeConstants::HARD_LIMIT_RATIO >= 2.0);
    assert(TimeConstants::HARD_LIMIT_RATIO <= 10.0);
    
    assert(TimeConstants::MAX_TIME_FACTOR > 0.0);
    assert(TimeConstants::MAX_TIME_FACTOR <= 0.5);
    
    std::cout << "  ✓ All constants have reasonable values" << std::endl;
    std::cout << "  Test passed!" << std::endl;
}

void testTypeSizes() {
    std::cout << "Testing type sizes..." << std::endl;
    
    // TimeMs should be at least 64-bit to handle long time controls
    assert(sizeof(TimeMs) >= 8);
    std::cout << "  TimeMs size: " << sizeof(TimeMs) << " bytes" << std::endl;
    
    // TimeInfo should be reasonably sized
    assert(sizeof(TimeInfo) <= 256);  // Should not be huge
    std::cout << "  TimeInfo size: " << sizeof(TimeInfo) << " bytes" << std::endl;
    
    std::cout << "  ✓ Type sizes reasonable" << std::endl;
    std::cout << "  Test passed!" << std::endl;
}

int main() {
    std::cout << "\n=== Stage 13, Deliverable 2.1a: Time Management Types Test ===" << std::endl;
    
    try {
        testTimeInfoStructure();
        testTimeConstants();
        testTypeSizes();
        
        std::cout << "\n✓ All tests passed!" << std::endl;
        std::cout << "Deliverable 2.1a COMPLETE: Time management types defined correctly" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Test failed with exception: " << e.what() << std::endl;
        return 1;
    }
}