#pragma once

// Stage 13: Iterative Deepening - Enhanced search data structure
// Phase 1, Deliverable 1.1b: IterativeSearchData class skeleton
// Phase 2, Deliverable 2.1d: Stability tracking structure

#include "types.h"
#include "iteration_info.h"
#include "time_management.h"
#include <array>
#include <chrono>

namespace seajay::search {

// Enhanced search data that tracks iteration history for iterative deepening
// This extends SearchData with iteration-specific tracking
class IterativeSearchData : public SearchData {
public:
    // Maximum depth we'll ever search (reasonable limit)
    static constexpr size_t MAX_ITERATIONS = 64;
    
    // Constructor - initialize base class
    IterativeSearchData() : SearchData() {
        m_iterationCount = 0;
        // Initialize iteration array (already zero-initialized by default)
    }
    
    // Destructor
    ~IterativeSearchData() = default;
    
    // Public data members (skeleton only - no logic yet)
    std::array<IterationInfo, MAX_ITERATIONS> m_iterations{};  // Iteration history
    size_t m_iterationCount{0};                                // Number of completed iterations
    
    // Time management fields (for Phase 2)
    TimeMs m_softLimit{0};      // Soft time limit (can be exceeded if unstable)
    TimeMs m_hardLimit{0};      // Hard time limit (never exceed)
    TimeMs m_optimumTime{0};    // Optimal time to use for this move
    
    // Move stability tracking (Phase 2, Deliverable 2.1d)
    Move m_stableBestMove{NO_MOVE};     // Best move that has been stable
    int m_stabilityCount{0};             // How many iterations with same best move
    int m_requiredStability{2};          // Iterations needed to consider stable
    bool m_positionStable{false};        // Is position considered stable?
    
    // Score stability tracking
    eval::Score m_stableScore{eval::Score::zero()};  // Score when stable
    int m_scoreStabilityCount{0};                    // Iterations with similar score
    eval::Score m_scoreWindow{eval::Score(10)};      // Window for score stability (10 cp)
    
    // Reset for new search
    void reset() {
        SearchData::reset();  // Call base class reset
        m_iterationCount = 0;
        m_softLimit = 0;
        m_hardLimit = 0;
        m_optimumTime = 0;
        m_stableBestMove = NO_MOVE;
        m_stabilityCount = 0;
        m_requiredStability = 2;
        m_positionStable = false;
        m_stableScore = eval::Score::zero();
        m_scoreStabilityCount = 0;
        m_scoreWindow = eval::Score(10);
        
        // Clear iteration data
        for (auto& iter : m_iterations) {
            iter = IterationInfo{};
        }
    }
    
    // Basic methods for iteration tracking (Deliverable 1.1c)
    
    // Record data from a completed iteration
    void recordIteration(const IterationInfo& info) {
        if (m_iterationCount < MAX_ITERATIONS) {
            m_iterations[m_iterationCount] = info;
            m_iterationCount++;
        }
    }
    
    // Get the last completed iteration (or empty if none)
    const IterationInfo& getLastIteration() const {
        static const IterationInfo empty{};
        if (m_iterationCount > 0) {
            return m_iterations[m_iterationCount - 1];
        }
        return empty;
    }
    
    // Get iteration at specific index
    const IterationInfo& getIteration(size_t index) const {
        static const IterationInfo empty{};
        if (index < m_iterationCount) {
            return m_iterations[index];
        }
        return empty;
    }
    
    // Check if we have any completed iterations
    bool hasIterations() const {
        return m_iterationCount > 0;
    }
    
    // Get count of completed iterations
    size_t getIterationCount() const {
        return m_iterationCount;
    }
    
    // Stability tracking methods (Deliverable 2.1d - no logic yet)
    
    // Update stability based on new iteration (stub for now)
    void updateStability(const IterationInfo& newIteration) {
        // Logic will be implemented in Deliverable 2.1e
        // For now, just a stub that does nothing
        (void)newIteration;  // Suppress unused parameter warning
    }
    
    // Check if position is stable (stub for now)
    bool isPositionStable() const {
        // Logic will be implemented in Deliverable 2.1e
        return m_positionStable;
    }
    
    // Get stability factor for time management (stub for now)
    double getStabilityFactor() const {
        // Logic will be implemented in Deliverable 2.1e
        // Returns 1.0 (neutral) for now
        return 1.0;
    }
    
    // Check if we should extend search due to instability (stub for now)
    bool shouldExtendDueToInstability() const {
        // Logic will be implemented in Deliverable 2.1e
        return false;
    }
};

} // namespace seajay::search