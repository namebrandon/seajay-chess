#pragma once

// Stage 13: Iterative Deepening - Time Management
// Phase 2, Deliverable 2.1a: Time management types
// Phase 2, Deliverable 2.1b: Basic time calculation
// Phase 2, Deliverable 2.1c: Soft/hard limits

#include "types.h"  // For SearchLimits
#include "../core/types.h"
#include <chrono>
#include <cstdint>
#include <algorithm>

namespace seajay::search {

// Time measurement type (milliseconds)
using TimeMs = std::chrono::milliseconds::rep;

// TimeInfo: Structure containing time control information for a search
struct TimeInfo {
    // Time remaining for each side (milliseconds)
    TimeMs whiteTime{0};     // Time remaining for white
    TimeMs blackTime{0};     // Time remaining for black
    
    // Increment per move (milliseconds)
    TimeMs whiteInc{0};      // Increment for white after each move
    TimeMs blackInc{0};      // Increment for black after each move
    
    // Move-specific time controls
    TimeMs moveTime{0};      // Fixed time for this move (if set)
    int movesToGo{0};        // Moves until next time control (0 = sudden death)
    
    // Calculated time limits
    TimeMs optimumTime{0};   // Optimal time to use for this move
    TimeMs maximumTime{0};   // Maximum time allowed (hard limit)
    TimeMs softLimit{0};     // Soft limit (can be exceeded if position unstable)
    TimeMs hardLimit{0};     // Hard limit (never exceed)
    
    // Helper to check if we have any time control
    bool hasTimeControl() const {
        return whiteTime > 0 || blackTime > 0 || moveTime > 0;
    }
    
    // Get time for side to move
    TimeMs getTimeForSide(Color side) const {
        return (side == WHITE) ? whiteTime : blackTime;
    }
    
    // Get increment for side to move
    TimeMs getIncrementForSide(Color side) const {
        return (side == WHITE) ? whiteInc : blackInc;
    }
};

// Time management constants (can be tuned later)
namespace TimeConstants {
    // Minimum time to reserve (never use all time)
    constexpr TimeMs MIN_TIME_RESERVE = 50;  // 50ms minimum
    
    // Time allocation factors
    constexpr double MOVES_TO_GO_FACTOR = 0.8;   // Use 80% of available when moves-to-go set
    constexpr double SUDDEN_DEATH_FACTOR = 0.04;  // Use 4% per move in sudden death
    constexpr double INCREMENT_FACTOR = 0.75;     // Consider 75% of increment as usable
    
    // Stability factors
    constexpr double STABLE_POSITION_FACTOR = 0.7;    // Use less time if position stable
    constexpr double UNSTABLE_POSITION_FACTOR = 1.5;  // Use more time if position unstable
    
    // Soft/hard limit ratios
    constexpr double SOFT_LIMIT_RATIO = 1.0;   // Soft limit = optimum time
    constexpr double HARD_LIMIT_RATIO = 3.0;   // Hard limit = 3x optimum time
    
