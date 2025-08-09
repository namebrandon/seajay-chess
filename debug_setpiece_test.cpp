#include <iostream>
#include "src/core/board.h"

using namespace seajay;

int main() {
    std::cout << "Debug setPiece test" << std::endl;
    
    Board board;
    std::cout << "Board created" << std::endl;
    
    Square s = E2;
    Piece p = WHITE_PAWN;
    
    std::cout << "About to call setPiece with s=" << static_cast<int>(s) 
              << " p=" << static_cast<int>(p) << std::endl;
    
    // Check the condition that setPiece checks
    std::cout << "isValidSquare(s): " << isValidSquare(s) << std::endl;
    std::cout << "p > NO_PIECE: " << (p > NO_PIECE) << std::endl;
    std::cout << "NO_PIECE value: " << static_cast<int>(NO_PIECE) << std::endl;
    
    if (!isValidSquare(s) || p > NO_PIECE) {
        std::cout << "setPiece would return early" << std::endl;
        return 0;
    }
    
    std::cout << "About to access mailbox..." << std::endl;
    Piece oldPiece = board.pieceAt(s);  // This calls m_mailbox[s]
    std::cout << "oldPiece = " << static_cast<int>(oldPiece) << std::endl;
    
    std::cout << "Test completed without calling setPiece" << std::endl;
    return 0;
}