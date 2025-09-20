#include <iostream>
#include <string>
#include "../src/core/board.h"
#include "../src/core/move_generation.h"
#include "../src/core/move_list.h"

using namespace seajay;

int main() {
    std::string fen = "k7/8/8/8/8/8/8/R6K b - - 0 1";
    Board board;
    board.fromFEN(fen);
    
    std::cout << board.toString() << "\n";
    
    MoveList moves;
    MoveGenerator::generateLegalMoves(board, moves);
    
    std::cout << "Legal moves:\n";
    for (size_t i = 0; i < moves.size(); ++i) {
        Move move = moves[i];
        std::cout << squareToString(moveFrom(move)) << "->" 
                 << squareToString(moveTo(move)) << "\n";
    }
    
    return 0;
}