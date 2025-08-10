// Test specific failing case
#include <iostream>
#include <iomanip>
#include "../src/core/board.h"
#include "../src/core/move_generation.h"
#include "../src/core/move_list.h"
#include "../src/core/bitboard.h"
#include "../src/core/types.h"

using namespace seajay;

void showBitboard(Bitboard bb, const std::string& name) {
    std::cout << "\n" << name << ":\n";
    std::cout << "  Hex: 0x" << std::hex << bb << std::dec << "\n";
    
    for (int rank = 7; rank >= 0; rank--) {
        std::cout << "  " << (rank + 1) << " ";
        for (int file = 0; file < 8; file++) {
            Square sq = static_cast<Square>(rank * 8 + file);
            std::cout << ((bb & squareBB(sq)) ? "1" : ".");
            if (file < 7) std::cout << " ";
        }
        std::cout << "\n";
    }
    std::cout << "    a b c d e f g h\n";
}

int main() {
    // Test the FAILING case from test #2
    std::string fen = "rnbqkbnr/P7/8/8/8/8/8/4K3 w kq - 0 1";
    
    std::cout << "========================================\n";
    std::cout << "Testing failing case #2\n";
    std::cout << "Position: " << fen << "\n";
    std::cout << "Expected: 5 moves (king only)\n";
    std::cout << "========================================\n\n";
    
    Board board;
    if (!board.fromFEN(fen)) {
        std::cerr << "Failed to parse FEN!\n";
        return 1;
    }
    
    std::cout << board.toString() << "\n";
    
    // Check squares
    Square a7 = static_cast<Square>(48);  // White pawn
    Square a8 = static_cast<Square>(56);  // Should be black rook
    Square b8 = static_cast<Square>(57);  // Should be black knight
    
    std::cout << "Piece check:\n";
    std::cout << "  a7: " << static_cast<int>(board.pieceAt(a7)) << " (should be WHITE_PAWN=0)\n";
    std::cout << "  a8: " << static_cast<int>(board.pieceAt(a8)) << " (should be BLACK_ROOK=9)\n";
    std::cout << "  b8: " << static_cast<int>(board.pieceAt(b8)) << " (should be BLACK_KNIGHT=7)\n\n";
    
    // Show bitboards
    Bitboard occupied = board.occupied();
    showBitboard(occupied, "Occupied squares");
    
    // Check if a8 and b8 are marked as occupied
    std::cout << "\nOccupancy check:\n";
    std::cout << "  a8 occupied: " << ((occupied & squareBB(a8)) ? "YES" : "NO") << "\n";
    std::cout << "  b8 occupied: " << ((occupied & squareBB(b8)) ? "YES" : "NO") << "\n";
    
    // Generate moves
    MoveList moves;
    MoveGenerator::generateLegalMoves(board, moves);
    
    std::cout << "\nMoves generated: " << moves.size() << "\n";
    
    int promotions = 0;
    for (size_t i = 0; i < moves.size(); i++) {
        Move move = moves[i];
        if (isPromotion(move)) {
            promotions++;
            std::cout << "  ILLEGAL promotion: " 
                      << squareToString(moveFrom(move)) 
                      << squareToString(moveTo(move)) << "\n";
        }
    }
    
    std::cout << "\nTotal promotions: " << promotions << "\n";
    
    if (promotions > 0) {
        std::cout << "\n✗ BUG CONFIRMED: Pawn can capture b8 knight but NOT a8 (blocked) or move forward!\n";
        return 1;
    }
    
    return 0;
}