#include "move_ordering.h"
#include <vector>
#include <algorithm>
#include <iostream>

namespace seajay::search {

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
                // Attacker is PAWN, not the promoted piece!
                baseScore += mvvLvaScore(victim, PAWN);
            }
        }
        
        return baseScore;
    }
    
    // Handle en passant captures (special case)
    if (isEnPassant(move)) {
        stats.en_passants_scored++;
        // En passant is always PxP (pawn captures pawn)
        return mvvLvaScore(PAWN, PAWN);
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
        
        return mvvLvaScore(victim, attacker);
    }
    
    // Quiet moves get zero score (will be ordered last)
    stats.quiet_moves++;
    return 0;
}

// Order moves using MVV-LVA scoring
void MvvLvaOrdering::orderMoves(const Board& board, MoveList& moves) const {
    // Nothing to order if empty or single move
    if (moves.size() <= 1) {
        return;
    }
    
    // Create scored moves array
    std::vector<MoveScore> scoredMoves;
    scoredMoves.reserve(moves.size());
    
    // Score each move
    for (Move move : moves) {
        int score = scoreMove(board, move);
        scoredMoves.push_back({move, score});
    }
    
    // Use stable_sort for deterministic ordering
    // When scores are equal, maintains original move order
    // For additional determinism, we could use from-square as tiebreaker
    std::stable_sort(scoredMoves.begin(), scoredMoves.end(), 
        [](const MoveScore& a, const MoveScore& b) {
            if (a.score != b.score) {
                return a.score > b.score;  // Higher scores first
            }
            // Tiebreaker: use from-square for deterministic ordering
            return moveFrom(a.move) < moveFrom(b.move);
        });
    
    // Copy sorted moves back to original container
    moves.clear();
    for (const auto& ms : scoredMoves) {
        moves.add(ms.move);
    }
}

// Template implementation for integrating with existing code
template<typename MoveContainer>
void orderMovesWithMvvLva(const Board& board, MoveContainer& moves) noexcept {
    // Nothing to order if empty or single move
    if (moves.size() <= 1) {
        return;
    }
    
    // Create temporary array for scoring
    std::vector<MoveScore> scoredMoves;
    scoredMoves.reserve(moves.size());
    
    // Score each move
    for (auto it = moves.begin(); it != moves.end(); ++it) {
        Move move = *it;
        int score = MvvLvaOrdering::scoreMove(board, move);
        scoredMoves.push_back({move, score});
    }
    
    // Use stable_sort for deterministic ordering
    std::stable_sort(scoredMoves.begin(), scoredMoves.end());
    
    // Copy sorted moves back
    auto writeIt = moves.begin();
    for (const auto& ms : scoredMoves) {
        *writeIt++ = ms.move;
    }
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