#include <iostream>
#include "src/core/board.h"

using namespace seajay;

int main() {
    std::cout << "Super minimal test" << std::endl;
    
    Board board;
    std::cout << "Board created" << std::endl;
    
    // Try setting side to move
    board.setSideToMove(WHITE);
    std::cout << "Set side to move" << std::endl;
    
    // Try getting side to move
    Color side = board.sideToMove();
    std::cout << "Side to move: " << (side == WHITE ? "White" : "Black") << std::endl;
    
    // Try setting a piece without calling toString
    std::cout << "About to set piece..." << std::endl;
    board.setPiece(E2, WHITE_PAWN);
    std::cout << "Piece set successfully" << std::endl;
    
    // Get the piece back
    Piece p = board.pieceAt(E2);
    std::cout << "Piece at E2: " << static_cast<int>(p) << std::endl;
    
    std::cout << "Test completed" << std::endl;
    return 0;
}