#include "src/core/board.h"
#include "src/core/move_generation.h"
#include <iostream>

using namespace seajay;

int main() {
    std::cout << "Simple Legal Move Test" << std::endl;
    
    try {
        Board board;
        board.setStartingPosition();
        std::cout << "Board initialized successfully" << std::endl;
        
        // Test basic pseudo-legal move generation first
        MoveList pseudoMoves;
        MoveGenerator::generatePseudoLegalMoves(board, pseudoMoves);
        std::cout << "Pseudo-legal moves: " << pseudoMoves.size() << std::endl;
        
        // Test if isAttacked works
        Square e1 = E1;
        bool attacked = MoveGenerator::isSquareAttacked(board, e1, BLACK);
        std::cout << "E1 attacked by black: " << (attacked ? "yes" : "no") << std::endl;
        
        // Test basic legal move generation
        MoveList legalMoves;
        MoveGenerator::generateLegalMoves(board, legalMoves);
        std::cout << "Legal moves: " << legalMoves.size() << std::endl;
        
        std::cout << "Test completed successfully" << std::endl;
        
    } catch (const std::exception& e) {
        std::cout << "Exception: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cout << "Unknown exception occurred" << std::endl;
        return 1;
    }
    
    return 0;
}
