#include "../src/search/aspiration_window.h"
#include "../src/search/aspiration_window.cpp"
#include <iostream>
#include <cassert>

using namespace seajay::search;
using namespace seajay::eval;

void testInfiniteWindowBelowMinDepth() {
    Score previousScore(100);
    
    // Test depths below MIN_DEPTH (4)
    for (int depth = 1; depth < AspirationConstants::MIN_DEPTH; ++depth) {
        AspirationWindow window = calculateInitialWindow(previousScore, depth);
        assert(window.isInfinite());
        assert(window.attempts == 0);
        assert(window.delta == AspirationConstants::INITIAL_DELTA);
    }
    
    std::cout << "✓ Infinite window for depths < " << AspirationConstants::MIN_DEPTH << std::endl;
}

void testInitialWindowAtMinDepth() {
    Score previousScore(100);
    int depth = AspirationConstants::MIN_DEPTH; // 4
    
    AspirationWindow window = calculateInitialWindow(previousScore, depth);
    
    // Expected: 16 cp window + depth adjustment (4/2 = 2) = 18 cp total
    int expectedDelta = AspirationConstants::INITIAL_DELTA + (depth / AspirationConstants::DEPTH_ADJUSTMENT_FACTOR);
    assert(window.delta == expectedDelta);
    
    // Check bounds: previousScore ± expectedDelta
    assert(window.alpha.value() == previousScore.value() - expectedDelta);
    assert(window.beta.value() == previousScore.value() + expectedDelta);
    assert(!window.isInfinite());
    assert(window.attempts == 0);
    assert(!window.failedLow);
    assert(!window.failedHigh);
    
    std::cout << "✓ Initial window at depth " << depth 
              << ": [" << window.alpha.value() << ", " << window.beta.value() << "]"
              << " (delta=" << window.delta << ")" << std::endl;
}

void testWindowWidensWithDepth() {
    Score previousScore(0);
    
    // Test that windows get slightly wider at higher depths
    int prevDelta = 0;
    for (int depth = AspirationConstants::MIN_DEPTH; depth <= 20; depth += 4) {
        AspirationWindow window = calculateInitialWindow(previousScore, depth);
        
        int expectedDelta = AspirationConstants::INITIAL_DELTA + 
                           (depth / AspirationConstants::DEPTH_ADJUSTMENT_FACTOR);
        assert(window.delta == expectedDelta);
        
        // Window should widen or stay same as depth increases
        assert(window.delta >= prevDelta);
        prevDelta = window.delta;
        
        std::cout << "✓ Depth " << depth << " delta: " << window.delta << std::endl;
    }
}

void testBoundsClamping() {
    // Test near maximum score - should clamp beta
    Score highScore(999990);
    AspirationWindow window = calculateInitialWindow(highScore, 10);
    assert(window.beta.value() <= Score::infinity().value());
    assert(window.alpha.value() < highScore.value());
    std::cout << "✓ High score clamping: beta=" << window.beta.value() << std::endl;
    
    // Test near minimum score - should clamp alpha
    Score lowScore(-999990);
    window = calculateInitialWindow(lowScore, 10);
    assert(window.alpha.value() >= Score::minus_infinity().value());
    assert(window.beta.value() > lowScore.value());
    std::cout << "✓ Low score clamping: alpha=" << window.alpha.value() << std::endl;
}

void testTypicalPositions() {
    struct TestCase {
        const char* name;
        Score score;
        int depth;
    };
    
    TestCase cases[] = {
        {"Starting position", Score(30), 8},
        {"Slight advantage", Score(150), 10},
        {"Winning position", Score(500), 12},
        {"Tactical position", Score(-200), 6},
        {"Equal position", Score(0), 14}
    };
    
    for (const auto& tc : cases) {
        AspirationWindow window = calculateInitialWindow(tc.score, tc.depth);
        
        // Verify window is symmetric around score
        int deltaFromAlpha = tc.score.value() - window.alpha.value();
        int deltaToBeta = window.beta.value() - tc.score.value();
        assert(deltaFromAlpha == deltaToBeta || 
               window.alpha == Score::minus_infinity() ||
               window.beta == Score::infinity());
        
        std::cout << "✓ " << tc.name << " (score=" << tc.score.value() 
                  << ", depth=" << tc.depth << "): "
                  << "[" << window.alpha.value() << ", " << window.beta.value() << "]"
                  << std::endl;
    }
}

int main() {
    std::cout << "Testing calculateInitialWindow()..." << std::endl;
    
    testInfiniteWindowBelowMinDepth();
    testInitialWindowAtMinDepth();
    testWindowWidensWithDepth();
    testBoundsClamping();
    testTypicalPositions();
    
    std::cout << "\n✅ All initial window calculation tests passed!" << std::endl;
    
    return 0;
}