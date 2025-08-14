#pragma once

// Stage 13: Iterative Deepening - Time Management
// Phase 2, Deliverable 2.1a: Time management types

#include "../core/types.h"
#include <chrono>
#include <cstdint>

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

} // namespace seajay::search