    // Maximum time factors
    constexpr double MAX_TIME_FACTOR = 0.25;   // Never use more than 25% of remaining time
}

// Time calculation functions (Deliverable 2.1b)

// Calculate optimum time for a move based on time control
// Simple formula for now - no stability tracking yet
inline TimeMs calculateOptimumTime(const TimeInfo& timeInfo, Color sideToMove) {
    // If fixed move time is set, use it
    if (timeInfo.moveTime > 0) {
        return timeInfo.moveTime - TimeConstants::MIN_TIME_RESERVE;
    }
    
    TimeMs remainingTime = timeInfo.getTimeForSide(sideToMove);
    TimeMs increment = timeInfo.getIncrementForSide(sideToMove);
    
    // If no time control, return 0 (infinite time)
    if (remainingTime <= 0) {
        return 0;
    }
    
    // Reserve minimum time
    remainingTime = std::max(remainingTime - TimeConstants::MIN_TIME_RESERVE, TimeMs(0));
    
    TimeMs optimum = 0;
    
    if (timeInfo.movesToGo > 0) {
        // We have a specific number of moves to make
        // Divide time equally among remaining moves, with a safety factor
        optimum = static_cast<TimeMs>(
            (remainingTime * TimeConstants::MOVES_TO_GO_FACTOR) / timeInfo.movesToGo
        );
    } else {
        // Sudden death time control
        // Assume we need to play ~25 more moves (typical endgame length)
        // Use a small percentage of remaining time per move
        optimum = static_cast<TimeMs>(
            remainingTime * TimeConstants::SUDDEN_DEATH_FACTOR
        );
    }
    
    // Add some of the increment if available
    if (increment > 0) {
        optimum += static_cast<TimeMs>(increment * TimeConstants::INCREMENT_FACTOR);
    }
    
    // Ensure we don't use too much of remaining time
    TimeMs maxAllowed = static_cast<TimeMs>(remainingTime * TimeConstants::MAX_TIME_FACTOR);
    optimum = std::min(optimum, maxAllowed);
    
    // Ensure minimum time
    optimum = std::max(optimum, TimeMs(1));
    
    return optimum;
}

// Calculate soft time limit (can be exceeded if position is unstable)
// Deliverable 2.1c
inline TimeMs calculateSoftLimit(TimeMs optimumTime) {
    // For now, soft limit equals optimum time
    // Can be adjusted based on position stability later
    return static_cast<TimeMs>(optimumTime * TimeConstants::SOFT_LIMIT_RATIO);
}

// Calculate hard time limit (never exceed this)
// Deliverable 2.1c
inline TimeMs calculateHardLimit(TimeMs optimumTime, const TimeInfo& timeInfo, Color sideToMove) {
    // Start with a multiple of optimum time
    TimeMs hardLimit = static_cast<TimeMs>(optimumTime * TimeConstants::HARD_LIMIT_RATIO);
    
    // But never use more than available time minus reserve
    TimeMs remainingTime = timeInfo.getTimeForSide(sideToMove);
    if (remainingTime > 0) {
        TimeMs maxUsable = remainingTime - TimeConstants::MIN_TIME_RESERVE;
        if (maxUsable > 0) {
            hardLimit = std::min(hardLimit, maxUsable);
        } else {
            // Critical time pressure - use what we can
            hardLimit = std::max(TimeMs(1), remainingTime / 2);
        }
    }
    
    // For fixed move time, hard limit should not exceed it
    if (timeInfo.moveTime > 0) {
        hardLimit = std::min(hardLimit, timeInfo.moveTime - TimeMs(10));  // Leave 10ms buffer
    }
    
    // Ensure hard limit is at least as much as soft limit
    TimeMs softLimit = calculateSoftLimit(optimumTime);
    hardLimit = std::max(hardLimit, softLimit);
    
    // Minimum hard limit
    hardLimit = std::max(hardLimit, TimeMs(1));
    
    return hardLimit;
}

// Helper function to calculate all time limits at once
// Deliverable 2.1c
inline void calculateTimeLimits(TimeInfo& timeInfo, Color sideToMove) {
    // Calculate optimum time
    timeInfo.optimumTime = calculateOptimumTime(timeInfo, sideToMove);
    
    // Calculate soft limit (can be exceeded if unstable)
    timeInfo.softLimit = calculateSoftLimit(timeInfo.optimumTime);
    
    // Calculate hard limit (never exceed)
    timeInfo.hardLimit = calculateHardLimit(timeInfo.optimumTime, timeInfo, sideToMove);
    
    // Maximum time is same as hard limit for now
    timeInfo.maximumTime = timeInfo.hardLimit;
}

// Stage 13, Deliverable 2.2a: Enhanced time management
// Structure for time limits
struct TimeLimits {
    std::chrono::milliseconds soft;     // Soft limit (can exceed if unstable)
    std::chrono::milliseconds hard;     // Hard limit (never exceed)
    std::chrono::milliseconds optimum;  // Optimum/target time
};

} // namespace seajay::search

// Forward declarations
namespace seajay {
class Board;
}

namespace seajay::search {

// Enhanced time calculation with stability factor
std::chrono::milliseconds calculateEnhancedTimeLimit(const SearchLimits& limits,
                                                    const seajay::Board& board,
                                                    double stabilityFactor = 1.0);

// Calculate soft and hard time limits
TimeLimits calculateTimeLimits(const SearchLimits& limits,
                              const seajay::Board& board,
                              double stabilityFactor = 1.0);

// Check if we should stop searching based on time
bool shouldStopOnTime(const TimeLimits& limits,
                     std::chrono::milliseconds elapsed,
                     int completedDepth,
                     bool positionStable);

// Predict if we have time for another iteration
bool hasTimeForNextIteration(const TimeLimits& limits,
                            std::chrono::milliseconds elapsed,
                            double lastIterationTime,
                            double branchingFactor);

} // namespace seajay::search