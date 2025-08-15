#include "../../src/core/board.h"
#include <iostream>

using namespace seajay;

int main() {
    Board board;
    board.fromFEN("1k1r4/1pp4p/p7/4p3/8/P5P1/1PP4P/2K1R3 w - - 0 1");
    
    std::cout << board.toString() << "\n";
    
    // Check exact pieces
    std::cout << "White rook at: e1\n";
    std::cout << "Black pawn at: e5\n";
    std::cout << "Black rook at: d8\n";
    
    std::cout << "\nAfter Re1xe5:\n";
    std::cout << "White rook would be at e5\n";
    std::cout << "Black rook at d8 - can it reach e5? NO! (different file AND rank)\n";
    std::cout << "\nThis test case seems wrong! A rook on d8 cannot capture on e5.\n";
    
    return 0;
}