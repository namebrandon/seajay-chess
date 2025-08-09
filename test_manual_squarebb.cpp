#include <iostream>
#include "src/core/board.h"

using namespace seajay;

int main() {
    std::cout << "Creating Board..." << std::endl;
    Board board;
    board.clear();
    std::cout << "Board created!" << std::endl;
    
    // Test squareBB directly
    Square s = A1;
    std::cout << "About to call squareBB(" << static_cast<int>(s) << ")..." << std::endl;
    Bitboard bb = squareBB(s);
    std::cout << "squareBB result: " << bb << std::endl;
    
    // Test if the issue is in context
    Piece p = WHITE_ROOK;
    std::cout << "About to test member array access..." << std::endl;
    
    // We can't access private members directly, so let's see if we can narrow down the issue
    std::cout << "Testing constexpr functions..." << std::endl;
    std::cout << "typeOf(" << static_cast<int>(p) << ") = " << static_cast<int>(typeOf(p)) << std::endl;
    std::cout << "colorOf(" << static_cast<int>(p) << ") = " << static_cast<int>(colorOf(p)) << std::endl;
    std::cout << "isValidSquare(" << static_cast<int>(s) << ") = " << isValidSquare(s) << std::endl;
    
    std::cout << "All basic tests passed. The issue might be in array initialization." << std::endl;
    
    return 0;
}