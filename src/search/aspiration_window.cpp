#include "aspiration_window.h"
#include <algorithm>

namespace seajay::search {

AspirationWindow calculateInitialWindow(Score previousScore, int depth, int initialDelta) {
    AspirationWindow window;
    
    // For depths below MIN_DEPTH, use infinite window
    if (depth < AspirationConstants::MIN_DEPTH) {
        return window; // Already initialized to infinite
    }
    
    // Calculate initial delta using configurable value
    // Slightly adjust for depth (wider windows at higher depths)
    int depthAdjustment = depth / AspirationConstants::DEPTH_ADJUSTMENT_FACTOR;
    int delta = initialDelta + depthAdjustment;
    
    // Set window bounds around previous score
    // Clamp to prevent overflow
    window.alpha = Score(std::max(previousScore.value() - delta, 
                                  Score::minus_infinity().value()));
    window.beta = Score(std::min(previousScore.value() + delta,
                                 Score::infinity().value()));
    window.delta = delta;
    window.attempts = 0;
    window.failedLow = false;
    window.failedHigh = false;
    
    return window;
}

AspirationWindow widenWindow(const AspirationWindow& window, 
                             Score score, 
                             bool failedHigh,
                             int maxAttempts) {
    AspirationWindow newWindow = window;
    
    // Increment attempt counter
    newWindow.attempts++;
    
    // Check if we've exceeded max attempts - use infinite window
    if (newWindow.attempts >= maxAttempts) {
        newWindow.makeInfinite();
        return newWindow;
    }
    
    // Apply delta growth: delta += delta/3 (approximately 1.33x per fail)
    // This is the chess-engine-expert recommended growth rate
    newWindow.delta += newWindow.delta / AspirationConstants::GROWTH_DIVISOR;
    
    if (failedHigh) {
        // Score exceeded beta - raise beta, keep alpha close
        newWindow.failedHigh = true;
        newWindow.beta = Score(std::min(
            score.value() + newWindow.delta,
            Score::infinity().value()
        ));
        // Keep alpha relatively close to avoid missing good moves
        // This asymmetric adjustment is based on Stockfish's approach
        newWindow.alpha = Score(std::max(
            score.value() - newWindow.delta / 2,
            Score::minus_infinity().value()
        ));
    } else {
        // Score fell below alpha - lower alpha, keep beta close
        newWindow.failedLow = true;
        newWindow.alpha = Score(std::max(
            score.value() - newWindow.delta,
            Score::minus_infinity().value()
        ));
        // Keep beta relatively close to catch improvements
        newWindow.beta = Score(std::min(
            score.value() + newWindow.delta / 2,
            Score::infinity().value()
        ));
    }
    
    return newWindow;
}

} // namespace seajay::search