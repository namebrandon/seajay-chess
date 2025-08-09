#include <iostream>
#include "src/core/board.h"

using namespace seajay;

int main() {
    // Check if LUT is initialized properly
    std::cout << "Checking PIECE_CHAR_LUT initialization..." << std::endl;
    
    // Create a board to ensure initialization
    Board board;
    
    // Check some key values
    std::cout << "LUT['p'] = " << (int)Board::PIECE_CHAR_LUT['p'] << " (should be " << (int)BLACK_PAWN << ")" << std::endl;
    std::cout << "LUT['P'] = " << (int)Board::PIECE_CHAR_LUT['P'] << " (should be " << (int)WHITE_PAWN << ")" << std::endl;
    std::cout << "LUT['k'] = " << (int)Board::PIECE_CHAR_LUT['k'] << " (should be " << (int)BLACK_KING << ")" << std::endl;
    std::cout << "LUT['K'] = " << (int)Board::PIECE_CHAR_LUT['K'] << " (should be " << (int)WHITE_KING << ")" << std::endl;
    
    // Check numeric characters
    std::cout << "\nChecking numeric characters (should all be NO_PIECE = 12):" << std::endl;
    for (char c = '1'; c <= '8'; c++) {
        std::cout << "LUT['" << c << "'] = " << (int)Board::PIECE_CHAR_LUT[c] << std::endl;
    }
    
    // Check the specific problematic character '2'
    std::cout << "\nSpecific check for '2':" << std::endl;
    std::cout << "LUT['2'] = " << (int)Board::PIECE_CHAR_LUT['2'] << " (should be " << (int)NO_PIECE << " = 12)" << std::endl;
    std::cout << "Is '2' a piece? " << (Board::PIECE_CHAR_LUT['2'] != NO_PIECE ? "YES (BUG!)" : "NO (correct)") << std::endl;
    
    // Now check what happens with the pattern "2pp"
    std::cout << "\nChecking pattern '2pp':" << std::endl;
    for (char c : std::string("2pp")) {
        uint8_t idx = static_cast<uint8_t>(c);
        Piece p = Board::PIECE_CHAR_LUT[idx];
        std::cout << "  '" << c << "' -> LUT[" << (int)idx << "] = " << (int)p;
        if (p == NO_PIECE) {
            std::cout << " (empty/digit)";
        } else {
            std::cout << " (piece: " << PIECE_CHARS[p] << ")";
        }
        std::cout << std::endl;
    }
    
    return 0;
}