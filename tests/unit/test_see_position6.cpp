#include "../../src/core/board.h"
#include "../../src/core/see.h"
#include <iostream>

using namespace seajay;

int main() {
    Board board;
    board.fromFEN("4k3/8/8/2q5/8/8/8/B3K3 w - - 0 1");
    
    std::cout << board.toString() << "\n";
    
    std::cout << "Position analysis:\n";
    std::cout << "White Bishop at a1\n";
    std::cout << "Black Queen at c5\n";
    std::cout << "Move: Ba1-c3\n";
    std::cout << "There's no piece at c3, so this is not a capture\n";
    std::cout << "But the queen can capture the bishop after it moves to c3\n";
    std::cout << "This is a blunder - bishop hangs\n";
    
    Move move = makeMove(A1, C3);
    SEEValue value = see(board, move);
    std::cout << "SEE value: " << value << "\n";
    std::cout << "Expected: 0 (no capture), Got: " << value << "\n";
    std::cout << "\nNote: SEE evaluates captures. For non-captures, it should return 0.\n";
    
    return 0;
}