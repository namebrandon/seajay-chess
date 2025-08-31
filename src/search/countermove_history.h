#pragma once

#include "../core/types.h"
#include <cstdint>
#include <cstring>
#include <algorithm>

namespace seajay::search {

/**
 * Counter-Move History Heuristic
 * 
 * This extends the history heuristic concept by tracking successful move pairs.
 * Instead of just tracking individual moves, we track how successful a move is
 * when played in response to a specific previous move.
 * 
 * The table is indexed by [color][prevFrom][prevTo][from][to] and stores
 * a score indicating how often that move sequence has been successful.
 * 
 * Memory usage: 2 * 64 * 64 * 64 * 64 * sizeof(int16_t) = 32MB
 * 
 * Thread-safety: Each thread should have its own instance (thread-local)
 * or use atomic operations for updates.
 */
class CounterMoveHistory {
public:
    // Maximum history value before saturation (same as HistoryHeuristic)
    static constexpr int16_t HISTORY_MAX = 8192;
    
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
     * @param side Color of the moving side
     * @param prevMove The previous move played
     * @param move The current move that caused the cutoff
     * @param depth Current search depth (used for bonus calculation)
     */
    void update(Color side, Move prevMove, Move move, int depth) {
        if (prevMove == NO_MOVE || move == NO_MOVE) {
            return;
        }
        
        // Don't track captures or promotions in history
        if (isCapture(move) || isPromotion(move)) {
            return;
        }
        
        const Square prevFrom = moveFrom(prevMove);
        const Square prevTo = moveTo(prevMove);
        const Square from = moveFrom(move);
        const Square to = moveTo(move);
        
        // Validate squares
        if (prevFrom >= 64 || prevTo >= 64 || from >= 64 || to >= 64 || side >= NUM_COLORS) {
            return;
        }
        
        // Calculate bonus based on depth (same formula as HistoryHeuristic)
        int bonus = depth * depth;
        
        // Saturating add with automatic aging
        int16_t& entry = m_history[side][prevFrom][prevTo][from][to];
        int newValue = static_cast<int>(entry) + bonus;
        
        if (newValue >= HISTORY_MAX) {
            // Age all values when we hit the max
            ageHistory();
            entry = HISTORY_MAX / 2;  // Set this entry to half max after aging
        } else {
            entry = static_cast<int16_t>(newValue);
        }
    }
    
    /**
     * Reduce history score for a move pair that was tried but didn't cause cutoff
     * @param side Color of the moving side
     * @param prevMove The previous move played
     * @param move The current move that failed
     * @param depth Current search depth (used for penalty calculation)
     */
    void updateFailed(Color side, Move prevMove, Move move, int depth) {
        if (prevMove == NO_MOVE || move == NO_MOVE) {
            return;
        }
        
        // Don't track captures or promotions in history
        if (isCapture(move) || isPromotion(move)) {
            return;
        }
        
        const Square prevFrom = moveFrom(prevMove);
        const Square prevTo = moveTo(prevMove);
        const Square from = moveFrom(move);
        const Square to = moveTo(move);
        
        // Validate squares
        if (prevFrom >= 64 || prevTo >= 64 || from >= 64 || to >= 64 || side >= NUM_COLORS) {
            return;
        }
        
        // Calculate penalty based on depth (same formula as HistoryHeuristic)
        int penalty = depth * depth;
        
        // Saturating subtract
        int16_t& entry = m_history[side][prevFrom][prevTo][from][to];
        int newValue = static_cast<int>(entry) - penalty;
        entry = static_cast<int16_t>(std::max(newValue, -static_cast<int>(HISTORY_MAX)));
    }
    
    /**
     * Get the history score for a move pair
     * @param side Color of the moving side
     * @param prevMove The previous move played
     * @param move The current move to score
     * @return History score (higher is better)
     */
    inline int getScore(Color side, Move prevMove, Move move) const {
        if (prevMove == NO_MOVE || move == NO_MOVE) {
            return 0;
        }
        
        const Square prevFrom = moveFrom(prevMove);
        const Square prevTo = moveTo(prevMove);
        const Square from = moveFrom(move);
        const Square to = moveTo(move);
        
        // Validate inputs
        if (prevFrom >= 64 || prevTo >= 64 || from >= 64 || to >= 64 || side >= NUM_COLORS) {
            return 0;
        }
        
        return static_cast<int>(m_history[side][prevFrom][prevTo][from][to]);
    }
    
    /**
     * Age all history values by dividing by 2
     * Called automatically when any value reaches HISTORY_MAX
     */
    void ageHistory() {
        for (int color = 0; color < NUM_COLORS; ++color) {
            for (int pf = 0; pf < 64; ++pf) {
                for (int pt = 0; pt < 64; ++pt) {
                    for (int f = 0; f < 64; ++f) {
                        for (int t = 0; t < 64; ++t) {
                            m_history[color][pf][pt][f][t] /= 2;
                        }
                    }
                }
            }
        }
    }
    
private:
    // History table: [color][prevFrom][prevTo][from][to]
    // Using int16_t to keep memory reasonable (32MB total)
    // Cache-aligned for better performance
    alignas(64) int16_t m_history[2][64][64][64][64];
};

} // namespace seajay::search