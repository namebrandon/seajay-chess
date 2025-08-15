#include "../../src/core/board.h"
#include "../../src/core/see.h"
#include <iostream>

using namespace seajay;

int main() {
    Board board;
    board.fromFEN("R6r/8/8/2r5/8/8/8/K6k b - - 0 1");
    
    std::cout << board.toString() << "\n";
    
    std::cout << "Position analysis:\n";
    std::cout << "White Rook at a8\n";
    std::cout << "Black Rook at h8\n";
    std::cout << "Black Rook at c5\n";
    std::cout << "White Rook at a5? NO - there's no rook at a5\n";
    
    std::cout << "\nMove: Rc5xa5 (Black to move)\n";
    std::cout << "But there's no piece at a5 to capture!\n";
    std::cout << "This test is invalid.\n";
    
    // Let's try the intended test with correct FEN
    std::cout << "\n=== Corrected position ===\n";
    board.fromFEN("R6r/8/8/R1r5/8/8/8/K6k b - - 0 1");  // Add white rook at a5
    std::cout << board.toString() << "\n";
    
    std::cout << "Now: Rc5xa5 (black rook takes white rook)\n";
    std::cout << "White Ra8 can recapture (x-ray through a5)\n";
    std::cout << "Black Rh8 cannot help (wrong file)\n";
    
    Move capture = makeMove(C5, A5);
    SEEValue value = see(board, capture);
    std::cout << "SEE value: " << value << " (expected 0 - equal trade)\n";
    
    return 0;
}