#include <iostream>
#include "src/core/board.h"

using namespace seajay;

int main() {
    std::cout << "Starting simple test...\n";
    
    try {
        Board board;
        std::cout << "Board created successfully\n";
        
        board.clear();
        std::cout << "Board cleared successfully\n";
        
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << "\n";
        return 1;
    } catch (...) {
        std::cerr << "Unknown exception\n";
        return 1;
    }
}