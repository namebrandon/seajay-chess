#pragma once

/**
 * Phase 2a.3d: Ranked MovePicker - Legacy-Aligned Shortlist (Final Fix)
 * 
 * This implementation combines all fixes to eliminate regression:
 * - Captures only (no quiets, no non-capture promotions)
 * - K=8 (reduced from 10)
 * - CRITICAL: Shortlist extracted from legacy-ordered moves
 * - Ensures exact ordering alignment with legacy behavior
 * 
 * Safety constraints for Phase 2a:
 * - Disabled at root (ply==0) - enforced by caller
 * - Disabled in quiescence search - not wired in
 * - In-check nodes use legacy path (no shortlist)
 * 
 * Move yield order:
 * 1. TT move (if legal)
 * 2. Top-8 captures from legacy order (exact same order as legacy)
 * 3. Remainder via legacy ordering (skipping TT and shortlist)
 * 
 * Design principles:
 * - Legacy ordering applied ONCE to all moves
 * - Shortlist = first K captures from legacy order
 * - No separate scoring/sorting of shortlist
 * - Perfect alignment with existing behavior
 * - No tie-break differences or ordering drift
 */

#include "../core/types.h"
#include "../core/board.h"
#include "../core/move_list.h"
#include "../core/move_generation.h"
#include "history_heuristic.h"
#include "killer_moves.h"
#include "countermoves.h"
#include "countermove_history.h"
#include "types.h"  // For SearchData and SearchLimits

namespace seajay {
namespace search {

/**
 * RankedMovePicker - Main class for negamax search
 * 
 * Phase 2a.0: Stub implementation only
 * No actual ordering logic yet - just returns MOVE_NONE
 */
class RankedMovePicker {
public:
    /**
     * Constructor
     * @param board Current board position
     * @param ttMove Move from transposition table (NO_MOVE if none)
     * @param killers Killer moves table pointer
     * @param history History heuristic table pointer
     * @param counterMoves Counter moves table pointer
     * @param counterMoveHistory Counter move history table pointer
     * @param prevMove Previous move played
     * @param ply Current ply from root
     * @param depth Current search depth
     * @param countermoveBonus Bonus for countermove ordering
     * @param limits Search limits (for CMH weight)
     */
    RankedMovePicker(const Board& board,
                     Move ttMove,
                     const KillerMoves* killers,
                     const HistoryHeuristic* history,
                     const CounterMoves* counterMoves,
                     const CounterMoveHistory* counterMoveHistory,
                     Move prevMove,
                     int ply,
                     int depth,
                     int countermoveBonus,
                     const SearchLimits* limits);
    
    /**
     * Get next move in ranked order
     * @return Next move, or NO_MOVE when no more moves
     * 
     * Phase 2a.3: Yields TT, then shortlist (captures/quiets/promotions), then remainder
     */
    Move next();
    
private:
    // Constants
    static constexpr int SHORTLIST_SIZE = 8;  // Top-K moves (K=8 diagnostic)
    
    // References to tables (no ownership)
    const Board& m_board;
    const KillerMoves* m_killers;
    const HistoryHeuristic* m_history;
    const CounterMoves* m_counterMoves;
    const CounterMoveHistory* m_counterMoveHistory;
    
    // Current state
    Move m_ttMove;
    Move m_prevMove;
    int m_ply;
    int m_depth;
    int m_countermoveBonus;
    const SearchLimits* m_limits;
    
    // Phase 2a.3: Shortlist for top moves (captures, promotions, quiets)
    Move m_shortlist[SHORTLIST_SIZE];
    int16_t m_shortlistScores[SHORTLIST_SIZE];
    int m_shortlistSize;
    int m_shortlistIndex;
    bool m_inCheck;  // Flag to bypass shortlist when in check
    
    // Legacy-ordered move list
    MoveList m_moves;
    size_t m_moveIndex;
    bool m_ttMoveYielded;
    
    // Helper methods
    int16_t computeMvvLvaScore(Move move) const;
    int16_t computeQuietScore(Move move) const;
    int16_t computePromotionScore(Move move) const;
    void insertIntoShortlist(Move move, int16_t score);
    bool isInShortlist(Move move) const;
    
#ifdef DEBUG
    // Coverage tracking
    size_t m_generatedCount;
    size_t m_yieldedCount;
#endif
};

/**
 * RankedMovePickerQS - Simplified variant for quiescence search
 * 
 * Phase 2a.0: Stub implementation only
 * Not actually used in Phase 2a (QS uses legacy path)
 */
class RankedMovePickerQS {
public:
    /**
     * Constructor - only considers captures and promotions
     * @param board Current board position
     * @param ttMove Move from transposition table (NO_MOVE if none)
     */
    RankedMovePickerQS(const Board& board, Move ttMove);
    
    /**
     * Get next move
     * @return Next move, or NO_MOVE when no more moves
     * 
     * Phase 2a.0: Always returns NO_MOVE (stub)
     */
    Move next();
    
    /**
     * Check if picker has more moves
     * @return false (stub always empty)
     */
    bool hasNext() const { return false; }
    
private:
    const Board& m_board;
    Move m_ttMove;
};

} // namespace search
} // namespace seajay