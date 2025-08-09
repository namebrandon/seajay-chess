#include "move_list.h"
#include "board.h"
#include <sstream>
#include <iostream>
#include <algorithm>

namespace seajay {

std::string MoveList::toString() const {
    std::ostringstream oss;
    oss << "[";
    
    for (size_t i = 0; i < size(); ++i) {
        if (i > 0) oss << ", ";
        
        Move move = m_moves[i];
        Square from = moveFrom(move);
        Square to = moveTo(move);
        uint8_t flags = moveFlags(move);
        
        oss << squareToString(from) << squareToString(to);
        
        // Add flag information
        if (isPromotion(move)) {
            switch (promotionType(move)) {
                case KNIGHT: oss << "n"; break;
                case BISHOP: oss << "b"; break;
                case ROOK: oss << "r"; break;
                case QUEEN: oss << "q"; break;
                default: oss << "?"; break;
            }
        } else if (flags != NORMAL) {
            switch (flags) {
                case DOUBLE_PAWN: oss << "*"; break;
                case CASTLING: oss << "#"; break;
                case EN_PASSANT: oss << "ep"; break;
                default: oss << "(" << static_cast<int>(flags) << ")"; break;
            }
        }
    }
    
    oss << "]";
    return oss.str();
}

std::string MoveList::toAlgebraicNotation(const Board& board) const {
    // TODO: Implement full algebraic notation
    // For Stage 3, we'll provide a basic implementation
    std::ostringstream oss;
    oss << "{";
    
    for (size_t i = 0; i < size(); ++i) {
        if (i > 0) oss << ", ";
        
        Move move = m_moves[i];
        Square from = moveFrom(move);
        Square to = moveTo(move);
        Piece piece = board.pieceAt(from);
        
        // Basic algebraic notation (will be enhanced in later stages)
        if (typeOf(piece) != PAWN) {
            switch (typeOf(piece)) {
                case KNIGHT: oss << "N"; break;
                case BISHOP: oss << "B"; break;
                case ROOK: oss << "R"; break;
                case QUEEN: oss << "Q"; break;
                case KING: oss << "K"; break;
                default: break;
            }
        }
        
        oss << squareToString(from) << "-" << squareToString(to);
        
        if (isPromotion(move)) {
            oss << "=";
            switch (promotionType(move)) {
                case KNIGHT: oss << "N"; break;
                case BISHOP: oss << "B"; break;
                case ROOK: oss << "R"; break;
                case QUEEN: oss << "Q"; break;
                default: break;
            }
        }
    }
    
    oss << "}";
    return oss.str();
}

std::ostream& operator<<(std::ostream& os, const MoveList& moves) {
    os << moves.toString();
    return os;
}

} // namespace seajay