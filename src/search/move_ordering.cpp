#include "move_ordering.h"
#include <algorithm>
#include <iostream>

// Debug output control
#ifdef DEBUG_MOVE_ORDERING
    static bool g_debugMoveOrdering = true;
#else
    static bool g_debugMoveOrdering = false;
#endif

namespace seajay::search {

// MVV-LVA table: [victim_type][attacker_type] = score
// Higher scores = better moves (capture high value with low value)
static constexpr int MVV_LVA_TABLE[7][7] = {
    {0, 0, 0, 0, 0, 0, 0},              // NO_PIECE_TYPE victim (not used)
    {0, 105, 104, 103, 102, 101, 100},  // PAWN victim
    {0, 205, 204, 203, 202, 201, 200},  // KNIGHT victim
    {0, 305, 304, 303, 302, 301, 300},  // BISHOP victim
    {0, 405, 404, 403, 402, 401, 400},  // ROOK victim
    {0, 505, 504, 503, 502, 501, 500},  // QUEEN victim
    {0, 605, 604, 603, 602, 601, 600},  // KING victim (shouldn't happen)
};

// Score a single move using MVV-LVA heuristic
int MvvLvaOrdering::scoreMove(const Board& board, Move move) noexcept {
    auto& stats = getStatistics();
    
    // Handle promotions first (highest priority)
    if (isPromotion(move)) {
        stats.promotions_scored++;
        
        // Get the promotion piece type (KNIGHT=1, BISHOP=2, ROOK=3, QUEEN=4)
        PieceType promoType = promotionType(move);
        int baseScore = PROMOTION_BASE_SCORE;
        
        // Add promotion type bonus (Queen > Rook > Bishop > Knight)
        if (promoType >= KNIGHT && promoType <= QUEEN) {
            baseScore += PROMOTION_TYPE_BONUS[promoType - KNIGHT];
        }
        
        // If it's also a capture, add MVV-LVA score
        // CRITICAL: Attacker is always PAWN for promotions, not the promoted piece!
        if (isCapture(move)) {
            Square toSq = moveTo(move);
            Piece capturedPiece = board.pieceAt(toSq);
            
            if (capturedPiece != NO_PIECE) {
                PieceType victim = typeOf(capturedPiece);
                // Use table lookup for consistency
                baseScore += MVV_LVA_TABLE[victim][PAWN];
            }
        }
        
        return baseScore;
    }
    
    // Handle en passant captures (special case)
    if (isEnPassant(move)) {
        stats.en_passants_scored++;
        // En passant is always PxP (pawn captures pawn)
        return MVV_LVA_TABLE[PAWN][PAWN];
    }
    
    // Handle regular captures
    if (isCapture(move)) {
        stats.captures_scored++;
        
        Square fromSq = moveFrom(move);
        Square toSq = moveTo(move);
        
        Piece attackingPiece = board.pieceAt(fromSq);
        Piece capturedPiece = board.pieceAt(toSq);
        
        // Validate pieces
        MVV_LVA_ASSERT(attackingPiece != NO_PIECE, "No attacking piece at from square");
        MVV_LVA_ASSERT(capturedPiece != NO_PIECE, "No captured piece at to square");
        
        if (attackingPiece == NO_PIECE || capturedPiece == NO_PIECE) {
            return 0;  // Safety fallback
        }
        
        PieceType attacker = typeOf(attackingPiece);
        PieceType victim = typeOf(capturedPiece);
        
        // King captures should never happen in legal chess
        MVV_LVA_ASSERT(victim != KING, "Attempting to capture king!");
        
        return MVV_LVA_TABLE[victim][attacker];
    }
    
    // Quiet moves get zero score (will be ordered last)
    stats.quiet_moves++;
    return 0;
}

// Order moves using MVV-LVA scoring - OPTIMIZED VERSION
// CRITICAL: Only sorts captures, preserves quiet move order from generator
void MvvLvaOrdering::orderMoves(const Board& board, MoveList& moves) const {
    // Nothing to order if empty or single move
    if (moves.size() <= 1) {
        return;
    }
    
#ifdef DEBUG_MOVE_ORDERING
    if (g_debugMoveOrdering) {
        std::cout << "[MVV-LVA] Ordering " << moves.size() << " moves (optimized)" << std::endl;
    }
#endif
    
    // CRITICAL OPTIMIZATION: In-place partition to separate captures from quiet moves
    // This preserves the natural order of quiet moves from the generator
    auto captureEnd = std::stable_partition(moves.begin(), moves.end(),
        [&board](const Move& move) {
            // Promotions and captures go to the front
            return isPromotion(move) || isCapture(move) || isEnPassant(move);
        });
    
    // Only sort the captures/promotions portion if there are any
    if (captureEnd != moves.begin()) {
        // Sort captures by MVV-LVA score (higher scores first)
        std::sort(moves.begin(), captureEnd,
            [&board](const Move& a, const Move& b) {
                // Inline scoring for performance (avoid function call overhead)
                int scoreA, scoreB;
                
                // Score move A
                if (isPromotion(a)) {
                    scoreA = PROMOTION_BASE_SCORE;
                    PieceType promoType = promotionType(a);
                    if (promoType >= KNIGHT && promoType <= QUEEN) {
                        scoreA += PROMOTION_TYPE_BONUS[promoType - KNIGHT];
                    }
                    if (isCapture(a)) {
                        Piece victim = board.pieceAt(moveTo(a));
                        if (victim != NO_PIECE) {
                            scoreA += MVV_LVA_TABLE[typeOf(victim)][PAWN];
                        }
                    }
                } else if (isEnPassant(a)) {
                    scoreA = MVV_LVA_TABLE[PAWN][PAWN];
                } else if (isCapture(a)) {
                    Piece victim = board.pieceAt(moveTo(a));
                    Piece attacker = board.pieceAt(moveFrom(a));
                    if (victim != NO_PIECE && attacker != NO_PIECE) {
                        scoreA = MVV_LVA_TABLE[typeOf(victim)][typeOf(attacker)];
                    } else {
                        scoreA = 0;
                    }
                } else {
                    scoreA = 0;
                }
                
                // Score move B
                if (isPromotion(b)) {
                    scoreB = PROMOTION_BASE_SCORE;
                    PieceType promoType = promotionType(b);
                    if (promoType >= KNIGHT && promoType <= QUEEN) {
                        scoreB += PROMOTION_TYPE_BONUS[promoType - KNIGHT];
                    }
                    if (isCapture(b)) {
                        Piece victim = board.pieceAt(moveTo(b));
                        if (victim != NO_PIECE) {
                            scoreB += MVV_LVA_TABLE[typeOf(victim)][PAWN];
                        }
                    }
                } else if (isEnPassant(b)) {
                    scoreB = MVV_LVA_TABLE[PAWN][PAWN];
                } else if (isCapture(b)) {
                    Piece victim = board.pieceAt(moveTo(b));
                    Piece attacker = board.pieceAt(moveFrom(b));
                    if (victim != NO_PIECE && attacker != NO_PIECE) {
                        scoreB = MVV_LVA_TABLE[typeOf(victim)][typeOf(attacker)];
                    } else {
                        scoreB = 0;
                    }
                } else {
                    scoreB = 0;
                }
                
                return scoreA > scoreB;  // Higher scores first
            });
    }
    
    // Quiet moves remain at the end in their original order (castling first, etc.)
}

// Template implementation for integrating with existing code
// OPTIMIZED: No heap allocation, in-place sorting
template<typename MoveContainer>
void orderMovesWithMvvLva(const Board& board, MoveContainer& moves) noexcept {
    // Nothing to order if empty or single move
    if (moves.size() <= 1) {
        return;
    }
    
    // Use the optimized in-place implementation
    static MvvLvaOrdering ordering;
    ordering.orderMoves(board, moves);
}

// Explicit instantiation for common container types
template void orderMovesWithMvvLva<MoveList>(const Board& board, MoveList& moves) noexcept;

// Debug helper function to print move ordering (optional)
#ifdef DEBUG_MOVE_ORDERING
void printMoveOrdering(const Board& board, const MoveList& moves) {
    std::cout << "Move Ordering:\n";
    for (Move move : moves) {
        int score = MvvLvaOrdering::scoreMove(board, move);
        std::cout << "  " << moveToString(move) 
                  << " score=" << score;
        
        if (isPromotion(move)) {
            std::cout << " (promotion";
            if (isCapture(move)) std::cout << "-capture";
            std::cout << ")";
        } else if (isEnPassant(move)) {
            std::cout << " (en passant)";
        } else if (isCapture(move)) {
            Square fromSq = moveFrom(move);
            Square toSq = moveTo(move);
            Piece attacker = board.pieceAt(fromSq);
            Piece victim = board.pieceAt(toSq);
            std::cout << " (" << PIECE_CHARS[attacker] 
                      << "x" << PIECE_CHARS[victim] << ")";
        }
        std::cout << "\n";
    }
}
#endif

} // namespace seajay::search