#pragma once

#include "../core/types.h"
#include "../core/board.h"  // Need full Board definition for pieceAt()
#include <cstring>

namespace seajay {

class CounterMoves {
public:
    static constexpr int DEFAULT_BONUS = 12000;  // Between killers (16k) and good history (8k)
    
    CounterMoves() { 
        clear(); 
    }
    
    void clear() {
        std::memset(m_counters, 0, sizeof(m_counters));
    }
    
    // CRITICAL FIX: Use piece-type + to-square indexing to avoid collisions
    // This matches Stockfish/Ethereal approach and prevents different pieces
    // moving to the same square from overwriting each other's countermoves
    void update(const Board& board, Move prevMove, Move counterMove) {
        if (prevMove == NO_MOVE) {
            return;
        }
        
        if (isCapture(counterMove) || isPromotion(counterMove)) {
            return;
        }
        
        // Get the piece that made the previous move (now at 'to' square)
        const Square to = moveTo(prevMove);
        const Piece piece = board.pieceAt(to);
        if (piece == NO_PIECE) {
            return;  // Shouldn't happen but be safe
        }
        
        // Index by [piece_type][to_square] for better distribution
        // This prevents Nc3-e4 from overwriting Bc3-e4's countermove
        const PieceType pt = typeOf(piece);
        m_counters[pt][to] = counterMove;
    }
    
    Move getCounterMove(const Board& board, Move prevMove) const {
        if (prevMove == NO_MOVE) {
            return NO_MOVE;
        }
        
        const Square to = moveTo(prevMove);
        const Piece piece = board.pieceAt(to);
        if (piece == NO_PIECE) {
            return NO_MOVE;
        }
        
        const PieceType pt = typeOf(piece);
        return m_counters[pt][to];
    }
    
    bool hasCounterMove(const Board& board, Move prevMove) const {
        return getCounterMove(board, prevMove) != NO_MOVE;
    }

private:
    // Index by [piece_type][to_square] - 6 piece types × 64 squares
    // This gives much better distribution than [from][to] which has 4096 entries
    // but many collisions in practice
    alignas(64) Move m_counters[6][64];
};

} // namespace seajay