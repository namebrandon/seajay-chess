#include "ranked_move_picker.h"
#include "move_ordering.h"
#include <algorithm>
#include <cassert>

namespace seajay {
namespace search {

/**
 * Phase 2a.1: TT-first implementation
 * Generate moves using legacy path, yield TT move first
 */
RankedMovePicker::RankedMovePicker(const Board& board,
                                   Move ttMove,
                                   const KillerMoves* killers,
                                   const HistoryHeuristic* history,
                                   const CounterMoves* counterMoves,
                                   const CounterMoveHistory* counterMoveHistory,
                                   Move prevMove,
                                   int ply,
                                   int depth,
                                   int countermoveBonus,
                                   const SearchLimits* limits)
    : m_board(board)
    , m_killers(killers)
    , m_history(history)
    , m_counterMoves(counterMoves)
    , m_counterMoveHistory(counterMoveHistory)
    , m_ttMove(ttMove)
    , m_prevMove(prevMove)
    , m_ply(ply)
    , m_depth(depth)
    , m_countermoveBonus(countermoveBonus)
    , m_limits(limits)
    , m_moveIndex(0)
    , m_ttMoveYielded(false)
#ifdef DEBUG
    , m_generatedCount(0)
    , m_yieldedCount(0)
#endif
{
    // Phase 2a.1: Generate and order moves using legacy path
    // Generate pseudo-legal moves
    MoveGenerator::generateMovesForSearch(board, m_moves, false);
    
#ifdef DEBUG
    m_generatedCount = m_moves.size();
#endif
    
    // Order moves using the exact same logic as negamax.cpp's orderMoves
    // This ensures we match the legacy ordering exactly
    static MvvLvaOrdering mvvLva;
    
    // Optional SEE capture ordering first (prefix-only) when enabled
    if (g_seeMoveOrdering.getMode() != SEEMode::OFF) {
        // SEE policy orders only captures/promotions prefix; quiets preserved
        g_seeMoveOrdering.orderMoves(board, m_moves);
    } else {
        // Default capture ordering
        mvvLva.orderMoves(board, m_moves);
    }

    // Use history heuristics for quiet moves if available
    if (depth >= 6 && killers && history && counterMoves && counterMoveHistory) {
        // Use counter-move history for enhanced move ordering at sufficient depth
        float cmhWeight = limits ? limits->counterMoveHistoryWeight : 1.5f;
        mvvLva.orderMovesWithHistory(board, m_moves, *killers, *history,
                                    *counterMoves, *counterMoveHistory,
                                    prevMove, ply, countermoveBonus, cmhWeight);
    } else if (killers && history && counterMoves) {
        // Fallback to basic countermoves without history
        mvvLva.orderMovesWithHistory(board, m_moves, *killers, *history,
                                    *counterMoves, prevMove, ply, countermoveBonus);
    }

    // Root-specific quiet move tweaks: we skip this since ply > 0
    // Note: We skip TT move ordering here since we handle it in next()
}

Move RankedMovePicker::next() {
    // Phase 2a.1: Yield TT move first if legal and not yet yielded
    if (m_ttMove != NO_MOVE && !m_ttMoveYielded) {
        m_ttMoveYielded = true;
        
        // Check if TT move is in our move list (pseudo-legal validation)
        bool ttMoveInList = std::find(m_moves.begin(), m_moves.end(), m_ttMove) != m_moves.end();
        
        if (ttMoveInList) {
#ifdef DEBUG
            m_yieldedCount++;
#endif
            return m_ttMove;
        }
        // If TT move not in list, skip it and continue to legacy moves
    }
    
    // Yield legacy-ordered moves, skipping TT move if present
    while (m_moveIndex < m_moves.size()) {
        Move move = m_moves[m_moveIndex++];
        
        // Skip TT move since we already yielded it (or tried to)
        if (move == m_ttMove) {
            continue;
        }
        
#ifdef DEBUG
        m_yieldedCount++;
#endif
        return move;
    }
    
#ifdef DEBUG
    // Assert coverage: yielded count should equal generated count (minus skipped TT move if present)
    size_t expectedYields = m_generatedCount;
    if (m_ttMove != NO_MOVE && std::find(m_moves.begin(), m_moves.end(), m_ttMove) != m_moves.end()) {
        // We yield TT move once, but skip it in the list, so count stays the same
        assert(m_yieldedCount == expectedYields && "Coverage mismatch: not all moves yielded");
    } else {
        assert(m_yieldedCount == expectedYields && "Coverage mismatch: not all moves yielded");
    }
#endif
    
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