#include <iostream>
#include "src/core/board.h"

using namespace seajay;

int main() {
    std::cout << "Zobrist test" << std::endl;
    
    // Try to create a board object, but this time use gdb to see where it hangs
    std::cout << "About to create Board object..." << std::endl;
    
    Board* board = new Board();
    
    std::cout << "Board object created successfully" << std::endl;
    
    delete board;
    std::cout << "Test completed" << std::endl;
    return 0;
}