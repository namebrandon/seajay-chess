#pragma once

// Stage 13: Iterative Deepening - Iteration tracking data structures
// Phase 1, Deliverable 1.1a: Basic type definitions

#include "../evaluation/types.h"
#include "../core/types.h"
#include <cstdint>
#include <chrono>

namespace seajay::search {

// Forward declarations (POD only, no methods in this deliverable)
struct IterationInfo;

// Time measurement type
using TimeMs = std::chrono::milliseconds::rep;

// IterationInfo: POD struct to track data for each iteration of iterative deepening
// This is filled in during each depth iteration and stored for analysis
struct IterationInfo {
    // Basic search data
    int depth{0};                              // Search depth for this iteration
    eval::Score score{eval::Score::zero()};   // Best score found
    Move bestMove{NO_MOVE};                    // Best move found
    uint64_t nodes{0};                         // Nodes searched in this iteration
    TimeMs elapsed{0};                         // Time spent on this iteration (ms)
    
    // Aspiration window data (for Phase 3)
    eval::Score alpha{eval::Score::minus_infinity()};  // Alpha bound used
    eval::Score beta{eval::Score::infinity()};         // Beta bound used
    int windowAttempts{0};                            // Number of aspiration re-searches
    bool failedHigh{false};                           // Score failed high (beta cutoff)
    bool failedLow{false};                            // Score failed low (below alpha)
    
    // Move stability tracking (for Phase 2)
    bool moveChanged{false};                          // Best move changed from previous iteration
    int moveStability{0};                             // Consecutive iterations with same best move
    
    // Additional statistics (for Phase 4)
    bool firstMoveFailHigh{false};                    // First move caused beta cutoff
    int failHighMoveIndex{-1};                        // Index of move that failed high
    eval::Score secondBestScore{eval::Score::minus_infinity()}; // Score of second best move
    double branchingFactor{0.0};                      // Effective branching factor
};

// Static assertions to ensure POD nature (C++20)
// Note: IterationInfo contains eval::Score which has constructors, 
// so it's not trivial, but it is standard layout
static_assert(std::is_standard_layout_v<IterationInfo>, "IterationInfo must be standard layout");

} // namespace seajay::search