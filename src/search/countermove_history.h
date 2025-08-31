#pragma once

#include "../core/types.h"
#include <cstdint>
#include <cstring>
#include <algorithm>

namespace seajay::search {

/**
 * Counter-Move History Heuristic (Optimized Version)
 * 
 * Tracks historical success of move sequences (prevMove -> currentMove).
 * Uses a 3D table indexed by [prevTo][from][to] for memory efficiency.
 * 
 * Memory usage: 64 * 64 * 64 * sizeof(int16_t) = 512 KB per thread
 * 
 * Features:
 * - Local exponential decay instead of global aging
 * - Saturating arithmetic to prevent overflow
 * - Aligned scoring with HistoryHeuristic
 * - Thread-safe (each thread has its own instance)
 */
class CounterMoveHistory {
public:
    // Maximum history value (same as HistoryHeuristic for consistency)
    static constexpr int16_t HISTORY_MAX = 8192;
    
    // Maximum bonus/penalty per update (aligned with HistoryHeuristic)
    static constexpr int MAX_BONUS = 800;
    static constexpr int MAX_PENALTY = 400;
    
    // Decay shift (entry >> 6 = ~1.6% decay per update)
    static constexpr int DECAY_SHIFT = 6;
    
    /**
     * Constructor - initializes empty history table
     */
    CounterMoveHistory() { 
        clear(); 
    }
    
    /**
     * Clear all history values to zero
     */
    void clear() {
        std::memset(m_history, 0, sizeof(m_history));
    }
    
    /**
     * Update history score for a move pair that caused a beta cutoff
     * @param prevMove The previous move played
     * @param move The current move that caused the cutoff
     * @param depth Current search depth (used for bonus calculation)
     */
    void update(Move prevMove, Move move, int depth) {
        if (prevMove == NO_MOVE || move == NO_MOVE) {
            return;
        }
        
        // Don't track captures or promotions
        if (isCapture(move) || isPromotion(move)) {
            return;
        }
        
        const Square prevTo = moveTo(prevMove);
        const Square from = moveFrom(move);
        const Square to = moveTo(move);
        
        // Validate squares
        if (prevTo >= 64 || from >= 64 || to >= 64) {
            return;
        }
        
        // Calculate bonus (capped for consistency with HistoryHeuristic)
        const int bonus = std::min(depth * depth * 2, MAX_BONUS);
        
        int16_t& entry = m_history[prevTo][from][to];
        
        // Apply local decay before update (prevents saturation buildup)
        entry -= entry >> DECAY_SHIFT;
        
        // Saturating add with clamping
        const int newValue = static_cast<int>(entry) + bonus;
        entry = static_cast<int16_t>(std::clamp(newValue, 
                                                 -static_cast<int>(HISTORY_MAX), 
                                                 static_cast<int>(HISTORY_MAX)));
    }
    
    /**
     * Reduce history score for a move pair that was tried but didn't cause cutoff
     * @param prevMove The previous move played
     * @param move The current move that failed
     * @param depth Current search depth (used for penalty calculation)
     */
    void updateFailed(Move prevMove, Move move, int depth) {
        if (prevMove == NO_MOVE || move == NO_MOVE) {
            return;
        }
        
        // Don't track captures or promotions
        if (isCapture(move) || isPromotion(move)) {
            return;
        }
        
        const Square prevTo = moveTo(prevMove);
        const Square from = moveFrom(move);
        const Square to = moveTo(move);
        
        // Validate squares
        if (prevTo >= 64 || from >= 64 || to >= 64) {
            return;
        }
        
        // Calculate penalty (capped and smaller than bonus)
        const int penalty = std::min(depth * depth, MAX_PENALTY);
        
        int16_t& entry = m_history[prevTo][from][to];
        
        // Apply local decay before update
        entry -= entry >> DECAY_SHIFT;
        
        // Saturating subtract with clamping
        const int newValue = static_cast<int>(entry) - penalty;
        entry = static_cast<int16_t>(std::clamp(newValue, 
                                                 -static_cast<int>(HISTORY_MAX), 
                                                 static_cast<int>(HISTORY_MAX)));
    }
    
    /**
     * Get the history score for a move pair
     * @param prevMove The previous move played
     * @param move The current move to score
     * @return History score (higher is better)
     */
    inline int getScore(Move prevMove, Move move) const {
        if (prevMove == NO_MOVE || move == NO_MOVE) {
            return 0;
        }
        
        const Square prevTo = moveTo(prevMove);
        const Square from = moveFrom(move);
        const Square to = moveTo(move);
        
        // Validate inputs
        if (prevTo >= 64 || from >= 64 || to >= 64) {
            return 0;
        }
        
        return static_cast<int>(m_history[prevTo][from][to]);
    }
    
private:
    // History table: [prevTo][from][to]
    // Reduced from 67MB to 512KB by dropping [color] and [prevFrom]
    // Cache-aligned for better performance
    alignas(64) int16_t m_history[64][64][64];
};

} // namespace seajay::search