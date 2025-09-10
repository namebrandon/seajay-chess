#pragma once

/**
 * Phase 2a.2: Ranked MovePicker - Captures-only Shortlist
 * 
 * This implementation adds a Top-K shortlist of captures scored by MVV-LVA.
 * The shortlist improves early decision quality for tactical moves.
 * 
 * Safety constraints for Phase 2a:
 * - Disabled at root (ply==0) - enforced by caller
 * - Disabled in quiescence search - not wired in
 * - In-check nodes use legacy path (no shortlist)
 * 
 * Move yield order:
 * 1. TT move (if legal)
 * 2. Top-K captures shortlist (MVV-LVA ordered)
 * 3. Remainder via legacy ordering (skipping TT and shortlist)
 * 
 * Design principles:
 * - Single-pass O(n), no quadratic work or repeated sorts
 * - No dynamic allocations, fixed-size stack arrays only
 * - In-check parity: bypass shortlist when in check
 * - Clean fallback: feature behind UCI toggle, legacy path intact
 * - Deterministic ordering with clear tie-breaks
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
     * Phase 2a.2: Yields TT, then shortlist, then remainder
     */
    Move next();
    
private:
    // Constants
    static constexpr int SHORTLIST_SIZE = 8;  // Top-K captures
    
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
    
    // Phase 2a.2: Shortlist for top captures
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