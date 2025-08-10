#include "evaluate.h"
#include "material.h"
#include "pst.h"  // Stage 9: Include PST header
#include "../core/board.h"
#include "../core/bitboard.h"

namespace seajay::eval {

Score evaluate(const Board& board) {
    // Get material from board
    const Material& material = board.material();
    
    // Check for insufficient material draws
    if (material.isInsufficientMaterial()) {
        return Score::draw();
    }
    
    // Check for same-colored bishops (KB vs KB)
    // This is a special case where we need board position info
    if (material.count(WHITE, BISHOP) == 1 && 
        material.count(BLACK, BISHOP) == 1 &&
        material.count(WHITE, PAWN) == 0 && 
        material.count(BLACK, PAWN) == 0 &&
        material.count(WHITE, KNIGHT) == 0 && 
        material.count(BLACK, KNIGHT) == 0 &&
        material.count(WHITE, ROOK) == 0 && 
        material.count(BLACK, ROOK) == 0 &&
        material.count(WHITE, QUEEN) == 0 && 
        material.count(BLACK, QUEEN) == 0) {
        
        // Find the bishops and check if they're on same color squares
        Bitboard whiteBishops = board.pieces(WHITE, BISHOP);
        Bitboard blackBishops = board.pieces(BLACK, BISHOP);
        
        if (whiteBishops && blackBishops) {
            Square wbSq = lsb(whiteBishops);
            Square bbSq = lsb(blackBishops);
            
            // Check if both bishops are on same color squares
            // (rank + file) % 2 == 0 means dark square
            bool wbDark = ((wbSq / 8) + (wbSq % 8)) % 2 == 0;
            bool bbDark = ((bbSq / 8) + (bbSq % 8)) % 2 == 0;
            
            if (wbDark == bbDark) {
                return Score::draw();  // Same colored bishops = draw
            }
        }
    }
    
    // Get PST score (Stage 9)
    // PST score is stored from white's perspective in the board
    const MgEgScore& pstScore = board.pstScore();
    
    // For Stage 9, we only use middlegame PST values (no tapering yet)
    Score pstValue = pstScore.mg;
    
    // Calculate total evaluation from white's perspective
    // Material difference + PST score
    Score materialDiff = material.value(WHITE) - material.value(BLACK);
    Score totalWhite = materialDiff + pstValue;
    
    // Return from side-to-move perspective
    if (board.sideToMove() == WHITE) {
        return totalWhite;
    } else {
        return -totalWhite;
    }
}

#ifdef DEBUG
bool verifyMaterialIncremental(const Board& board) {
    // Recount material from scratch
    Material scratch;
    
    for (Square s = A1; s <= H8; ++s) {
        Piece p = board.pieceAt(s);
        if (p != NO_PIECE) {
            scratch.add(p);
        }
    }
    
    // Compare with board's incremental material
    return scratch == board.material();
}
#endif

} // namespace seajay::eval