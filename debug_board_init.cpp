#include <iostream>
#include "src/core/board.h"

using namespace seajay;

int main() {
    std::cout << "Testing Board constructor..." << std::flush;
    
    try {
        Board board;
        std::cout << " OK\n";
        
        std::cout << "Testing clear..." << std::flush;
        board.clear();
        std::cout << " OK\n";
        
        std::cout << "Testing FEN parsing..." << std::flush;
        auto result = board.parseFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
        
        if (result.hasError()) {
            std::cout << " ERROR: " << result.error().message << "\n";
            return 1;
        } else {
            std::cout << " OK\n";
        }
        
        std::cout << "Testing board display..." << std::flush;
        std::string display = board.toString();
        std::cout << " OK\n";
        
        std::cout << "All tests passed!\n";
        
    } catch (const std::exception& e) {
        std::cout << " EXCEPTION: " << e.what() << "\n";
        return 1;
    } catch (...) {
        std::cout << " UNKNOWN EXCEPTION\n";
        return 1;
    }
    
    return 0;
}