#include <iostream>
#include "src/core/board.h"
#include "src/core/move_generation.h"

using namespace seajay;

int main() {
    std::cout << "Simple move generation test" << std::endl;
    
    Board board;
    board.setStartingPosition();
    
    std::cout << "Board created and set to starting position" << std::endl;
    std::cout << board.toString() << std::endl;
    
    std::cout << "Testing pawn attacks for white..." << std::endl;
    
    // Let's test just a simple pawn attack generation
    MoveList moves;
    std::cout << "About to generate pseudo-legal moves..." << std::endl;
    
    try {
        // Let's just test the table initialization first
        std::cout << "Initializing attack tables..." << std::endl;
        // This should happen automatically in generatePseudoLegalMoves
        
        MoveGenerator::generatePseudoLegalMoves(board, moves);
        std::cout << "Generated " << moves.size() << " pseudo-legal moves" << std::endl;
        
        if (moves.size() > 0) {
            std::cout << "First few moves: ";
            for (size_t i = 0; i < std::min(moves.size(), size_t(5)); ++i) {
                Move m = moves[i];
                std::cout << squareToString(moveFrom(m)) << squareToString(moveTo(m)) << " ";
            }
            std::cout << std::endl;
        }
        
    } catch (const std::exception& e) {
        std::cout << "Exception: " << e.what() << std::endl;
        return 1;
    }
    
    std::cout << "Test completed successfully" << std::endl;
    return 0;
}