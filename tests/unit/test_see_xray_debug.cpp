#include "../../src/core/see.h"
#include "../../src/core/board.h"
#include "../../src/core/move_generation.h"
#include <iostream>
#include <string>

using namespace seajay;

void debugSEE(const std::string& fen, Move move) {
    Board board;
    if (!board.fromFEN(fen)) {
        std::cerr << "Failed to parse FEN\n";
        return;
    }
    
    std::cout << "=== DEBUG SEE ===\n";
    std::cout << "FEN: " << fen << "\n";
    std::cout << "Move: " << squareToString(moveFrom(move)) 
              << squareToString(moveTo(move)) << "\n";
    
    // Display board
    std::cout << "\nBoard:\n" << board.toString() << "\n";
    
    // Call SEE
    SEEValue value = see(board, move);
    std::cout << "SEE Value: " << value << "\n";
}

int main() {
    // Test 1: Simple rook x-ray
    std::cout << "Test 1: Rook takes pawn with x-ray\n";
    std::cout << "Expected: -400 (Rook-Pawn = 500-100 = 400, then rook x-ray recaptures = -400)\n";
    debugSEE("1k1r4/1pp4p/p7/4p3/8/P5P1/1PP4P/2K1R3 w - - 0 1", makeMove(E1, E5));
    
    return 0;
}