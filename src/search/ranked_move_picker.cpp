#include "ranked_move_picker.h"
#include "move_ordering.h"
#include <algorithm>
#include <cassert>

namespace seajay {
namespace search {

// Use the MVV-LVA constants from move_ordering.h
// VICTIM_VALUES and ATTACKER_VALUES are already defined there

/**
 * Compute MVV-LVA score for a capture move
 * Higher scores = better captures (e.g., PxQ > QxP)
 */
int16_t RankedMovePicker::computeMvvLvaScore(Move move) const {
    // Only score captures (including capture-promotions)
    if (!isCapture(move) && !isEnPassant(move)) {
        return 0;
    }
    
    // Handle en passant (always PxP)
    if (isEnPassant(move)) {
        return VICTIM_VALUES[PAWN] - ATTACKER_VALUES[PAWN];  // 100 - 1 = 99
    }
    
    // Get attacker and victim pieces
    Square fromSq = moveFrom(move);
    Square toSq = moveTo(move);
    
    Piece attackingPiece = m_board.pieceAt(fromSq);
    Piece capturedPiece = m_board.pieceAt(toSq);
    
    if (attackingPiece == NO_PIECE || capturedPiece == NO_PIECE) {
        return 0;  // Safety fallback
    }
    
    PieceType attacker = typeOf(attackingPiece);
    PieceType victim = typeOf(capturedPiece);
    
    // For promotions, attacker is always PAWN
    if (isPromotion(move)) {
        attacker = PAWN;
    }
    
    // MVV-LVA formula: victim_value - attacker_value
    return VICTIM_VALUES[victim] - ATTACKER_VALUES[attacker];
}

/**
 * Insert a move into the shortlist if it's good enough
 * Maintains top-K moves sorted by score
 */
void RankedMovePicker::insertIntoShortlist(Move move, int16_t score) {
    // If shortlist not full, always insert
    if (m_shortlistSize < SHORTLIST_SIZE) {
        // Find insertion position (keep sorted, highest scores first)
        int insertPos = m_shortlistSize;
        for (int i = 0; i < m_shortlistSize; i++) {
            if (score > m_shortlistScores[i]) {
                insertPos = i;
                break;
            }
        }
        
        // Shift elements to make room
        for (int i = m_shortlistSize; i > insertPos; i--) {
            m_shortlist[i] = m_shortlist[i-1];
            m_shortlistScores[i] = m_shortlistScores[i-1];
        }
        
        // Insert the new move
        m_shortlist[insertPos] = move;
        m_shortlistScores[insertPos] = score;
        m_shortlistSize++;
    }
    // If shortlist is full, check if this move is better than the worst
    else if (score > m_shortlistScores[SHORTLIST_SIZE - 1]) {
        // Find insertion position
        int insertPos = SHORTLIST_SIZE - 1;
        for (int i = 0; i < SHORTLIST_SIZE - 1; i++) {
            if (score > m_shortlistScores[i]) {
                insertPos = i;
                break;
            }
        }
        
        // Shift elements (dropping the worst)
        for (int i = SHORTLIST_SIZE - 1; i > insertPos; i--) {
            m_shortlist[i] = m_shortlist[i-1];
            m_shortlistScores[i] = m_shortlistScores[i-1];
        }
        
        // Insert the new move
        m_shortlist[insertPos] = move;
        m_shortlistScores[insertPos] = score;
    }
}

/**
 * Check if a move is in the shortlist
 */
bool RankedMovePicker::isInShortlist(Move move) const {
    for (int i = 0; i < m_shortlistSize; i++) {
        if (m_shortlist[i] == move) {
            return true;
        }
    }
    return false;
}

/**
 * Phase 2a.2: Captures-only shortlist implementation
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
    , m_shortlistSize(0)
    , m_shortlistIndex(0)
    , m_inCheck(board.isAttacked(board.kingSquare(board.sideToMove()), ~board.sideToMove()))
    , m_moveIndex(0)
    , m_ttMoveYielded(false)
#ifdef DEBUG
    , m_generatedCount(0)
    , m_yieldedCount(0)
#endif
{
    // Generate pseudo-legal moves
    MoveGenerator::generateMovesForSearch(board, m_moves, false);
    
#ifdef DEBUG
    m_generatedCount = m_moves.size();
#endif
    
    // Phase 2a.2: Build shortlist only if NOT in check
    if (!m_inCheck) {
        // Single pass to find top captures
        for (const Move& move : m_moves) {
            // Only consider captures (including capture-promotions and en passant)
            if (isCapture(move) || isEnPassant(move)) {
                // Skip TT move (will be yielded first)
                if (move == m_ttMove) {
                    continue;
                }
                
                // Compute MVV-LVA score
                int16_t score = computeMvvLvaScore(move);
                
                // Try to insert into shortlist
                insertIntoShortlist(move, score);
            }
        }
    }
    
    // Order the remainder using legacy path
    // This ensures we have a complete ordering for all moves
    static MvvLvaOrdering mvvLva;
    
    // Optional SEE capture ordering first (prefix-only) when enabled
    if (g_seeMoveOrdering.getMode() != SEEMode::OFF) {
        g_seeMoveOrdering.orderMoves(board, m_moves);
    } else {
        mvvLva.orderMoves(board, m_moves);
    }

    // Use history heuristics for quiet moves if available
    if (depth >= 6 && killers && history && counterMoves && counterMoveHistory) {
        float cmhWeight = limits ? limits->counterMoveHistoryWeight : 1.5f;
        mvvLva.orderMovesWithHistory(board, m_moves, *killers, *history,
                                    *counterMoves, *counterMoveHistory,
                                    prevMove, ply, countermoveBonus, cmhWeight);
    } else if (killers && history && counterMoves) {
        mvvLva.orderMovesWithHistory(board, m_moves, *killers, *history,
                                    *counterMoves, prevMove, ply, countermoveBonus);
    }
}

Move RankedMovePicker::next() {
    // Phase 2a.2: Yield TT move first if legal and not yet yielded
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
        // If TT move not in list, skip it and continue
    }
    
    // Phase 2a.2: Yield shortlist moves (only if not in check)
    if (!m_inCheck && m_shortlistIndex < m_shortlistSize) {
        Move move = m_shortlist[m_shortlistIndex++];
#ifdef DEBUG
        m_yieldedCount++;
#endif
        return move;
    }
    
    // Yield legacy-ordered moves, skipping TT move and shortlist moves
    while (m_moveIndex < m_moves.size()) {
        Move move = m_moves[m_moveIndex++];
        
        // Skip TT move since we already yielded it (or tried to)
        if (move == m_ttMove) {
            continue;
        }
        
        // Skip moves that are in the shortlist (already yielded)
        if (!m_inCheck && isInShortlist(move)) {
            continue;
        }
        
#ifdef DEBUG
        m_yieldedCount++;
#endif
        return move;
    }
    
#ifdef DEBUG
    // Assert coverage: all generated moves should be yielded exactly once
    assert(m_yieldedCount == m_generatedCount && "Coverage mismatch: not all moves yielded");
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