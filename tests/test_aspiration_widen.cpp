#include "../src/search/aspiration_window.h"
#include "../src/search/aspiration_window.cpp"
#include <iostream>
#include <cassert>
#include <iomanip>

using namespace seajay::search;
using namespace seajay::eval;

void testBasicWidening() {
    // Start with a typical window
    Score previousScore(100);
    AspirationWindow window = calculateInitialWindow(previousScore, 8);
    
    std::cout << "Initial window: [" << window.alpha.value() 
              << ", " << window.beta.value() << "] delta=" << window.delta << std::endl;
    
    // Test fail high
    Score failHighScore(window.beta.value() + 10);
    AspirationWindow widened = widenWindow(window, failHighScore, true);
    
    assert(widened.attempts == 1);
    assert(widened.failedHigh == true);
    assert(widened.delta > window.delta);
    assert(widened.beta.value() > window.beta.value());
    std::cout << "✓ After fail high: [" << widened.alpha.value() 
              << ", " << widened.beta.value() << "] delta=" << widened.delta << std::endl;
    
    // Test fail low
    Score failLowScore(window.alpha.value() - 10);
    widened = widenWindow(window, failLowScore, false);
    
    assert(widened.attempts == 1);
    assert(widened.failedLow == true);
    assert(widened.delta > window.delta);
    assert(widened.alpha.value() < window.alpha.value());
    std::cout << "✓ After fail low: [" << widened.alpha.value() 
              << ", " << widened.beta.value() << "] delta=" << widened.delta << std::endl;
}

void testDeltaGrowthRate() {
    AspirationWindow window;
    window.delta = 30; // Start with delta=30
    
    std::cout << "\nTesting delta growth (delta += delta/3):" << std::endl;
    std::cout << "Attempt 0: delta=" << window.delta << std::endl;
    
    for (int i = 1; i <= 4; ++i) {
        int expectedDelta = window.delta + window.delta / AspirationConstants::GROWTH_DIVISOR;
        window = widenWindow(window, Score(1000), true);
        
        std::cout << "Attempt " << i << ": delta=" << window.delta 
                  << " (expected " << expectedDelta << ")" << std::endl;
        assert(window.delta == expectedDelta);
    }
    
    std::cout << "✓ Delta growth rate verified (approximately 1.33x per fail)" << std::endl;
}

void testAsymmetricAdjustment() {
    // Test that fail high/low use asymmetric adjustments
    AspirationWindow window;
    window.alpha = Score(0);
    window.beta = Score(100);
    window.delta = 50;
    
    // Fail high - beta expands more than alpha
    Score failHighScore(150);
    AspirationWindow highWindow = widenWindow(window, failHighScore, true);
    
    int alphaExpansion = window.alpha.value() - highWindow.alpha.value();
    int betaExpansion = highWindow.beta.value() - window.beta.value();
    
    std::cout << "\nFail high asymmetry:" << std::endl;
    std::cout << "  Alpha moved: " << alphaExpansion << " cp" << std::endl;
    std::cout << "  Beta moved: " << betaExpansion << " cp" << std::endl;
    assert(betaExpansion > alphaExpansion); // Beta should move more
    std::cout << "✓ Beta expands more on fail high" << std::endl;
    
    // Fail low - alpha expands more than beta
    Score failLowScore(-50);
    AspirationWindow lowWindow = widenWindow(window, failLowScore, false);
    
    alphaExpansion = window.alpha.value() - lowWindow.alpha.value();
    betaExpansion = lowWindow.beta.value() - window.beta.value();
    
    std::cout << "\nFail low asymmetry:" << std::endl;
    std::cout << "  Alpha moved: " << alphaExpansion << " cp" << std::endl;
    std::cout << "  Beta moved: " << betaExpansion << " cp" << std::endl;
    assert(alphaExpansion > betaExpansion); // Alpha should move more
    std::cout << "✓ Alpha expands more on fail low" << std::endl;
}

