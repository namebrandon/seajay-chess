#include <iostream>
#include <vector>
#include "src/core/board.h"

using namespace seajay;

int main() {
    std::cout << "Testing Board constructor..." << std::endl;
    
    Board board;
    std::cout << "Board created successfully!" << std::endl;
    
    // std::cout << "Zobrist initialized: " << (board.s_zobristInitialized ? "YES" : "NO") << std::endl;
    std::cout << "LUT initialized: " << (board.s_lutInitialized ? "YES" : "NO") << std::endl;
    
    // Test individual FEN parsing steps
    std::string testFen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
    
    std::cout << "Testing FEN tokenization..." << std::endl;
    
    // Parse manually to find the exact step that hangs
    std::vector<std::string> tokens;
    size_t start = 0;
    size_t end = 0;
    
    while (end < testFen.length()) {
        if (testFen[end] == ' ') {
            if (start < end) {
                tokens.push_back(testFen.substr(start, end - start));
            }
            start = end + 1;
        }
        end++;
    }
    
    if (start < end) {
        tokens.push_back(testFen.substr(start, end - start));
    }
    
    std::cout << "Found " << tokens.size() << " tokens:" << std::endl;
    for (size_t i = 0; i < tokens.size(); ++i) {
        std::cout << "  " << i << ": '" << tokens[i] << "'" << std::endl;
    }
    
    // Test board position parsing step by step
    std::cout << "Testing board position parsing: '" << tokens[0] << "'" << std::endl;
    
    // Test character by character
    std::string boardStr = tokens[0];
    std::cout << "Board string length: " << boardStr.length() << std::endl;
    
    for (size_t i = 0; i < boardStr.length() && i < 10; ++i) {
        char c = boardStr[i];
        std::cout << "  Char " << i << ": '" << c << "' (ASCII " << static_cast<int>(c) << ")";
        
        if (c >= '1' && c <= '8') {
            std::cout << " -> empty squares: " << (c - '0');
        } else if (c == '/') {
            std::cout << " -> rank separator";
        } else {
            Piece p = board.PIECE_CHAR_LUT[static_cast<uint8_t>(c)];
            std::cout << " -> piece: " << static_cast<int>(p);
        }
        std::cout << std::endl;
    }
    
    return 0;
}