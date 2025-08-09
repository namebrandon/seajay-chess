#include <iostream>
#include "src/core/board.h"

using namespace seajay;

int main() {
    std::cout << "Constructor test" << std::endl;
    
    {
        std::cout << "About to create Board" << std::endl;
        Board board;
        std::cout << "Board created, about to exit scope" << std::endl;
    }
    
    std::cout << "Exited scope, Board destroyed" << std::endl;
    return 0;
}