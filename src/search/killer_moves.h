#pragma once

#include "../core/types.h"

namespace seajay::search {

/**
 * Killer Moves Heuristic (Stage 19)
 * 
 * Tracks quiet moves that caused beta cutoffs at each ply.
 * The hypothesis is that a move that was good in a sibling 
 * position might also be good in the current position.
 * 
 * Maintains 2 killer slots per ply, with newer killers
 * replacing older ones in a simple replacement scheme.
 */
class KillerMoves {
public:
    static constexpr int MAX_PLY = 128;
    static constexpr int KILLERS_PER_PLY = 2;
    
    KillerMoves() { clear(); }
    
    /**
     * Clear all killer moves
     */
    void clear();
    
    /**
     * Update killer moves for a given ply
     * @param ply The current search depth/ply
     * @param move The quiet move that caused a beta cutoff
     */
    void update(int ply, Move move);
    
    /**
     * Check if a move is a killer at the given ply
     * @param ply The current search depth/ply
     * @param move The move to check
     * @return true if the move is a killer at this ply
     */
    bool isKiller(int ply, Move move) const;
    
    /**
     * Get a specific killer move
     * @param ply The current search depth/ply
     * @param slot Which killer slot (0 or 1)
     * @return The killer move or NO_MOVE if slot is empty
     */
    Move getKiller(int ply, int slot) const;
    
private:
    // Killer move storage: [ply][slot]
    // Slot 0 is the most recent killer, slot 1 is the previous
    Move m_killers[MAX_PLY][KILLERS_PER_PLY];
};

} // namespace seajay::search