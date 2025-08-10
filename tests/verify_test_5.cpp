// Verify test case #5 expectations
#include <iostream>
#include "../src/core/board.h"
#include "../src/core/types.h"

using namespace seajay;

int main() {
    std::string fen = "b3k3/1P6/8/8/8/8/8/4K3 w - - 0 1";
    // Note: FEN starts with '1b2...' which means a8 is empty, b8 has black bishop
    
    std::cout << "Analyzing FEN: " << fen << "\n";
    std::cout << "First rank 8 part: 'b3k3'\n";
    std::cout << "This means:\n";
    std::cout << "  a8: black bishop\n";
    std::cout << "  b8: empty\n";
    std::cout << "  c8: empty\n";
    std::cout << "  d8: empty\n";
    std::cout << "  e8: black king\n";
    std::cout << "  f8-h8: empty\n\n";
    
    Board board;
    board.fromFEN(fen);
    
    std::cout << board.toString() << "\n";
    
    Square a8 = static_cast<Square>(56);
    Square b8 = static_cast<Square>(57);
    Square c8 = static_cast<Square>(58);
    
    std::cout << "Actual pieces:\n";
    std::cout << "  a8: " << static_cast<int>(board.pieceAt(a8)) 
              << " (8=BLACK_BISHOP, 12=NO_PIECE)\n";
    std::cout << "  b8: " << static_cast<int>(board.pieceAt(b8)) 
              << " (8=BLACK_BISHOP, 12=NO_PIECE)\n";
    std::cout << "  c8: " << static_cast<int>(board.pieceAt(c8)) 
              << " (8=BLACK_BISHOP, 12=NO_PIECE)\n";
    
    return 0;
}