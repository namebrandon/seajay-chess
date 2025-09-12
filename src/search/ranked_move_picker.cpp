#include "ranked_move_picker.h"
#include "move_ordering.h"
#include <algorithm>
#include <cassert>
#include <cstring>  // For std::fill

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
#ifdef DEBUG
    // Phase 2a.5b: Pre-condition checks
    assert(m_shortlistSize >= 0 && m_shortlistSize <= MAX_SHORTLIST_SIZE);
#endif
    
    // If shortlist not full, always insert
    if (m_shortlistSize < MAX_SHORTLIST_SIZE) {
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
#ifdef DEBUG
            // Phase 2a.5b: Assert array bounds
            assert(i > 0 && i <= MAX_SHORTLIST_SIZE);
            assert(i-1 >= 0 && i-1 < MAX_SHORTLIST_SIZE);
#endif
            m_shortlist[i] = m_shortlist[i-1];
            m_shortlistScores[i] = m_shortlistScores[i-1];
        }
        
        // Insert the new move
#ifdef DEBUG
        assert(insertPos >= 0 && insertPos < MAX_SHORTLIST_SIZE);
        assert(m_shortlistSize < MAX_SHORTLIST_SIZE);
#endif
        m_shortlist[insertPos] = move;
        m_shortlistScores[insertPos] = score;
        m_shortlistSize++;
#ifdef DEBUG
        assert(m_shortlistSize <= MAX_SHORTLIST_SIZE);
#endif
    }
    // If shortlist is full, check if this move is better than the worst
    else if (score > m_shortlistScores[MAX_SHORTLIST_SIZE - 1]) {
        // Find insertion position
        int insertPos = MAX_SHORTLIST_SIZE - 1;
        for (int i = 0; i < MAX_SHORTLIST_SIZE - 1; i++) {
            if (score > m_shortlistScores[i]) {
                insertPos = i;
                break;
            }
        }
        
        // Shift elements (dropping the worst)
        for (int i = MAX_SHORTLIST_SIZE - 1; i > insertPos; i--) {
#ifdef DEBUG
            // Phase 2a.5b: Assert array bounds  
            assert(i > 0 && i < MAX_SHORTLIST_SIZE);
            assert(i-1 >= 0 && i-1 < MAX_SHORTLIST_SIZE);
#endif
            m_shortlist[i] = m_shortlist[i-1];
            m_shortlistScores[i] = m_shortlistScores[i-1];
        }
        
        // Insert the new move
#ifdef DEBUG
        assert(insertPos >= 0 && insertPos < MAX_SHORTLIST_SIZE);
#endif
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
 * Phase 2a.4: In-check parity implementation
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
                                   const SearchLimits* limits,
                                   SearchData* searchData)
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
    , m_searchData(searchData)
    , m_shortlistSize(0)
    , m_shortlistIndex(0)
    , m_effectiveShortlistSize(0)  // Will be set based on depth
    , m_inCheck(board.isAttacked(board.kingSquare(board.sideToMove()), ~board.sideToMove()))
    , m_moveIndex(0)
    , m_ttMoveYielded(false)
#ifdef SEARCH_STATS
    , m_yieldIndex(0)
#endif
#ifdef DEBUG
    , m_generatedCount(0)
    , m_yieldedCount(0)
