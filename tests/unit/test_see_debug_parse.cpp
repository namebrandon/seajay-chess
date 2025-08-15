#include "../../src/core/board.h"
#include "../../src/core/see.h"
#include <iostream>

using namespace seajay;

int main() {
    Board board;
    board.fromFEN("1k2r3/1pp4p/p7/4p3/8/P5P1/1PP4P/2K1R3 w - - 0 1");
    
    std::cout << board.toString() << "\n";
    
    // The move e1e5 should be Rook takes pawn
    Move move = makeMove(E1, E5);
    
    std::cout << "Move from E1 to E5\n";
    std::cout << "Piece at E1: " << (board.pieceAt(E1) == WHITE_ROOK ? "White Rook" : "?") << "\n";
    std::cout << "Piece at E5: " << (board.pieceAt(E5) == BLACK_PAWN ? "Black Pawn" : "?") << "\n";
    
    SEEValue value = see(board, move);
    std::cout << "SEE value: " << value << "\n";
    std::cout << "Expected: -400 (Rook takes pawn, rook x-ray recaptures)\n";
    
    return 0;
}