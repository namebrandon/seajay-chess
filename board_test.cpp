#include <iostream>
#include "src/core/board.h"

using namespace seajay;

int main() {
    std::cout << "Board test starting..." << std::endl;
    
    try {
        std::cout << "About to create Board..." << std::endl;
        Board board;
        std::cout << "Board created successfully" << std::endl;
        
        std::cout << "About to set starting position..." << std::endl;
        board.setStartingPosition();
        std::cout << "Starting position set successfully" << std::endl;
        
        std::cout << "Getting side to move: " << (board.sideToMove() == WHITE ? "White" : "Black") << std::endl;
        
    } catch (const std::exception& e) {
        std::cout << "Exception: " << e.what() << std::endl;
        return 1;
    }
    
    std::cout << "Board test completed successfully" << std::endl;
    return 0;
}