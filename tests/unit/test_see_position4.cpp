#include "../../src/core/board.h"
#include "../../src/core/see.h"
#include <iostream>

using namespace seajay;

int main() {
    Board board;
    board.fromFEN("4k3/8/4r3/4P3/8/8/4Q3/4K3 w - - 0 1");
    
    std::cout << board.toString() << "\n";
    
    std::cout << "Position analysis:\n";
    std::cout << "White Queen at e2\n";
    std::cout << "White Pawn at e5\n";
    std::cout << "Black Rook at e6\n";
    
    std::cout << "\nMove: Qe2-e5\n";
    std::cout << "This is NOT a capture (e5 has a white pawn)\n";
    std::cout << "Can't move queen to a square occupied by own pawn!\n";
    
    std::cout << "\nThis test case is invalid - queen can't capture own pawn.\n";
    
    return 0;
}