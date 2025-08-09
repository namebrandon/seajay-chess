#include <iostream>
#include "src/core/board.h"

using namespace seajay;

int main() {
    std::cout << "Creating Board..." << std::endl;
    Board board;
    board.clear();
    std::cout << "Board created and cleared!" << std::endl;
    
    // Test the individual components of setPiece manually
    Square s = A1;
    Piece p = WHITE_ROOK;
    
    std::cout << "Testing setPiece logic step by step..." << std::endl;
    std::cout << "Square: " << static_cast<int>(s) << ", Piece: " << static_cast<int>(p) << std::endl;
    
    // Check the conditions
    std::cout << "isValidSquare(s): " << isValidSquare(s) << std::endl;
    std::cout << "p > NO_PIECE: " << (p > NO_PIECE) << std::endl;
    std::cout << "Should continue: " << (!isValidSquare(s) || p > NO_PIECE) << std::endl;
    
    if (!isValidSquare(s) || p > NO_PIECE) {
        std::cout << "Would return early due to invalid parameters" << std::endl;
        return 1;
    }
    
    std::cout << "Parameters are valid, would call pieceAt..." << std::endl;
    Piece oldPiece = board.pieceAt(s);
    std::cout << "pieceAt returned: " << static_cast<int>(oldPiece) << std::endl;
    
    if (oldPiece != NO_PIECE) {
        std::cout << "Would call updateBitboards(remove) and updateZobristKey for old piece" << std::endl;
        // Don't actually call these, just check
    }
    
    std::cout << "Would set mailbox..." << std::endl;
    // board.m_mailbox[s] = p;  // Can't access private member directly
    
    if (p != NO_PIECE) {
        std::cout << "Would call updateBitboards(add) and updateZobristKey for new piece" << std::endl;
        
        // Let's test if the Zobrist access would hang
        std::cout << "Testing Zobrist initialization check..." << std::endl;
        
        // Can't access private members directly, so let's just see if the full setPiece hangs
        std::cout << "About to call actual setPiece..." << std::endl;
        board.setPiece(s, p);
        std::cout << "setPiece completed successfully!" << std::endl;
    }
    
    std::cout << "Test completed!" << std::endl;
    return 0;
}