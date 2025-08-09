#include <iostream>
#include "src/core/board.h"

using namespace seajay;

int main() {
    std::cout << "Starting FEN test...\n";
    
    try {
        Board board;
        std::cout << "Board created successfully\n";
        
        // Test Result type first
        FenResult success = true;
        if (success) {
            std::cout << "Result<T,E> type works\n";
        } else {
            std::cout << "Result<T,E> type failed\n";
            return 1;
        }
        
        // Test new FEN parser
        std::string startFen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
        auto result = board.parseFEN(startFen);
        if (result) {
            std::cout << "FEN parsing successful\n";
            std::cout << "Position hash: " << board.positionHash() << "\n";
            std::cout << "Zobrist key: " << board.zobristKey() << "\n";
        } else {
            std::cout << "FEN parsing failed: " << result.error().message << "\n";
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