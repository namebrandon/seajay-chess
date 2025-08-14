#include "aspiration_window.h"
#include <algorithm>

namespace seajay::search {

AspirationWindow calculateInitialWindow(Score previousScore, int depth) {
    AspirationWindow window;
    
    // For depths below MIN_DEPTH, use infinite window
    if (depth < AspirationConstants::MIN_DEPTH) {
        return window; // Already initialized to infinite
    }
    
    // Calculate initial delta based on Stockfish-proven 16 cp value
    // Slightly adjust for depth (wider windows at higher depths)
    int depthAdjustment = depth / AspirationConstants::DEPTH_ADJUSTMENT_FACTOR;
    int delta = AspirationConstants::INITIAL_DELTA + depthAdjustment;
    
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
                             bool failedHigh) {
    // Implementation for next deliverable
    return window;
}

} // namespace seajay::search