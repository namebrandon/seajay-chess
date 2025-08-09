#include "src/core/board.h"
#include "src/core/move_generation.h"
#include <iostream>

using namespace seajay;

int main() {
    std::cout << "Creating empty board..." << std::endl;
    Board board;
    
    // We need to manually set up a simple position to avoid FEN parsing issues
    // Let's create the most basic test possible
    std::cout << "Testing basic functionality..." << std::endl;
    
    // Test the starting position
    std::cout << "Testing starting position..." << std::endl;
    bool result = board.fromFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    
    if (!result) {
        std::cerr << "Failed to set up starting position" << std::endl;
        return 1;
    }
    
    std::cout << "Starting position loaded successfully" << std::endl;
    
    // Test one simple attack - e4 square attacked by d2 pawn moving to d4
    board.fromFEN("rnbqkbnr/pppppppp/8/8/3P4/8/PPP1PPPP/RNBQKBNR w KQkq - 0 1");
    
    std::cout << "Testing if e5 is attacked by WHITE (should be attacked by d4 pawn)..." << std::endl;
    bool attacked = board.isAttacked(E5, WHITE);
    std::cout << "e5 attacked by WHITE: " << (attacked ? "YES" : "NO") << std::endl;
    
    std::cout << "Testing if c5 is attacked by WHITE (should be attacked by d4 pawn)..." << std::endl;
    attacked = board.isAttacked(C5, WHITE);
    std::cout << "c5 attacked by WHITE: " << (attacked ? "YES" : "NO") << std::endl;
    
    std::cout << "Testing if d5 is attacked by WHITE (should NOT be attacked by d4 pawn)..." << std::endl;
    attacked = board.isAttacked(D5, WHITE);
    std::cout << "d5 attacked by WHITE: " << (attacked ? "YES" : "NO") << std::endl;
    
    std::cout << "Basic attack tests completed!" << std::endl;
    return 0;
}