#endif
{
    // Initialize shortlist map to all false
    std::fill(std::begin(m_inShortlistMap), std::end(m_inShortlistMap), false);
    
    // Phase 2a.5b: Initialize arrays to prevent UB
    std::fill(std::begin(m_shortlist), std::end(m_shortlist), NO_MOVE);
    std::fill(std::begin(m_shortlistScores), std::end(m_shortlistScores), 0);
    
    // Calculate depth-based shortlist size (K)
    // Shallow depths: less overhead, fewer moves in shortlist
    // Deeper depths: full shortlist for better move ordering
    if (depth < 3) {
        m_effectiveShortlistSize = 0;  // No shortlist at very shallow depths
    } else if (depth < 6) {
        m_effectiveShortlistSize = 4;  // Small shortlist at shallow depths
    } else {
        m_effectiveShortlistSize = 8;  // Full shortlist at deeper depths
    }
    
#ifdef DEBUG
    // Phase 2a.5b: Assert shortlist size bounds
    assert(m_effectiveShortlistSize >= 0 && m_effectiveShortlistSize <= MAX_SHORTLIST_SIZE);
    assert(m_shortlistSize == 0 && "Shortlist size must be initialized to 0");
    assert(m_shortlistIndex == 0 && "Shortlist index must be initialized to 0");
#endif
    
    // Phase 2a.4: When in check, use optimized check evasion generation
    if (m_inCheck) {
        // Use the same generator as legacy (generateMovesForSearch calls generateCheckEvasions internally)
        // This is much more efficient than generateLegalMoves
        MoveGenerator::generateMovesForSearch(board, m_moves, false);
        
#ifdef DEBUG
        m_generatedCount = m_moves.size();
        // Phase 2a.8a: Assert that we only have evasions when in check
        // All moves generated should be check evasions
        assert(m_generatedCount > 0 || board.isCheckmate() && "Must have evasions unless checkmate");
#endif
        
        // Phase 2a.8b: Class-based ordering for check evasions
        if (limits && limits->useInCheckClassOrdering) {
            // Precompute checker information once
            const Color stm = board.sideToMove();
            const Square kingSq = board.kingSquare(stm);
            const Color oppSide = ~stm;

            Bitboard checkers = 0;
            // Pawn checks
            checkers |= ::seajay::pawnAttacks(stm, kingSq) & board.pieces(oppSide, PAWN);
            // Knight checks
            checkers |= MoveGenerator::getKnightAttacks(kingSq) & board.pieces(oppSide, KNIGHT);
            // Bishop/queen diagonal checks
            checkers |= ::seajay::bishopAttacks(kingSq, board.occupied()) & (board.pieces(oppSide, BISHOP) | board.pieces(oppSide, QUEEN));
            // Rook/queen orthogonal checks
            checkers |= ::seajay::rookAttacks(kingSq, board.occupied()) & (board.pieces(oppSide, ROOK) | board.pieces(oppSide, QUEEN));

            const int numCheckers = popCount(checkers);
            Square checkerSq = NO_SQUARE;
            Bitboard blockMask = 0;
            if (numCheckers == 1) {
                checkerSq = lsb(checkers);
                const Piece checkerPiece = board.pieceAt(checkerSq);
                const PieceType checkerType = typeOf(checkerPiece);
                if (checkerType == BISHOP || checkerType == ROOK || checkerType == QUEEN) {
                    blockMask = ::seajay::between(checkerSq, kingSq);
                }
            }

            // Helper lambdas to classify without auxiliary arrays
            auto isCaptureOfChecker = [&](const Move& mv) -> bool {
                if (numCheckers != 1) return false;
                const Square from = moveFrom(mv);
                if (typeOf(board.pieceAt(from)) == KING) return false; // King moves not here
                const Square to = moveTo(mv);
                if (isEnPassant(mv)) {
                    // EP capture square is behind 'to' depending on side to move
                    const int delta = (stm == WHITE) ? -8 : 8;
                    Square capturedSq = static_cast<Square>(to + delta);
                    return capturedSq == checkerSq;
                }
                return (isCapture(mv) || isEnPassant(mv)) && (to == checkerSq);
            };

            auto isBlockMove = [&](const Move& mv) -> bool {
                if (numCheckers != 1 || blockMask == 0) return false;
                const Square from = moveFrom(mv);
                if (typeOf(board.pieceAt(from)) == KING) return false; // King cannot block
                const Square to = moveTo(mv);
                return !isCapture(mv) && !isEnPassant(mv) && testBit(blockMask, to);
            };

            // Stable partition: [captures of checker][blocks][king/other]
            auto class1End = std::stable_partition(m_moves.begin(), m_moves.end(), isCaptureOfChecker);
            std::stable_partition(class1End, m_moves.end(), isBlockMove);

            // Skip the legacy ordering below since we've already ordered by class
            // No history/CMH applied in this path as per design

            // No shortlist when in check - we'll iterate evasions directly
#ifdef DEBUG
            // Phase 2a.5b: When in check, assert no shortlist
            assert(m_shortlistSize == 0 && "No shortlist when in check");
#endif
            return;  // Early return to skip legacy ordering
        }
        
        // Apply legacy ordering to the evasions with history (same as non-check path)
        static MvvLvaOrdering mvvLva;
        
        if (g_seeMoveOrdering.getMode() != SEEMode::OFF) {
            g_seeMoveOrdering.orderMoves(board, m_moves);
        } else {
            mvvLva.orderMoves(board, m_moves);
        }
        
        // Apply history heuristics to evasions for better move ordering
        // This matches legacy behavior and should improve tactical positions
        if (depth >= 6 && killers && history && counterMoves && counterMoveHistory) {
            float cmhWeight = limits ? limits->counterMoveHistoryWeight : 1.5f;
            mvvLva.orderMovesWithHistory(board, m_moves, *killers, *history,
                                        *counterMoves, *counterMoveHistory,
                                        prevMove, ply, countermoveBonus, cmhWeight);
        } else if (killers && history && counterMoves) {
            mvvLva.orderMovesWithHistory(board, m_moves, *killers, *history,
                                        *counterMoves, prevMove, ply, countermoveBonus);
        }
        
        // No shortlist when in check - we'll iterate evasions directly
        
#ifdef DEBUG
        // Phase 2a.5b: When in check, assert no shortlist
        assert(m_shortlistSize == 0 && "No shortlist when in check");
#endif
    }
    else {
        // Not in check: normal pseudo-legal generation and shortlist building
        MoveGenerator::generateMovesForSearch(board, m_moves, false);
        
#ifdef DEBUG
        m_generatedCount = m_moves.size();
#endif
        
        // Phase 2a.3d: Order moves using legacy ordering FIRST
        // This ensures perfect alignment with existing behavior
        static MvvLvaOrdering mvvLva;
        
        // Apply legacy ordering to all moves (with ttMove=NO_MOVE to avoid special handling)
        // This gives us the exact legacy order
        if (g_seeMoveOrdering.getMode() != SEEMode::OFF) {
            g_seeMoveOrdering.orderMoves(board, m_moves);
        } else {
            mvvLva.orderMoves(board, m_moves);
        }

        // Apply history heuristics for quiet moves if available
        if (depth >= 6 && killers && history && counterMoves && counterMoveHistory) {
            float cmhWeight = limits ? limits->counterMoveHistoryWeight : 1.5f;
            mvvLva.orderMovesWithHistory(board, m_moves, *killers, *history,
                                        *counterMoves, *counterMoveHistory,
                                        prevMove, ply, countermoveBonus, cmhWeight);
        } else if (killers && history && counterMoves) {
            mvvLva.orderMovesWithHistory(board, m_moves, *killers, *history,
                                        *counterMoves, prevMove, ply, countermoveBonus);
        }
        
        // Phase 2a.3d: Extract first K captures from legacy-ordered list as shortlist
        // Only build shortlist if K > 0 (depth >= 3)
        if (m_effectiveShortlistSize > 0) {
            // Walk the legacy-ordered list and take the first K captures
            for (size_t i = 0; i < m_moves.size(); ++i) {
            const Move& move = m_moves[i];
            
            // Skip TT move (will be yielded first)
            if (move == m_ttMove) {
                continue;
            }
            
            // Take captures and promotions for the shortlist (no quiets)
            // This ensures non-capture promotions aren't delayed
            if ((isCapture(move) || isEnPassant(move) || isPromotion(move)) && m_shortlistSize < m_effectiveShortlistSize) {
#ifdef DEBUG
                // Phase 2a.5b: Assert bounds before array write
                assert(m_shortlistSize >= 0 && m_shortlistSize < MAX_SHORTLIST_SIZE);
                assert(i < MAX_MOVES && "Move index out of bounds for shortlist map");
#endif
                
#ifdef SEARCH_STATS
                // Phase 2a.6b: Track captures observed (only when stats are requested)
                if (m_limits && m_limits->showMovePickerStats) {
                    if (m_searchData && (isCapture(move) || isEnPassant(move))) {
                        m_searchData->movePickerStats.capturesTotal++;
                    }
                }
#endif
                
                m_shortlist[m_shortlistSize] = move;
                m_shortlistScores[m_shortlistSize] = 0; // Not used, but initialize
                m_inShortlistMap[i] = true;  // Mark this index as in shortlist
                m_shortlistSize++;
            }
            
            // Stop once we have K captures/promotions (using depth-based K)
            if (m_shortlistSize >= m_effectiveShortlistSize) {
                break;
            }
            }
        }
    }
}

