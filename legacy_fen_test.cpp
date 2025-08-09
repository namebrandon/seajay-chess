#include <iostream>
#include "src/core/board.h"

using namespace seajay;

int main() {
    std::cout << "Starting legacy FEN test...\n";
    
    try {
        Board board;
        std::cout << "Board created successfully\n";
        
        // Test legacy FEN parser
        std::string startFen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
        if (board.fromFEN(startFen)) {
            std::cout << "Legacy FEN parsing successful\n";
            std::cout << "Position hash: " << board.positionHash() << "\n";
            std::cout << "Zobrist key: " << board.zobristKey() << "\n";
            
            // Test validation
            if (board.validatePosition()) {
                std::cout << "Position validation passed\n";
            } else {
                std::cout << "Position validation failed\n";
            }
            
        } else {
            std::cout << "Legacy FEN parsing failed\n";
            return 1;
        }
        
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << "\n";
        return 1;
    } catch (...) {
        std::cerr << "Unknown exception\n";
        return 1;
    }
}