void testMaxAttemptsLimit() {
    AspirationWindow window;
    window.delta = 16;
    window.alpha = Score(0);
    window.beta = Score(32);
    
    std::cout << "\nTesting max attempts limit:" << std::endl;
    
    // Simulate multiple re-searches
    for (int i = 1; i <= AspirationConstants::MAX_ATTEMPTS + 1; ++i) {
        window = widenWindow(window, Score(1000), true);
        std::cout << "Attempt " << i << ": ";
        
        if (i < AspirationConstants::MAX_ATTEMPTS) {
            assert(!window.isInfinite());
            std::cout << "[" << window.alpha.value() << ", " 
                     << window.beta.value() << "]" << std::endl;
        } else {
            assert(window.isInfinite());
            std::cout << "INFINITE WINDOW" << std::endl;
        }
    }
    
    std::cout << "✓ Window becomes infinite after " 
              << AspirationConstants::MAX_ATTEMPTS << " attempts" << std::endl;
}

void testBoundsClamping() {
    // Test that bounds are properly clamped to prevent overflow
    
    // Near maximum score
    AspirationWindow window;
    window.alpha = Score(999900);
    window.beta = Score(999950);
    window.delta = 100;
    
    AspirationWindow widened = widenWindow(window, Score(999960), true);
    assert(widened.beta.value() <= Score::infinity().value());
    std::cout << "✓ Beta clamped at maximum: " << widened.beta.value() << std::endl;
    
    // Near minimum score
    window.alpha = Score(-999950);
    window.beta = Score(-999900);
    window.delta = 100;
    
    widened = widenWindow(window, Score(-999960), false);
    assert(widened.alpha.value() >= Score::minus_infinity().value());
    std::cout << "✓ Alpha clamped at minimum: " << widened.alpha.value() << std::endl;
}

void testWideningSequence() {
    // Simulate a typical re-search sequence
    Score initialScore(150);
    AspirationWindow window = calculateInitialWindow(initialScore, 10);
    
    std::cout << "\nSimulating typical re-search sequence:" << std::endl;
    std::cout << std::setw(10) << "Attempt" 
              << std::setw(15) << "Alpha" 
              << std::setw(15) << "Beta" 
              << std::setw(10) << "Delta" 
              << std::setw(15) << "Status" << std::endl;
    std::cout << std::string(65, '-') << std::endl;
    
    auto printWindow = [](int attempt, const AspirationWindow& w, const char* status) {
        std::cout << std::setw(10) << attempt
                  << std::setw(15) << (w.alpha == Score::minus_infinity() ? "−∞" : 
                                       std::to_string(w.alpha.value()))
                  << std::setw(15) << (w.beta == Score::infinity() ? "+∞" : 
                                       std::to_string(w.beta.value()))
                  << std::setw(10) << w.delta
                  << std::setw(15) << status << std::endl;
    };
    
    printWindow(0, window, "Initial");
    
    // Simulate: fail high, fail low, fail high, fail high, max attempts
    struct ReSearch {
        Score score;
        bool failHigh;
        const char* status;
    };
    
    ReSearch sequence[] = {
        {Score(window.beta.value() + 5), true, "Fail high"},
        {Score(window.alpha.value() - 5), false, "Fail low"},
        {Score(300), true, "Fail high"},
        {Score(350), true, "Fail high"},
        {Score(400), true, "Max attempts"}
    };
    
    for (const auto& rs : sequence) {
        window = widenWindow(window, rs.score, rs.failHigh);
        printWindow(window.attempts, window, rs.status);
    }
    
    assert(window.isInfinite());
    std::cout << "✓ Window widening sequence completed successfully" << std::endl;
}

int main() {
    std::cout << "Testing widenWindow()..." << std::endl;
    
    testBasicWidening();
    testDeltaGrowthRate();
    testAsymmetricAdjustment();
    testMaxAttemptsLimit();
    testBoundsClamping();
    testWideningSequence();
    
    std::cout << "\n✅ All window widening tests passed!" << std::endl;
    
    return 0;
}