#include <iostream>
#include "src/core/types.h"

using namespace seajay;

int main() {
    std::cout << "Debug piece test" << std::endl;
    
    Piece p = WHITE_PAWN;
    std::cout << "WHITE_PAWN = " << static_cast<int>(p) << std::endl;
    std::cout << "NUM_PIECES = " << NUM_PIECES << std::endl;
    
    PieceType pt = typeOf(p);
    std::cout << "typeOf(WHITE_PAWN) = " << static_cast<int>(pt) << std::endl;
    std::cout << "NUM_PIECE_TYPES = " << NUM_PIECE_TYPES << std::endl;
    
    Color c = colorOf(p);
    std::cout << "colorOf(WHITE_PAWN) = " << static_cast<int>(c) << std::endl;
    std::cout << "NUM_COLORS = " << NUM_COLORS << std::endl;
    
    // Check if piece value is valid
    if (p <= NO_PIECE) {
        std::cout << "Piece value is within bounds" << std::endl;
    } else {
        std::cout << "ERROR: Piece value is out of bounds!" << std::endl;
    }
    
    std::cout << "Test completed" << std::endl;
    return 0;
}