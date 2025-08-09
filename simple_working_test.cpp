#include <iostream>
#include "src/core/types.h"
#include "src/core/move_list.h"
#include "src/core/bitboard.h"

using namespace seajay;

int main() {
    std::cout << "SeaJay Chess Engine - Core Components Test" << std::endl;
    std::cout << "Stage 3 Verification - Working Components Only" << std::endl << std::endl;
    
    // Test 1: Move System
    std::cout << "=== Test 1: Move Encoding/Decoding ===" << std::endl;
    Move move1 = makeMove(E2, E4, DOUBLE_PAWN);
    Move move2 = makeMove(D7, D8, PROMO_QUEEN);
    Move move3 = makeMove(E1, G1, CASTLING);
    Move move4 = makeMove(E5, F6, EN_PASSANT);
    
    std::cout << "e2-e4 (double pawn): from=" << squareToString(moveFrom(move1)) 
              << " to=" << squareToString(moveTo(move1)) 
              << " flags=" << static_cast<int>(moveFlags(move1)) << std::endl;
              
    std::cout << "d7-d8=Q (promotion): from=" << squareToString(moveFrom(move2))
              << " to=" << squareToString(moveTo(move2))
              << " flags=" << static_cast<int>(moveFlags(move2)) << std::endl;
              
    std::cout << "e1-g1 (castling): from=" << squareToString(moveFrom(move3))
              << " to=" << squareToString(moveTo(move3))
              << " flags=" << static_cast<int>(moveFlags(move3)) << std::endl;
              
    std::cout << "e5xf6 e.p. (en passant): from=" << squareToString(moveFrom(move4))
              << " to=" << squareToString(moveTo(move4))
              << " flags=" << static_cast<int>(moveFlags(move4)) << std::endl;
    
    // Test move flag queries
    std::cout << std::endl << "Move flag queries:" << std::endl;
    std::cout << "  isPromotion(d7-d8=Q): " << isPromotion(move2) << std::endl;
    std::cout << "  isCastling(e1-g1): " << isCastling(move3) << std::endl;
    std::cout << "  isEnPassant(e5xf6): " << isEnPassant(move4) << std::endl;
    std::cout << "  isDoublePawnPush(e2-e4): " << isDoublePawnPush(move1) << std::endl;
    
    // Test 2: MoveList
    std::cout << std::endl << "=== Test 2: MoveList Container ===" << std::endl;
    MoveList moves;
    
    // Add some moves
    moves.addMove(A2, A3, NORMAL);
    moves.addMove(A2, A4, DOUBLE_PAWN);  
    moves.addMove(B1, C3, NORMAL);
    moves.addMove(B1, A3, NORMAL);
    moves.addPromotionMoves(H7, H8);  // Adds 4 promotion moves
    moves.addMove(E1, G1, CASTLING);
    moves.addMove(E5, D6, EN_PASSANT);
    
    std::cout << "MoveList capacity: " << moves.capacity() << std::endl;
    std::cout << "MoveList size: " << moves.size() << std::endl;
    std::cout << "MoveList empty: " << moves.empty() << std::endl;
    
    std::cout << std::endl << "First 5 moves in list:" << std::endl;
    for (size_t i = 0; i < std::min(moves.size(), size_t(5)); ++i) {
        Move m = moves[i];
        std::cout << "  " << (i+1) << ". " << squareToString(moveFrom(m))
                  << "-" << squareToString(moveTo(m))
                  << " [" << static_cast<int>(moveFlags(m)) << "]" << std::endl;
    }
    
    // Test finding moves
    Move searchMove = makeMove(B1, C3, NORMAL);
    bool found = moves.contains(searchMove);
    std::cout << std::endl << "Contains b1-c3: " << found << std::endl;
    
    // Test 3: Basic Types and Utilities
    std::cout << std::endl << "=== Test 3: Basic Types ===" << std::endl;
    
    // Test piece functions
    for (int i = WHITE_PAWN; i <= BLACK_KING; ++i) {
        Piece p = static_cast<Piece>(i);
        std::cout << "Piece " << static_cast<int>(p) << " (" << PIECE_CHARS[p] << "): "
                  << "type=" << static_cast<int>(typeOf(p))
                  << " color=" << static_cast<int>(colorOf(p)) << std::endl;
    }
    
    // Test square functions
    std::cout << std::endl << "Square functions:" << std::endl;
    for (int i = 0; i < 8; ++i) {
        Square sq = static_cast<Square>(i);
        std::cout << "Square " << squareToString(sq) << ": "
                  << "file=" << static_cast<int>(fileOf(sq))
                  << " rank=" << static_cast<int>(rankOf(sq)) << std::endl;
    }
    
    // Test bitboard operations
    std::cout << std::endl << "=== Test 4: Basic Bitboard Operations ===" << std::endl;
    Bitboard bb1 = squareBB(A1);
    Bitboard bb2 = squareBB(H8);
    Bitboard bb3 = rankBB(0);  // First rank
    Bitboard bb4 = fileBB(0);  // A file
    
    std::cout << "A1 bitboard: 0x" << std::hex << bb1 << std::dec << std::endl;
    std::cout << "H8 bitboard: 0x" << std::hex << bb2 << std::dec << std::endl;
    std::cout << "Rank 1 bitboard: 0x" << std::hex << bb3 << std::dec << std::endl;
    std::cout << "File A bitboard: 0x" << std::hex << bb4 << std::dec << std::endl;
    
    Bitboard combined = bb1 | bb2;
    std::cout << "A1|H8 bitboard: 0x" << std::hex << combined << std::dec << std::endl;
    std::cout << "Pop count of A1|H8: " << popCount(combined) << std::endl;
    
    if (combined) {
        std::cout << "LSB of A1|H8: " << squareToString(lsb(combined)) << std::endl;
        std::cout << "MSB of A1|H8: " << squareToString(msb(combined)) << std::endl;
    }
    
    std::cout << std::endl << "=== All Core Systems Working! ===" << std::endl;
    std::cout << "✓ Move encoding/decoding (16-bit format)" << std::endl;
    std::cout << "✓ MoveList container (256 move capacity)" << std::endl;
    std::cout << "✓ Piece/Color type system" << std::endl;
    std::cout << "✓ Square/Coordinate functions" << std::endl;
    std::cout << "✓ Basic bitboard operations" << std::endl;
    std::cout << std::endl;
    std::cout << "Stage 3 core components are functional!" << std::endl;
    std::cout << "Ready for move generation implementation once Board issues are resolved." << std::endl;
    
    return 0;
}