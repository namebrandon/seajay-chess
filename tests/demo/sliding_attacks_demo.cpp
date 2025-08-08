#include "../../src/core/board.h"
#include "../../src/core/bitboard.h"
#include <iostream>

using namespace seajay;

void demonstrateRookAttacks() {
    std::cout << "\n=== Rook Attack Generation Demo ===\n\n";
    
    Board board;
    board.clear();
    
    // Place rook on e4
    Square rookSq = makeSquare(4, 3); // e4
    board.setPiece(rookSq, WHITE_ROOK);
    
    // Add some blocking pieces
    board.setPiece(makeSquare(4, 6), BLACK_PAWN);  // e7 - blocks north
    board.setPiece(makeSquare(2, 3), BLACK_KNIGHT); // c4 - blocks west
    board.setPiece(makeSquare(4, 1), WHITE_PAWN);   // e2 - blocks south
    
    std::cout << "Position with White Rook on e4:\n";
    std::cout << board.toString() << "\n";
    
    // Generate rook attacks
    Bitboard attacks = rookAttacks(rookSq, board.occupied());
    
    std::cout << "Rook attacks from e4 (X marks attack squares):\n";
    std::cout << bitboardToString(attacks) << "\n";
}

void demonstrateBishopAttacks() {
    std::cout << "\n=== Bishop Attack Generation Demo ===\n\n";
    
    Board board;
    board.clear();
    
    // Place bishop on d4
    Square bishopSq = makeSquare(3, 3); // d4
    board.setPiece(bishopSq, WHITE_BISHOP);
    
    // Add some blocking pieces
    board.setPiece(makeSquare(1, 1), BLACK_PAWN);   // b2 - blocks southwest
    board.setPiece(makeSquare(6, 6), BLACK_ROOK);   // g7 - blocks northeast
    
    std::cout << "Position with White Bishop on d4:\n";
    std::cout << board.toString() << "\n";
    
    // Generate bishop attacks
    Bitboard attacks = bishopAttacks(bishopSq, board.occupied());
    
    std::cout << "Bishop attacks from d4 (X marks attack squares):\n";
    std::cout << bitboardToString(attacks) << "\n";
}

void demonstrateQueenAttacks() {
    std::cout << "\n=== Queen Attack Generation Demo ===\n\n";
    
    Board board;
    board.clear();
    
    // Place queen on d4
    Square queenSq = makeSquare(3, 3); // d4
    board.setPiece(queenSq, WHITE_QUEEN);
    
    // Add blocking pieces in different directions
    board.setPiece(makeSquare(3, 6), BLACK_KNIGHT); // d7 - blocks north
    board.setPiece(makeSquare(1, 3), BLACK_PAWN);   // b4 - blocks west
    board.setPiece(makeSquare(6, 6), BLACK_ROOK);   // g7 - blocks northeast
    
    std::cout << "Position with White Queen on d4:\n";
    std::cout << board.toString() << "\n";
    
    // Generate queen attacks
    Bitboard attacks = queenAttacks(queenSq, board.occupied());
    
    std::cout << "Queen attacks from d4 (X marks attack squares):\n";
    std::cout << bitboardToString(attacks) << "\n";
}

int main() {
    std::cout << "\n🏰 SeaJay Chess Engine - Sliding Piece Attack Generation Demo\n";
    std::cout << "============================================================\n";
    std::cout << "\nThis demo shows the ray-based sliding piece attack generation\n";
    std::cout << "implemented for Phase 1 Stage 1. These functions will be replaced\n";
    std::cout << "with magic bitboards in Phase 3 for better performance.\n";
    
    demonstrateRookAttacks();
    demonstrateBishopAttacks(); 
    demonstrateQueenAttacks();
    
    std::cout << "\n✅ Demo completed!\n";
    std::cout << "\nNote: This is a temporary implementation using ray-based generation.\n";
    std::cout << "Magic bitboards will provide much faster attack generation in Phase 3.\n\n";
    
    return 0;
}