#include <iostream>
#include "src/core/types.h"

using namespace seajay;

int main() {
    std::cout << "Absolute minimal test" << std::endl;
    
    // Test basic Square functionality
    Square s = E2;
    std::cout << "Square E2 = " << static_cast<int>(s) << std::endl;
    
    // Test isValidSquare
    bool valid = isValidSquare(s);
    std::cout << "E2 is valid: " << valid << std::endl;
    
    // Test squareBB
    Bitboard bb = squareBB(s);
    std::cout << "SquareBB(E2) = " << bb << std::endl;
    
    // Test piece creation
    Piece p = WHITE_PAWN;
    std::cout << "WHITE_PAWN = " << static_cast<int>(p) << std::endl;
    
    std::cout << "Basic type test completed successfully" << std::endl;
    return 0;
}