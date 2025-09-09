#include "ranked_move_picker.h"

namespace seajay {
namespace search {

/**
 * Phase 2a.0: Stub implementation
 * No ordering logic yet - this is just scaffolding
 */
RankedMovePicker::RankedMovePicker(const Board& board,
                                   Move ttMove,
                                   const KillerMoves* killers,
                                   const HistoryHeuristic* history,
                                   const CounterMoves* counterMoves,
                                   const CounterMoveHistory* counterMoveHistory,
                                   Move prevMove,
                                   int ply,
                                   int depth)
    : m_board(board)
    , m_killers(killers)
    , m_history(history)
    , m_counterMoves(counterMoves)
    , m_counterMoveHistory(counterMoveHistory)
    , m_ttMove(ttMove)
    , m_prevMove(prevMove)
    , m_ply(ply)
    , m_depth(depth)
{
    // Phase 2a.0: No initialization logic yet
    // This will be filled in during subsequent phases
}

Move RankedMovePicker::next() {
    // Phase 2a.0: Stub - always return NO_MOVE
    // This ensures no behavior change when the class is instantiated
    // but not actually used in the search
    return NO_MOVE;
}

/**
 * Phase 2a.0: QS stub implementation
 * Not actually used in Phase 2a (QS uses legacy path)
 */
RankedMovePickerQS::RankedMovePickerQS(const Board& board, Move ttMove)
    : m_board(board)
    , m_ttMove(ttMove)
{
    // Phase 2a.0: No initialization logic yet
}

Move RankedMovePickerQS::next() {
    // Phase 2a.0: Stub - always return NO_MOVE
    return NO_MOVE;
}

} // namespace search
} // namespace seajay
