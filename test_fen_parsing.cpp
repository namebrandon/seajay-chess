#include <iostream>
#include "src/core/board.h"

using namespace seajay;

int main() {
    std::cout << "FEN test starting..." << std::endl;
    
    try {
        Board board;
        std::cout << "Board created successfully" << std::endl;
        
        std::cout << "About to parse FEN..." << std::endl;
        auto result = board.parseFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
        std::cout << "FEN parsed, checking result..." << std::endl;
        
        if (result.hasValue()) {
            std::cout << "FEN parsing successful!" << std::endl;
        } else {
            std::cout << "FEN parsing failed: " << result.error().message << std::endl;
        }
        
    } catch (const std::exception& e) {
        std::cout << "Exception: " << e.what() << std::endl;
        return 1;
    }
    
    std::cout << "FEN test completed" << std::endl;
    return 0;
}