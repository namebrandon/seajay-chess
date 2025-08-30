#pragma once

#include "../core/types.h"
#include <cstdint>

namespace seajay::search {

/**
 * History Heuristic for move ordering
 * 
 * Tracks which quiet moves have historically caused beta cutoffs
 * across the entire search tree, independent of position.
 * 
 * The history table is indexed by [side][from][to] and stores
 * a score indicating how often that move has been successful.
 * 
 * Features:
 * - Automatic aging to prevent overflow
 * - Cache-aligned for performance
 * - Depth-based bonus updates
 */
class HistoryHeuristic {
public:
    // Maximum history value before saturation
    // Using int16_t now, max safe value is 32767, but we keep 8192 for consistency
    static constexpr int16_t HISTORY_MAX = 8192;
    
    /**
     * Constructor - initializes empty history table
     */
    HistoryHeuristic() { clear(); }
    
    /**
     * Clear all history values to zero
     */
    void clear();
    
    /**
     * Update history score for a move that caused a beta cutoff
     * @param side Color of the moving side
     * @param from Source square of the move
     * @param to Destination square of the move
     * @param depth Current search depth (used for bonus calculation)
     */
    void update(Color side, Square from, Square to, int depth);
    
    /**
     * Reduce history score for a move that was tried but didn't cause cutoff
     * This implements the "butterfly" or "gravity" history update
     * @param side Color of the moving side
     * @param from Source square of the move
     * @param to Destination square of the move
     * @param depth Current search depth (used for penalty calculation)
     */
    void updateFailed(Color side, Square from, Square to, int depth);
    
    /**
     * Get the history score for a move
     * @param side Color of the moving side
     * @param from Source square of the move
     * @param to Destination square of the move
     * @return History score (higher is better)
     */
    inline int getScore(Color side, Square from, Square to) const {
        // Validate inputs only in debug builds
        #ifndef NDEBUG
        if (side >= NUM_COLORS || from >= 64 || to >= 64) {
            return 0;  // Invalid parameters
        }
        #endif
        
        // Return as int for compatibility with existing code
        return static_cast<int>(m_history[from][to][side]);
    }
    
    /**
     * Age all history values by dividing by 2
     * Called automatically when any value reaches HISTORY_MAX
     */
    void ageHistory();
    
private:
    // History table: [from][to][side]
    // Reordered for better cache locality - both colors for same move are adjacent
    // Using int16_t to halve memory footprint (16KB instead of 32KB)
    // Cache-aligned for better performance
    // Zero-initialized by default
    alignas(64) int16_t m_history[64][64][2] = {};
};

} // namespace seajay::search