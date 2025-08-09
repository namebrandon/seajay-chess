#include <iostream>
#include "src/core/board.h"

using namespace seajay;

int main() {
    Board board;
    std::string fen = "rnbqkbnr/ppp1pppp/8/2p1p3/3P4/8/PPP1PPPP/RNBQKBNR b KQkq - 0 1";
    
    std::cout << "Setting FEN: " << fen << std::endl;
    bool success = board.fromFEN(fen);
    
    std::cout << "fromFEN returned: " << (success ? "true" : "false") << std::endl;
    std::cout << "\nBoard state:" << std::endl;
    std::cout << board.toString() << std::endl;
    
    // Try the new parseFEN interface
    std::cout << "\nTrying parseFEN interface:" << std::endl;
    Board board2;
    auto result = board2.parseFEN(fen);
    if (result.hasValue()) {
        std::cout << "parseFEN succeeded" << std::endl;
        std::cout << board2.toString() << std::endl;
    } else {
        std::cout << "parseFEN failed: " << result.error().message << std::endl;
    }
    
    return 0;
}