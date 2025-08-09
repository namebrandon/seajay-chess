#include <iostream>
#include "src/core/board.h"
#include "src/core/move_generation.h"
#include "src/core/move_list.h"

using namespace seajay;

int main() {
    Board board;
    board.fromFEN("4k3/3P4/8/8/8/8/3p4/4K3 w - - 0 1");
    
    std::cout << board.toString() << std::endl;
    
    MoveList moves;
    MoveGenerator::generatePseudoLegalMoves(board, moves);
    
    std::cout << "Total moves: " << moves.size() << std::endl;
    
    for (size_t i = 0; i < moves.size(); ++i) {
        Move m = moves[i];
        std::cout << squareToString(moveFrom(m)) << squareToString(moveTo(m));
        if (isPromotion(m)) {
            std::cout << " PROMOTION type=" << static_cast<int>(promotionType(m));
        }
        if (isCapture(m)) {
            std::cout << " CAPTURE";
        }
        std::cout << " flags=" << static_cast<int>(moveFlags(m)) << std::endl;
    }
    
    return 0;
}