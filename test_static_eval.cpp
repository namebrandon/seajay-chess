#include <iostream>
#include "src/core/board.h"
#include "src/evaluation/evaluate.h"

int main() {
    seajay::Board board;
    
    // Test starting position
    board.fromFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    seajay::eval::Score eval = seajay::eval::evaluate(board);
    std::cout << "Starting position (White to move): " << eval.value() << " cp" << std::endl;
    
    // Same position with Black to move
    board.fromFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR b KQkq - 0 1");
    eval = seajay::eval::evaluate(board);
    std::cout << "Starting position (Black to move): " << eval.value() << " cp" << std::endl;
    
    // Just kings
    board.fromFEN("4k3/8/8/8/8/8/8/4K3 w - - 0 1");
    eval = seajay::eval::evaluate(board);
    std::cout << "Just kings (White to move): " << eval.value() << " cp" << std::endl;
    
    return 0;
}
