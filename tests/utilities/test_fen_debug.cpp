#include <iostream>
#include <string>
#include "../src/core/board.h"

using namespace seajay;

int main() {
    std::string fen = "4k3/8/8/8/8/8/8/4R3 b - - 0 1";
    std::cout << "Testing FEN: " << fen << std::endl;
    
    Board board;
    std::cout << "Calling fromFEN..." << std::endl;
    bool result = board.fromFEN(fen);
    std::cout << "Result: " << (result ? "SUCCESS" : "FAILURE") << std::endl;
    
    if (result) {
        std::cout << "Board after parsing:" << std::endl;
        std::cout << board.toString() << std::endl;
    }
    
    // Try parseFEN directly
    std::cout << "\nCalling parseFEN directly..." << std::endl;
    FenResult parseResult = board.parseFEN(fen);
    if (parseResult.hasValue()) {
        std::cout << "parseFEN succeeded" << std::endl;
    } else {
        std::cout << "parseFEN failed: " << parseResult.error().message << std::endl;
    }
    
    return 0;
}