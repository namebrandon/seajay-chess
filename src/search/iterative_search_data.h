#pragma once

// Stage 13: Iterative Deepening - Enhanced search data structure
// Phase 1, Deliverable 1.1b: IterativeSearchData class skeleton
// Phase 2, Deliverable 2.1d: Stability tracking structure

#include "types.h"
#include "iteration_info.h"
#include "time_management.h"
#include <array>
#include <chrono>
#include <cstdlib>  // For std::abs
#include <algorithm>  // For std::min

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
    
    // Override virtual method to identify as IterativeSearchData
    // This replaces expensive dynamic_cast in hot path
    virtual bool isIterativeSearch() const override { return true; }
    
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
    int m_requiredStability{6};          // Iterations needed to consider stable (default 6)
    bool m_positionStable{false};        // Is position considered stable?
    
    // Score stability tracking
    eval::Score m_stableScore{eval::Score::zero()};  // Score when stable
    int m_scoreStabilityCount{0};                    // Iterations with similar score
    eval::Score m_scoreWindow{eval::Score(10)};      // Window for score stability (10 cp)
    
    // UCI info update timing (Phase 1, enhanced in Phase 6)
    std::chrono::steady_clock::time_point m_lastInfoTime;  // Last time info was sent
    uint64_t m_nodesAtLastInfo{0};  // Node count at last info update (Phase 6)
    eval::Score m_scoreAtLastInfo{eval::Score::zero()};  // Score at last info update (Phase 6)
    
    // Phase 6: Adaptive update intervals based on search time
    static constexpr auto INFO_UPDATE_FAST = std::chrono::milliseconds(50);    // First 1 second
    static constexpr auto INFO_UPDATE_MEDIUM = std::chrono::milliseconds(200);  // 1-10 seconds
    static constexpr auto INFO_UPDATE_SLOW = std::chrono::milliseconds(1000);  // > 10 seconds
    static constexpr auto INFO_MIN_NODES = uint64_t(10000);  // Minimum nodes between updates (Phase 6)
    
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
        m_lastInfoTime = std::chrono::steady_clock::now();  // Phase 1: Reset info time
        m_nodesAtLastInfo = 0;  // Phase 6: Reset node counter
        m_scoreAtLastInfo = eval::Score::zero();  // Phase 6: Reset score
        
        // Clear iteration data
        for (auto& iter : m_iterations) {
            iter = IterationInfo{};
        }
    }
    
    // Phase 6: Enhanced check if we should send UCI info update with smart throttling
    bool shouldSendInfo(bool forceOnScoreChange = false) const {
        auto now = std::chrono::steady_clock::now();
        auto timeSinceLastInfo = std::chrono::duration_cast<std::chrono::milliseconds>(now - m_lastInfoTime);
        auto totalElapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - startTime);
        
        // Determine appropriate interval based on total search time
        std::chrono::milliseconds requiredInterval;
        if (totalElapsed < std::chrono::seconds(1)) {
            requiredInterval = INFO_UPDATE_FAST;    // 50ms for first second
        } else if (totalElapsed < std::chrono::seconds(10)) {
            requiredInterval = INFO_UPDATE_MEDIUM;  // 200ms for 1-10 seconds
        } else {
            requiredInterval = INFO_UPDATE_SLOW;    // 1000ms after 10 seconds
        }
        
        // Check time interval
        if (timeSinceLastInfo < requiredInterval) {
            return false;  // Not enough time has passed
        }
        
        // Phase 6: Check minimum node count between updates
        uint64_t nodesSinceLastInfo = nodes - m_nodesAtLastInfo;
        if (nodesSinceLastInfo < INFO_MIN_NODES) {
            return false;  // Not enough nodes searched
        }
        
        // Phase 6: Check for significant score change (optional)
        if (forceOnScoreChange && bestScore != eval::Score::zero()) {
            int32_t scoreDiff = std::abs((bestScore - m_scoreAtLastInfo).to_cp());
            if (scoreDiff >= 50) {  // Force update on 50cp change
                return true;
            }
        }
        
        return true;  // All conditions met
    }
    
    // Phase 6: Enhanced record that info was sent
    void recordInfoSent(eval::Score currentScore) {
        m_lastInfoTime = std::chrono::steady_clock::now();
        m_nodesAtLastInfo = nodes;
        m_scoreAtLastInfo = currentScore;
    }
    
    // Basic methods for iteration tracking (Deliverable 1.1c)
    
    // Set required stability threshold (Stage 13 Remediation)
    void setRequiredStability(int threshold) {
        m_requiredStability = threshold;
    }
    
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
    
    // Update stability based on new iteration (Deliverable 2.1e)
    void updateStability(const IterationInfo& newIteration) {
        // Move stability: Check if best move changed
        if (newIteration.bestMove == m_stableBestMove) {
            // Same move - increment stability counter
            m_stabilityCount++;
            
            // Mark as stable if we've reached required iterations
            if (m_stabilityCount >= m_requiredStability) {
                m_positionStable = true;
            }
        } else {
            // Different move - reset stability
            m_stableBestMove = newIteration.bestMove;
            m_stabilityCount = 1;  // This is the first iteration with new move
            m_positionStable = false;
            
            // Reset score stability too when move changes
            m_stableScore = newIteration.score;
            m_scoreStabilityCount = 1;
        }
        
        // Score stability: Check if score is within window
        // This function is called AFTER recordIteration, so m_iterationCount has been incremented
        if (m_iterationCount > 1) {
            // Compare with the previous iteration (index m_iterationCount - 2)
            const IterationInfo& prevIter = m_iterations[m_iterationCount - 2];
            eval::Score scoreDiff = eval::Score(std::abs(newIteration.score.value() - prevIter.score.value()));
            
            if (scoreDiff <= m_scoreWindow) {
                // Score is stable (within window)
                m_scoreStabilityCount++;
                
                // Update stable score to average of stable scores
                if (m_scoreStabilityCount > 1) {
                    // Simple averaging for stable score
                    int avgValue = (m_stableScore.value() + newIteration.score.value()) / 2;
                    m_stableScore = eval::Score(avgValue);
                }
            } else {
                // Score changed significantly - reset score stability
                m_stableScore = newIteration.score;
                m_scoreStabilityCount = 1;
                
                // If score is unstable, position is not stable
                if (m_positionStable && m_scoreStabilityCount < m_requiredStability) {
                    m_positionStable = false;
                }
            }
        } else {
            // First iteration - initialize
            m_stableScore = newIteration.score;
            m_scoreStabilityCount = 1;
        }
    }
    
    // Check if position is stable (Deliverable 2.1e)
    bool isPositionStable() const {
        // Position is stable if both move and score are stable
        return m_positionStable && (m_scoreStabilityCount >= m_requiredStability);
    }
    
    // Get stability factor for time management (Deliverable 2.1e)
    double getStabilityFactor() const {
        // Return a factor between 0.5 (very stable) and 1.5 (very unstable)
        // Stable positions can use less time, unstable need more
        
        if (isPositionStable()) {
            // Very stable - can save time
            if (m_stabilityCount >= 4 && m_scoreStabilityCount >= 4) {
                return 0.5;  // Use half the time
            }
            // Moderately stable
            if (m_stabilityCount >= 3 && m_scoreStabilityCount >= 3) {
                return 0.7;  // Use 70% of time
            }
            // Just became stable
            return 0.9;  // Use 90% of time
        } else {
            // Unstable - may need more time
            if (m_stabilityCount == 1 && m_iterationCount > 2) {
                // Move just changed after being stable
                return 1.5;  // Use 50% more time
            }
            if (m_scoreStabilityCount == 1 && m_iterationCount > 2) {
                // Score just changed significantly
                return 1.3;  // Use 30% more time
            }
            // Default unstable
            return 1.1;  // Use 10% more time
        }
    }
    
    // Check if we should extend search due to instability (Deliverable 2.1e)
    bool shouldExtendDueToInstability() const {
        // Extend search if position is unstable and we have time
        if (m_iterationCount < 4) {
            // Too early to judge stability
            return false;
        }
        
        // Check various instability indicators
        bool moveUnstable = (m_stabilityCount == 1 && m_iterationCount > 3);
        bool scoreUnstable = (m_scoreStabilityCount == 1 && m_iterationCount > 3);
        bool recentChange = false;
        
        // Check if best move changed in last 2 iterations
        if (m_iterationCount >= 2) {
            const IterationInfo& curr = getLastIteration();
            const IterationInfo& prev = getIteration(m_iterationCount - 2);
            recentChange = (curr.bestMove != prev.bestMove);
        }
        
        // Extend if any strong instability indicator is present
        return moveUnstable || scoreUnstable || recentChange;
    }
    
    // Stage 13, Deliverable 4.1c: Sophisticated EBF with weighted average
    // Uses last 3-4 iterations with more weight on recent data
    double getSophisticatedEBF() const {
        if (m_iterationCount < 2) {
            return 0.0;  // Not enough data
        }
        
        // Determine how many iterations to use (3-4, based on availability)
        size_t windowSize = std::min(size_t(4), m_iterationCount);
        if (windowSize < 2) windowSize = 2;  // Need at least 2
        
        // Calculate weighted average EBF
        // Weights: most recent gets highest weight
        // For 4 iterations: weights are 4, 3, 2, 1 (most recent to oldest)
        // For 3 iterations: weights are 3, 2, 1
        double totalEBF = 0.0;
        double totalWeight = 0.0;
        
        for (size_t i = 1; i < windowSize && i < m_iterationCount; ++i) {
            size_t currIdx = m_iterationCount - i;
            size_t prevIdx = currIdx - 1;
            
            const auto& curr = m_iterations[currIdx];
            const auto& prev = m_iterations[prevIdx];
            
            if (prev.nodes > 0) {
                double ebf = static_cast<double>(curr.nodes) / prev.nodes;
                double weight = windowSize - i + 1;  // Higher weight for more recent
                
                totalEBF += ebf * weight;
                totalWeight += weight;
            }
        }
        
        if (totalWeight > 0) {
            return totalEBF / totalWeight;
        }
        
        // Fall back to simple EBF if no valid data
        if (m_iterationCount >= 2) {
            const auto& curr = m_iterations[m_iterationCount - 1];
            const auto& prev = m_iterations[m_iterationCount - 2];
            if (prev.nodes > 0) {
                return static_cast<double>(curr.nodes) / prev.nodes;
            }
        }
        
        return 0.0;
    }
};

} // namespace seajay::search