#include <iostream>
#include "src/core/board.h"
#include "src/core/move_generation.h"

using namespace seajay;

int main() {
    std::cout << "Minimal move generation test" << std::endl;
    
    // Create board and manually set up a simple position
    Board board;
    
    // Set a white pawn on e2
    board.setPiece(E2, WHITE_PAWN);
    board.setSideToMove(WHITE);
    
    std::cout << "Set up simple position with white pawn on e2" << std::endl;
    std::cout << board.toString() << std::endl;
    
    // Test move generation
    MoveList moves;
    
    std::cout << "About to generate moves..." << std::endl;
    try {
        MoveGenerator::generatePseudoLegalMoves(board, moves);
        std::cout << "Generated " << moves.size() << " moves" << std::endl;
        
        for (size_t i = 0; i < moves.size(); ++i) {
            Move m = moves[i];
            std::cout << "Move " << i << ": " 
                      << squareToString(moveFrom(m)) << squareToString(moveTo(m))
                      << " flags=" << static_cast<int>(moveFlags(m)) << std::endl;
        }
        
    } catch (const std::exception& e) {
        std::cout << "Exception in move generation: " << e.what() << std::endl;
    }
    
    std::cout << "Test completed" << std::endl;
    return 0;
}