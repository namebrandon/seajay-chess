#pragma once

#include "../core/board.h"
#include "../core/types.h"

namespace seajay::search {

/**
 * @brief Detect if a move creates a discovered check
 * 
 * A discovered check occurs when moving a piece uncovers an attack
 * from another piece to the enemy king.
 * 
 * @param board The current board position
 * @param move The move to check
 * @return true if the move creates a discovered check
 */
inline bool isDiscoveredCheck(const Board& board, Move move) {
    // Quick check: if not a capture, less likely to be discovered check
    // (though non-captures can also create discovered checks)
    
    Square fromSq = from(move);
    Square toSq = to(move);
    Piece movingPiece = board.pieceAt(fromSq);
    
    if (movingPiece == NO_PIECE) return false;
    
    Color us = colorOf(movingPiece);
    Color them = ~us;
    Square theirKing = board.kingSquare(them);
    
    // Check if the piece moving away from fromSq could uncover an attack
    // This is a simplified check - full implementation would check sliding pieces
    
    // Check if there's a sliding piece behind the moving piece that could
    // attack the enemy king once the piece moves
    
    // For now, return a simple heuristic: pieces moving away from the 
    // line between our back rank and enemy king might create discovered checks
    
    int fromRank = rankOf(fromSq);
    int kingRank = rankOf(theirKing);
    int fromFile = fileOf(fromSq);
    int kingFile = fileOf(theirKing);
    
    // Simple heuristic: if moving piece is between our back rank and enemy king
    // on same file/rank/diagonal, it might create discovered check
    bool sameFile = (fromFile == kingFile);
    bool sameRank = (fromRank == kingRank);
    bool sameDiag = (abs(fromFile - kingFile) == abs(fromRank - kingRank));
    
    return (sameFile || sameRank || sameDiag) && (toSq != theirKing);
}

} // namespace seajay::search