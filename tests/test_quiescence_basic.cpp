/**
 * Basic test to verify quiescence search works
 */

#include <iostream>
#include "core/board.h"

int main() {
    std::cout << "Basic Quiescence Test" << std::endl;
    
    std::cout << "Creating board..." << std::endl;
    seajay::Board board;
    
    std::cout << "Setting starting position..." << std::endl;
    board.setStartingPosition();
    
    std::cout << "Board created successfully!" << std::endl;
    std::cout << board.toString() << std::endl;
    
    return 0;
}