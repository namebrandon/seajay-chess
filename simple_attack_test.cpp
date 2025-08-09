#include "src/core/board.h"
#include "src/core/move_generation.h"
#include <iostream>

using namespace seajay;

int main() {
    std::cout << "Creating board..." << std::endl;
    
    Board board;
    std::cout << "Board created successfully" << std::endl;
    
    std::cout << "Setting up starting position..." << std::endl;
    bool result = board.fromFEN("rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR w KQkq - 0 1");
    
    if (!result) {
        std::cerr << "Failed to parse FEN" << std::endl;
        return 1;
    }
    
    std::cout << "FEN parsed successfully" << std::endl;
    
    // Test simple attack
    std::cout << "Testing if D5 is attacked by WHITE..." << std::endl;
    bool attacked = board.isAttacked(D5, WHITE);
    std::cout << "Result: " << (attacked ? "true" : "false") << std::endl;
    
    return 0;
}