Move RankedMovePicker::next() {
    // Phase 2a.4: Yield TT move first if legal and not yet yielded
    if (m_ttMove != NO_MOVE && !m_ttMoveYielded) {
        m_ttMoveYielded = true;
        
        // Check if TT move is in our move list
        // For in-check: TT must be a valid evasion (in m_moves)
        // For normal: TT must be pseudo-legal (in m_moves)
        bool ttMoveInList = std::find(m_moves.begin(), m_moves.end(), m_ttMove) != m_moves.end();
        
#ifdef DEBUG
        // Phase 2a.8a: Assert TT move validity when in check
        if (m_inCheck && ttMoveInList) {
            // TT move must be a legal evasion (present in the evasion list)
            assert(std::find(m_moves.begin(), m_moves.end(), m_ttMove) != m_moves.end() 
                   && "TT move must be in evasion list when in check");
        }
#endif
        
        if (ttMoveInList) {
#ifdef SEARCH_STATS
            // Increment only when stats are requested to avoid hot-path overhead
            if (m_limits && m_limits->showMovePickerStats) {
                m_yieldIndex++;  // Increment yield index for TT move
                // Phase 2a.6b: Track TT first yield
                if (m_searchData) {
                    m_searchData->movePickerStats.ttFirstYield++;
                }
            }
#endif
#ifdef DEBUG
            m_yieldedCount++;
#endif
            return m_ttMove;
        }
        // If TT move not in list, skip it and continue
    }
    
    // Phase 2a.4: Yield shortlist moves (only if not in check)
    if (!m_inCheck && m_shortlistIndex < m_shortlistSize) {
#ifdef DEBUG
        // Phase 2a.5b: Assert shortlist bounds
        assert(m_shortlistIndex >= 0 && m_shortlistIndex < m_shortlistSize);
        assert(m_shortlistSize <= MAX_SHORTLIST_SIZE);
        // Phase 2a.8a: Assert we're not in check when using shortlist
        assert(!m_inCheck && "Shortlist should not be used when in check");
#endif
        Move move = m_shortlist[m_shortlistIndex++];
#ifdef SEARCH_STATS
        if (m_limits && m_limits->showMovePickerStats) {
            m_yieldIndex++;  // Increment yield index for shortlist move
        }
#endif
#ifdef DEBUG
        m_yieldedCount++;
#endif
        return move;
    }
    
    // Yield moves from m_moves in legacy order
    // For in-check: these are check evasions (MVV-LVA/SEE ordered)
    // For normal: these are pseudo-legal moves (skipping TT and shortlist)
    while (m_moveIndex < m_moves.size()) {
#ifdef DEBUG
        // Phase 2a.5b: Assert iterator bounds
        assert(m_moveIndex <= m_moves.size() && "Move index out of bounds");
#endif
        size_t currentIndex = m_moveIndex;
        Move move = m_moves[m_moveIndex++];
        
        // Skip TT move since we already yielded it (or tried to)
        if (move == m_ttMove) {
            continue;
        }
        
        // Skip moves that are in the shortlist (already yielded) - only when not in check
        // Use O(1) lookup instead of linear search
        if (!m_inCheck && currentIndex < MAX_MOVES && m_inShortlistMap[currentIndex]) {
#ifdef DEBUG
            // Phase 2a.5b: Additional bounds check for paranoia
            assert(currentIndex < MAX_MOVES && "Current index must be within MAX_MOVES");
#endif
            continue;
        }
        
#ifdef SEARCH_STATS
        if (m_limits && m_limits->showMovePickerStats) {
            m_yieldIndex++;  // Increment yield index for remainder move
            // Phase 2a.6b: Track remainder yields
            if (m_searchData) {
                m_searchData->movePickerStats.remainderYields++;
            }
        }
#endif
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
    // Phase 2a.7: Verify this is never constructed in Phase 2a
#ifdef DEBUG
    assert(false && "RankedMovePickerQS should not be constructed in Phase 2a (QS uses legacy path)");
#endif
    // Phase 2a.0: No initialization logic yet
}

Move RankedMovePickerQS::next() {
    // Phase 2a.7: This should never be called in Phase 2a
#ifdef DEBUG
    assert(false && "RankedMovePickerQS::next() should not be called in Phase 2a");
#endif
    // Phase 2a.0: Stub - always return NO_MOVE
    return NO_MOVE;
}

#ifdef SEARCH_STATS
/**
 * Check if a move was in the shortlist
 * Phase 2a.6: Telemetry support
 */
bool RankedMovePicker::wasInShortlist(Move m) const {
    // Check if move is in our shortlist
    for (int i = 0; i < m_shortlistSize; i++) {
        if (m_shortlist[i] == m) {
            return true;
        }
    }
    return false;
}
#endif

} // namespace search
} // namespace seajay
