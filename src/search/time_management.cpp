// Stage 13, Deliverable 2.2a: Time management integration prep
// New time management calculation alongside old one

#include "time_management.h"
#include "types.h"  // For SearchLimits
#include "../core/board.h"
#include <algorithm>
#include <cmath>

namespace seajay::search {

// Enhanced time management calculation (NEW)
// This will eventually replace the simple calculation in negamax.cpp
std::chrono::milliseconds calculateEnhancedTimeLimit(const SearchLimits& limits, 
                                 const Board& board,
                                 double stabilityFactor) {
    
    // Fixed move time takes priority
    if (limits.movetime > std::chrono::milliseconds(0)) {
        return limits.movetime;
    }
    
    // Infinite analysis mode
    if (limits.infinite) {
        return std::chrono::milliseconds::max();
    }
    
    // Get side to move's time
    Color stm = board.sideToMove();
    auto remaining = limits.time[stm];
    auto increment = limits.inc[stm];
    
    // If no time specified, use a default
    if (remaining == std::chrono::milliseconds(0)) {
        return std::chrono::milliseconds(5000);  // 5 seconds default
    }
    
    // Enhanced calculation with more factors
    
    // 1. Estimate moves remaining in game
    // Early game: 40+ moves, middle game: 30 moves, endgame: 20 moves
    int moveNumber = board.fullmoveNumber();
    int estimatedMovesRemaining;
    
    if (moveNumber < 15) {
        // Opening phase
        estimatedMovesRemaining = 40;
    } else if (moveNumber < 40) {
        // Middle game
        estimatedMovesRemaining = 35 - (moveNumber - 15) / 2;
    } else {
        // Endgame
        estimatedMovesRemaining = std::max(15, 60 - moveNumber);
    }
    
    // 2. Calculate base time allocation
    // Use a fraction of remaining time based on moves remaining
    auto baseTime = remaining / estimatedMovesRemaining;
    
    // 3. Add increment consideration
    // Use most of the increment (80%) since we get it every move
    auto incrementBonus = increment * 4 / 5;
    
    // 4. Apply stability factor
    // Stable positions need less time, unstable need more
    auto adjustedTime = std::chrono::milliseconds(static_cast<int64_t>(
        (baseTime.count() + incrementBonus.count()) * stabilityFactor
    ));
    
    // 5. Apply safety bounds
    
    // Minimum time to search something
    adjustedTime = std::max(adjustedTime, std::chrono::milliseconds(10));
    
    // Never use more than 30% of remaining time (safety)
    auto maxTime = remaining * 3 / 10;
    adjustedTime = std::min(adjustedTime, maxTime);
    
    // Keep at least 100ms buffer for lag - reverting to Candidate 1
    // The 200ms buffer was too conservative and reduced search quality
    if (remaining > std::chrono::milliseconds(200)) {
        adjustedTime = std::min(adjustedTime, remaining - std::chrono::milliseconds(100));
    }
    
    // 6. Soft and hard limits
    // Soft limit: normal target time
    // Hard limit: maximum we'll ever use (2x soft limit, but capped)
    
    return adjustedTime;
}

// Calculate soft and hard time limits
TimeLimits calculateTimeLimits(const SearchLimits& limits,
                              const Board& board,
                              double stabilityFactor) {
    
    TimeLimits result;
    
    // Calculate the base/optimum time
    result.optimum = calculateEnhancedTimeLimit(limits, board, stabilityFactor);
    
    // Soft limit is the same as optimum (can be exceeded if position is unstable)
    result.soft = result.optimum;
    
    // Hard limit is 3x the optimum, but capped at 50% of remaining time
    result.hard = std::chrono::milliseconds(result.optimum.count() * 3);
    
    // But never more than 50% of remaining time
    Color stm = board.sideToMove();
    auto remaining = limits.time[stm];
    if (remaining > std::chrono::milliseconds(0)) {
        auto maxHard = remaining / 2;
        result.hard = std::min(result.hard, maxHard);
    }
    
    return result;
}

// Check if we should stop searching based on time
bool shouldStopOnTime(const TimeLimits& limits,
                     std::chrono::milliseconds elapsed,
                     int completedDepth,
                     bool positionStable) {
    
    // Never stop if we haven't searched anything
    if (completedDepth < 1) {
        return false;
    }
    
    // Always stop at hard limit
    if (elapsed >= limits.hard) {
        return true;
    }
    
    // For stable positions, stop at soft limit
    if (positionStable && elapsed >= limits.soft) {
        return true;
    }
    
    // For unstable positions, we can exceed soft limit up to hard limit
    // But be more aggressive about stopping as we approach hard limit
    if (!positionStable) {
        // Stop if we've used 80% of hard limit
        if (elapsed >= std::chrono::milliseconds(limits.hard.count() * 4 / 5)) {
            return true;
        }
    }
    
    return false;
}

// Predict if we have time for another iteration
bool hasTimeForNextIteration(const TimeLimits& limits,
                            std::chrono::milliseconds elapsed,
                            double lastIterationTime,
                            double branchingFactor) {
    
    // Estimate time for next iteration
    // Typically each iteration takes branchingFactor times longer
    // Use a conservative estimate
    double estimatedNextTime = lastIterationTime * branchingFactor * 1.5;
    
    // Check if we'd exceed soft limit
    std::chrono::milliseconds projected = elapsed + std::chrono::milliseconds(static_cast<int64_t>(estimatedNextTime));
    
    // For first few iterations, be optimistic
    if (elapsed < std::chrono::milliseconds(100)) {
        return projected < limits.hard;
    }
    
    // Otherwise, don't start if we'd exceed soft limit
    return projected < limits.soft;
}

// Stage 13, Deliverable 4.2a: Predict time for next iteration
std::chrono::milliseconds predictNextIterationTime(
    std::chrono::milliseconds lastIterationTime,
    double effectiveBranchingFactor,
    int currentDepth) {
    
    // Validate inputs
    if (effectiveBranchingFactor <= 0) {
        // No valid EBF, use conservative default
        effectiveBranchingFactor = 5.0;
    }
    
    // Handle very fast iterations (showing as 0ms)
    if (lastIterationTime.count() <= 0) {
        // Use a minimum iteration time of 1ms for prediction
        // This prevents unrealistic predictions when search is very fast
        lastIterationTime = std::chrono::milliseconds(1);
    }
    
    // Use sophisticated EBF if available (should be from getSophisticatedEBF())
    // If EBF is too low or too high, clamp to reasonable bounds
    double clampedEBF = effectiveBranchingFactor;
    if (clampedEBF < 1.5) clampedEBF = 1.5;  // Minimum expected growth
    if (clampedEBF > 10.0) clampedEBF = 10.0;  // Maximum reasonable growth
    
    // Apply depth-based adjustment
    // At deeper depths, EBF tends to stabilize or decrease slightly
    double depthFactor = 1.0;
    if (currentDepth >= 10) {
        depthFactor = 0.9;  // Expect 10% reduction in growth at high depths
    } else if (currentDepth >= 7) {
        depthFactor = 0.95;  // Expect 5% reduction at medium-high depths
    }
    
    // Calculate predicted time
    // Basic formula: next_time = last_time * EBF * depth_adjustment
    double predictedTime = lastIterationTime.count() * clampedEBF * depthFactor;
    
    // Add safety margin (10% extra, was 20% which was too conservative)
    predictedTime *= 1.1;
    
    // Convert to milliseconds, ensuring we don't overflow
    if (predictedTime > 3600000) {  // Cap at 1 hour
        return std::chrono::milliseconds(3600000);
    }
    
    return std::chrono::milliseconds(static_cast<int64_t>(predictedTime));
}

} // namespace seajay::search