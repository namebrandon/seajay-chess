#include <iostream>
#include <string>
#include "../src/core/board.h"

using namespace seajay;

int main() {
    // Test our problematic FEN by building it step by step
    Board board;
    board.clear();
    
    // Manually set up the position
    board.setPiece(E8, BLACK_KING);
    board.setPiece(E1, WHITE_KING);  // Add white king to pass validation
    board.setPiece(E1, WHITE_ROOK);  // Place rook
    board.setSideToMove(BLACK);
    
    std::cout << "Manually constructed board:" << std::endl;
    std::cout << board.toString() << std::endl;
    
    // Now try a FEN with both kings
    std::string testFen = "4k3/8/8/8/8/8/8/4RK2 b - - 0 1";
    std::cout << "\nTesting FEN with both kings: " << testFen << std::endl;
    Board board2;
    if (board2.fromFEN(testFen)) {
        std::cout << "SUCCESS - FEN parsed correctly" << std::endl;
        std::cout << board2.toString() << std::endl;
    } else {
        std::cout << "FAILED - FEN failed to parse" << std::endl;
        
        // Try parsing directly
        FenResult result = board2.parseFEN(testFen);
        if (!result.hasValue()) {
            std::cout << "Parse error: " << result.error().message << std::endl;
        }
    }
    
    // Try the original with white king too
    testFen = "4k3/8/8/8/8/8/8/3KR3 b - - 0 1";
    std::cout << "\nTesting FEN with white king at d1: " << testFen << std::endl;
    Board board3;
    if (board3.fromFEN(testFen)) {
        std::cout << "SUCCESS - FEN parsed correctly" << std::endl;
        std::cout << board3.toString() << std::endl;
    } else {
        std::cout << "FAILED - FEN failed to parse" << std::endl;
    }
    
    return 0;
}