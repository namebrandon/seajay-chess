#include <iostream>
#include "src/core/board.h"

using namespace seajay;

int main() {
    std::cout << "Step 1: Creating Board..." << std::endl;
    Board board;
    std::cout << "Step 2: Board created!" << std::endl;
    
    std::cout << "Step 3: Testing direct mailbox access..." << std::endl;
    // Access the mailbox directly without calling setPiece
    // board.m_mailbox[0] = WHITE_ROOK;  // This won't work, m_mailbox is private
    
    std::cout << "Step 4: Testing simple function calls..." << std::endl;
    Square sq = A1;
    Piece p = WHITE_ROOK;
    std::cout << "Square: " << static_cast<int>(sq) << std::endl;
    std::cout << "Piece: " << static_cast<int>(p) << std::endl;
    std::cout << "isValidSquare: " << isValidSquare(sq) << std::endl;
    std::cout << "typeOf: " << static_cast<int>(typeOf(p)) << std::endl;
    std::cout << "colorOf: " << static_cast<int>(colorOf(p)) << std::endl;
    
    std::cout << "Step 5: Testing squareBB..." << std::endl;
    Bitboard bb = squareBB(sq);
    std::cout << "squareBB result: " << bb << std::endl;
    
    std::cout << "All basic function tests passed!" << std::endl;
    return 0;
}