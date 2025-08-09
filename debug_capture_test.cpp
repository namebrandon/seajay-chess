#include <iostream>
#include "src/core/board.h"
#include "src/core/move_generation.h"
#include "src/core/move_list.h"

using namespace seajay;

int main() {
    Board board;
    board.fromFEN("rnbqkbnr/pppppppp/8/8/3P4/8/PPP1PPPP/RNBQKBNR b KQkq - 0 1");
    
    std::cout << board.toString() << std::endl;
    
    MoveList moves;
    MoveGenerator::generatePseudoLegalMoves(board, moves);
    
    std::cout << "Total moves: " << moves.size() << std::endl;
    
    int captures = 0;
    for (size_t i = 0; i < moves.size(); ++i) {
        Move m = moves[i];
        if (isCapture(m) && moveTo(m) == D4) {
            captures++;
            std::cout << "Capture to d4: " << squareToString(moveFrom(m)) << squareToString(moveTo(m)) << std::endl;
        }
    }
    
    std::cout << "Total captures to d4: " << captures << std::endl;
    
    // Let's see all captures
    std::cout << "\nAll captures:" << std::endl;
    for (size_t i = 0; i < moves.size(); ++i) {
        Move m = moves[i];
        if (isCapture(m)) {
            std::cout << squareToString(moveFrom(m)) << squareToString(moveTo(m)) << std::endl;
        }
    }
    
    return 0;
}