// Test case #5: White pawn b7 blocked by black bishop b8
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
    // Test case #5: White pawn b7 blocked by black bishop b8
    std::string fen = "1b2k3/1P6/8/8/8/8/8/4K3 w - - 0 1";
    
    std::cout << "========================================\n";
    std::cout << "Testing case #5\n";
    std::cout << "Position: " << fen << "\n";
    std::cout << "Expected: 5 moves (king only), pawn CANNOT capture on a8 or c8\n";
    std::cout << "========================================\n\n";
    
    Board board;
    if (!board.fromFEN(fen)) {
        std::cerr << "Failed to parse FEN!\n";
        return 1;
    }
    
    std::cout << board.toString() << "\n";
    
    // Check squares
    Square b7 = static_cast<Square>(49);  // White pawn
    Square a8 = static_cast<Square>(56);  // Should be empty
    Square b8 = static_cast<Square>(57);  // Should be black bishop
    Square c8 = static_cast<Square>(58);  // Should be empty
    
    std::cout << "Piece check:\n";
    std::cout << "  b7: " << static_cast<int>(board.pieceAt(b7)) << " (should be WHITE_PAWN=0)\n";
    std::cout << "  a8: " << static_cast<int>(board.pieceAt(a8)) << " (should be NO_PIECE=12)\n";
    std::cout << "  b8: " << static_cast<int>(board.pieceAt(b8)) << " (should be BLACK_BISHOP=8)\n";
    std::cout << "  c8: " << static_cast<int>(board.pieceAt(c8)) << " (should be NO_PIECE=12)\n\n";
    
    // Show bitboards
    Bitboard occupied = board.occupied();
    Bitboard blackPieces = board.pieces(BLACK);
    showBitboard(occupied, "Occupied squares");
    showBitboard(blackPieces, "Black pieces");
    
    // Check occupancy
    std::cout << "\nOccupancy check:\n";
    std::cout << "  a8 occupied: " << ((occupied & squareBB(a8)) ? "YES" : "NO") << " (should be NO)\n";
    std::cout << "  b8 occupied: " << ((occupied & squareBB(b8)) ? "YES" : "NO") << " (should be YES)\n";
    std::cout << "  c8 occupied: " << ((occupied & squareBB(c8)) ? "YES" : "NO") << " (should be NO)\n";
    
    // Generate moves
    MoveList moves;
    MoveGenerator::generateLegalMoves(board, moves);
    
    std::cout << "\nMoves generated: " << moves.size() << "\n";
    
    std::cout << "\nAll moves:\n";
    for (size_t i = 0; i < moves.size(); i++) {
        Move move = moves[i];
        std::cout << "  " << squareToString(moveFrom(move)) 
                  << squareToString(moveTo(move));
        if (isPromotion(move)) {
            std::cout << " [PROMOTION]";
        }
        std::cout << "\n";
    }
    
    int promotions = 0;
    for (size_t i = 0; i < moves.size(); i++) {
        Move move = moves[i];
        if (isPromotion(move)) {
            promotions++;
            Square to = moveTo(move);
            if (to == a8 || to == c8) {
                std::cout << "\n✗ ILLEGAL: Promotion to EMPTY square " 
                          << squareToString(to) << "!\n";
            }
        }
    }
    
    std::cout << "\nTotal promotions: " << promotions << "\n";
    
    if (promotions > 4) {
        std::cout << "\n✗ BUG CONFIRMED: Generating illegal promotions!\n";
        std::cout << "Pawn on b7 can ONLY capture b8 (4 promotion types).\n";
        std::cout << "Should NOT move to a8 or c8 (empty squares).\n";
        return 1;
    }
    
    return 0;
}