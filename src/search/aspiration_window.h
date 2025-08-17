#pragma once

#include "../core/types.h"
#include "../evaluation/types.h"

namespace seajay::search {

using eval::Score;

/**
 * @brief Aspiration window constants based on chess-engine-expert recommendations
 */
struct AspirationConstants {
    // Initial window size in centipawns (Stockfish-proven value)
    static constexpr int INITIAL_DELTA = 16;
    
    // Window growth factor (approximately 1.33x per fail)
    // delta += delta/3
    static constexpr int GROWTH_DIVISOR = 3;
    
    // Maximum number of re-search attempts before infinite window
    static constexpr int MAX_ATTEMPTS = 5;
    
    // Minimum depth to use aspiration windows
    static constexpr int MIN_DEPTH = 4;
    
    // Depth adjustment factor (slightly wider windows at higher depths)
    static constexpr int DEPTH_ADJUSTMENT_FACTOR = 2;
};

/**
 * @brief Aspiration window data for a search iteration
 */
struct AspirationWindow {
    Score alpha{Score::minus_infinity()};
    Score beta{Score::infinity()};
    int delta{AspirationConstants::INITIAL_DELTA};
    int attempts{0};
    bool failedLow{false};
    bool failedHigh{false};
    
    /**
     * @brief Check if window is infinite (no bounds)
     */
    bool isInfinite() const {
        return alpha == Score::minus_infinity() && 
               beta == Score::infinity();
    }
    
    /**
     * @brief Check if we've exceeded maximum attempts
     */
    bool exceedsMaxAttempts() const {
        return attempts >= AspirationConstants::MAX_ATTEMPTS;
    }
    
    /**
     * @brief Reset window to infinite bounds
     */
    void makeInfinite() {
        alpha = Score::minus_infinity();
        beta = Score::infinity();
    }
};

/**
 * @brief Calculate initial aspiration window around previous score
 * 
 * @param previousScore Score from previous iteration
 * @param depth Current search depth
 * @param initialDelta Initial window size in centipawns (default 16)
 * @return Initial aspiration window
 */
AspirationWindow calculateInitialWindow(Score previousScore, int depth, 
                                       int initialDelta = AspirationConstants::INITIAL_DELTA);

/**
 * @brief Widen aspiration window after fail high or fail low
 * 
 * @param window Current window to widen
 * @param score Score that failed high/low
 * @param failedHigh True if failed high, false if failed low
 * @param maxAttempts Maximum attempts before infinite window (default 5)
 * @return Widened aspiration window
 */
AspirationWindow widenWindow(const AspirationWindow& window, 
                             Score score, 
                             bool failedHigh,
                             int maxAttempts = AspirationConstants::MAX_ATTEMPTS);

} // namespace seajay::search