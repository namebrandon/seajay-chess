#include "src/core/board.h"
#include <iostream>

using namespace seajay;

int main() {
    std::cout << "Testing Board creation" << std::endl;
    
    Board board;
    std::cout << "Board constructor completed" << std::endl;
    
    board.setStartingPosition();
    std::cout << "Starting position set" << std::endl;
    
    std::cout << "Board FEN: " << board.toFEN() << std::endl;
    
    return 0;
}
