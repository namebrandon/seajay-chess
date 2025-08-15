#include "../../src/core/board.h"
#include "../../src/core/see.h"
#include <iostream>

using namespace seajay;

int main() {
    Board board;
    board.fromFEN("r3k3/8/8/8/R7/8/8/4K3 w - - 0 1");
    
    std::cout << board.toString() << "\n";
    
    std::cout << "Position analysis:\n";
    std::cout << "White Rook at a4\n";
    std::cout << "Black Rook at a8\n";
    std::cout << "Black King at e8\n";
    
    std::cout << "\nMove: Ra4xa8 (White to move)\n";
    std::cout << "Rook takes rook: +500\n";
    std::cout << "King cannot recapture (too far)\n";
    std::cout << "This is a win of material, not equal trade\n";
    
    Move capture = makeMove(A4, A8);
    SEEValue value = see(board, capture);
    std::cout << "\nSEE value: " << value << " (expected +500 - win a rook)\n";
    
    return 0;
}