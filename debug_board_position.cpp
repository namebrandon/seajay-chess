#include <iostream>
#include <vector>
#include "src/core/board.h"

using namespace seajay;

int main() {
    std::cout << "Creating board..." << std::endl;
    Board board;
    
    // Clear the board first
    board.clear();
    std::cout << "Board cleared!" << std::endl;
    
    // Test LUT manually
    std::cout << "Testing piece lookup table..." << std::endl;
    std::cout << "LUT['r'] = " << static_cast<int>(board.PIECE_CHAR_LUT['r']) << " (should be " << static_cast<int>(BLACK_ROOK) << ")" << std::endl;
    std::cout << "LUT['n'] = " << static_cast<int>(board.PIECE_CHAR_LUT['n']) << " (should be " << static_cast<int>(BLACK_KNIGHT) << ")" << std::endl;
    std::cout << "LUT['P'] = " << static_cast<int>(board.PIECE_CHAR_LUT['P']) << " (should be " << static_cast<int>(WHITE_PAWN) << ")" << std::endl;
    std::cout << "NO_PIECE = " << static_cast<int>(NO_PIECE) << std::endl;
    
    // Test simple board position parsing manually
    std::string boardStr = "8/8/8/8/8/8/8/R7";  // Just one white rook
    std::cout << "Testing simple board: " << boardStr << std::endl;
    
    // Parse manually to see what happens
    int sq = 56;  // A8
    int rank = 7;
    int file = 0;
    
    for (char c : boardStr) {
        std::cout << "Processing char '" << c << "' at position " << sq << " (rank=" << rank << ", file=" << file << ")" << std::endl;
        
        if (c == '/') {
            std::cout << "  Rank separator" << std::endl;
            if (file != 8) {
                std::cout << "  ERROR: Incomplete rank! file=" << file << std::endl;
            }
            rank--;
            file = 0;
            sq = rank * 8;
        } else if (c >= '1' && c <= '8') {
            int skip = c - '0';
            std::cout << "  Empty squares: " << skip << std::endl;
            file += skip;
            sq += skip;
        } else if (board.PIECE_CHAR_LUT[static_cast<uint8_t>(c)] != NO_PIECE) {
            Piece p = board.PIECE_CHAR_LUT[static_cast<uint8_t>(c)];
            std::cout << "  Piece: " << static_cast<int>(p) << std::endl;
            file++;
            sq++;
        } else {
            std::cout << "  ERROR: Invalid character!" << std::endl;
            break;
        }
        
        if (file > 8) {
            std::cout << "  ERROR: File overflow!" << std::endl;
            break;
        }
    }
    
    std::cout << "Manual parsing completed." << std::endl;
    return 0;
}