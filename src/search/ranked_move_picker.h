#pragma once

/**
 * Phase 2a.0: Ranked MovePicker - Scaffolds Only
 * 
 * This is a minimal stub implementation with no actual ordering logic.
 * The class exists only to establish the API and allow for clean compilation.
 * 
 * Safety constraints for Phase 2a:
 * - Disabled at root (ply==0) - enforced by caller
 * - Disabled in quiescence search - not wired in
 * - TT move deduplication to be handled in later phases
 * 
 * Design principles:
 * - Single-pass O(n), no quadratic work or repeated sorts
 * - No dynamic allocations, fixed-size stack arrays only
 * - In-check parity: use check evasions when in check
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
     */
    RankedMovePicker(const Board& board,
                     Move ttMove,
                     const KillerMoves* killers,
                     const HistoryHeuristic* history,
                     const CounterMoves* counterMoves,
                     const CounterMoveHistory* counterMoveHistory,
                     Move prevMove,
                     int ply,
                     int depth);
    
    /**
     * Get next move in ranked order
     * @return Next move, or NO_MOVE when no more moves
     * 
     * Phase 2a.0: Always returns NO_MOVE (stub)
     */
    Move next();
    
private:
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
    
    // Legacy-ordered move list
    MoveList m_moves;
    size_t m_moveIndex;
    bool m_ttMoveYielded;
    
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