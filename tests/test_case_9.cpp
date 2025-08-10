// Test case #9: White pawn a7 can capture rook a8 OR knight b8
#include <iostream>
#include <iomanip>
#include "../src/core/board.h"
#include "../src/core/move_generation.h"
#include "../src/core/move_list.h"
#include "../src/core/bitboard.h"
#include "../src/core/types.h"

using namespace seajay;

int main() {
    // Test case #9: White pawn a7 can capture rook a8 OR knight b8
    std::string fen = "rn2k3/P7/8/8/8/8/8/4K3 w - - 0 1";
    
    std::cout << "========================================\n";
    std::cout << "Testing case #9\n";
    std::cout << "Position: " << fen << "\n";
    std::cout << "Expected: 13 moves (5 king + 8 promotions)\n";
    std::cout << "Pawn can capture a8 (4 promos) AND b8 (4 promos)\n";
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
    
    // Generate moves
    MoveList moves;
    MoveGenerator::generateLegalMoves(board, moves);
    
    std::cout << "Moves generated: " << moves.size() << "\n\n";
    
    std::cout << "All moves:\n";
    int promoToA8 = 0, promoToB8 = 0;
    
    for (size_t i = 0; i < moves.size(); i++) {
        Move move = moves[i];
        Square from = moveFrom(move);
        Square to = moveTo(move);
        
        std::cout << "  " << squareToString(from) << squareToString(to);
        
        if (isPromotion(move)) {
            std::cout << " [PROMOTION]";
            if (from == a7 && to == a8) promoToA8++;
            if (from == a7 && to == b8) promoToB8++;
        }
        std::cout << "\n";
    }
    
    std::cout << "\nPromotion moves to a8: " << promoToA8 << " (expected: 4)\n";
    std::cout << "Promotion moves to b8: " << promoToB8 << " (expected: 4)\n";
    
    if (promoToA8 != 4 || promoToB8 != 4) {
        std::cout << "\n✗ BUG: Missing promotion captures!\n";
        return 1;
    }
    
    std::cout << "\n✓ Correct: All promotion captures generated.\n";
    return 0;
}