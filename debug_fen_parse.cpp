#include <iostream>
#include <vector>
#include "src/core/board.h"

using namespace seajay;

int main() {
    std::cout << "Creating board..." << std::endl;
    Board board;
    std::cout << "Board created!" << std::endl;
    
    // Skip direct parseBoardPosition test since it's private
    
    std::cout << "Testing full FEN parsing..." << std::endl;
    std::string testFen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
    
    std::cout << "About to call parseFEN..." << std::endl;
    auto fenResult = board.parseFEN(testFen);
    
    if (fenResult.hasError()) {
        std::cout << "Error in parseFEN: " << fenResult.error().message << std::endl;
        return 1;
    } else {
        std::cout << "parseFEN succeeded!" << std::endl;
    }
    
    return 0;
}