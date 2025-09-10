#include "ranked_move_picker.h"
#include "move_ordering.h"
#include <algorithm>
#include <cassert>

namespace seajay {
namespace search {

// Use the MVV-LVA constants from move_ordering.h
// VICTIM_VALUES and ATTACKER_VALUES are already defined there

// Phase 2a.3: Scoring constants
static constexpr int16_t PROMOTION_BONUS[4] = {
    200,  // Knight promotion
    200,  // Bishop promotion  
    500,  // Rook promotion
    900   // Queen promotion
};

static constexpr int16_t KILLER_BONUS = 1000;
static constexpr int16_t COUNTERMOVE_BONUS = 500;
static constexpr int16_t REFUTATION_BONUS = 300;
static constexpr int16_t CHECK_BONUS = 50;
static constexpr int16_t HISTORY_MULTIPLIER = 2;

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
 * Compute score for a quiet move
 * Based on killers, history, countermoves, and CMH
 */
int16_t RankedMovePicker::computeQuietScore(Move move) const {
    int16_t score = 0;
    
    // History score (can be negative)
    if (m_history) {
        score += m_history->getScore(m_board.sideToMove(), moveFrom(move), moveTo(move)) * HISTORY_MULTIPLIER;
    }
    
    // Killer move bonus
    if (m_killers && m_killers->isKiller(move, m_ply)) {
        score += KILLER_BONUS;
    }
    
    // Countermove bonus
    if (m_counterMoves && m_prevMove != NO_MOVE) {
        if (m_counterMoves->getCounterMove(m_prevMove) == move) {
            score += COUNTERMOVE_BONUS;
        }
    }
    
    // Counter-move history bonus (if available)
    if (m_counterMoveHistory && m_prevMove != NO_MOVE) {
        score += m_counterMoveHistory->getScore(m_prevMove, move) / 2;  // Scale down CMH
    }
    
    // Small check bonus (cheap detection - could add if needed)
    // For now, omit as it requires extra computation
    
    return score;
}

/**
 * Compute score for a non-capture promotion
 * Base quiet score plus promotion bonus
 */
int16_t RankedMovePicker::computePromotionScore(Move move) const {
    if (!isPromotion(move)) {
        return 0;
    }
    
    // Start with base quiet score
    int16_t score = computeQuietScore(move);
    
    // Add promotion bonus based on piece type
    PieceType promoType = promotionType(move);
    if (promoType >= KNIGHT && promoType <= QUEEN) {
        score += PROMOTION_BONUS[promoType - KNIGHT];
    }
    
    return score;
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
 * Phase 2a.3a: Shortlist implementation (captures + promotions only, no quiets)
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
    
    // Phase 2a.3a: Build shortlist only if NOT in check (captures + promotions only, no quiets)
    if (!m_inCheck) {
        // Single pass to find top captures and promotions
        for (const Move& move : m_moves) {
            // Skip TT move (will be yielded first)
            if (move == m_ttMove) {
                continue;
            }
            
            int16_t score = 0;
            
            // Score captures (including capture-promotions)
            if (isCapture(move) || isEnPassant(move)) {
                score = computeMvvLvaScore(move);
                // Capture-promotions get both MVV-LVA and promotion bonus
                if (isPromotion(move)) {
                    PieceType promoType = promotionType(move);
                    if (promoType >= KNIGHT && promoType <= QUEEN) {
                        score += PROMOTION_BONUS[promoType - KNIGHT];
                    }
                }
                // Try to insert capture into shortlist
                insertIntoShortlist(move, score);
            }
            // Score non-capture promotions
            else if (isPromotion(move)) {
                score = computePromotionScore(move);
                // Try to insert promotion into shortlist
                insertIntoShortlist(move, score);
            }
            // Phase 2a.3a: Skip quiet moves entirely (don't add to shortlist)
            // Quiets will still be yielded via legacy ordering after shortlist
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
    // Phase 2a.3a: Yield TT move first if legal and not yet yielded
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
    
    // Phase 2a.3a: Yield shortlist moves (only if not in check)
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