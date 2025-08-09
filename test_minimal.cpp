#include <iostream>
#include "src/core/board.h"

using namespace seajay;

int main() {
    std::cout << "Step 1: Creating Board..." << std::endl;
    Board board;
    std::cout << "Step 2: Board created successfully!" << std::endl;
    
    std::cout << "Step 3: Calling board.clear()..." << std::endl;
    board.clear();
    std::cout << "Step 4: board.clear() completed!" << std::endl;
    
    std::cout << "Step 5: Testing setPiece with A1..." << std::endl;
    board.setPiece(A1, WHITE_ROOK);
    std::cout << "Step 6: setPiece completed!" << std::endl;
    
    std::cout << "All steps completed successfully!" << std::endl;
    return 0;
}