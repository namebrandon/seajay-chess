// Test case #8: White pawn e7 with e8 empty
#include <iostream>
#include "../src/core/board.h"
#include "../src/core/move_generation.h"
#include "../src/core/move_list.h"
#include "../src/core/types.h"

using namespace seajay;

int main() {
    std::string fen = "4k3/4P3/8/8/8/8/8/4K3 w - - 0 1";
    
    std::cout << "Testing case #8\n";
    std::cout << "Position: " << fen << "\n\n";
    
    Board board;
    board.fromFEN(fen);
    
    std::cout << board.toString() << "\n";
    
    Square e7 = static_cast<Square>(52); // e7
    Square e8 = static_cast<Square>(60); // e8
    Square d8 = static_cast<Square>(59); // d8
    Square f8 = static_cast<Square>(61); // f8
    
    std::cout << "Piece check:\n";
    std::cout << "  e7: " << static_cast<int>(board.pieceAt(e7)) << " (should be 0=WHITE_PAWN)\n";
    std::cout << "  d8: " << static_cast<int>(board.pieceAt(d8)) << " (12=NO_PIECE)\n";
    std::cout << "  e8: " << static_cast<int>(board.pieceAt(e8)) << " (should be 11=BLACK_KING)\n";
    std::cout << "  f8: " << static_cast<int>(board.pieceAt(f8)) << " (12=NO_PIECE)\n\n";
    
    // Check if e8 is blocked
    Bitboard occupied = board.occupied();
    std::cout << "e8 occupied: " << ((occupied & squareBB(e8)) ? "YES" : "NO") << "\n\n";
    
    MoveList moves;
    MoveGenerator::generateLegalMoves(board, moves);
    
    std::cout << "Moves generated: " << moves.size() << "\n";
    std::cout << "All moves:\n";
    
    for (size_t i = 0; i < moves.size(); i++) {
        Move move = moves[i];
        std::cout << "  " << squareToString(moveFrom(move)) 
                  << squareToString(moveTo(move));
        if (isPromotion(move)) {
            std::cout << " [PROMOTION]";
        }
        std::cout << "\n";
    }
    
    std::cout << "\nAnalysis:\n";
    std::cout << "The pawn on e7 is blocked by the black king on e8.\n";
    std::cout << "It cannot move forward or capture (no enemies on d8/f8).\n";
    std::cout << "Expected: 5 king moves only.\n";
    
    return 0;
}