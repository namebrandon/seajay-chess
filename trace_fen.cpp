#include <iostream>
#include "src/core/board.h"
#include "src/core/bitboard.h"

using namespace seajay;

int main() {
    // Let's trace through what happens when parseFEN is called
    std::string testFen = "rnbqkbnr/ppp1pppp/8/2p1p3/3P4/8/PPP1PPPP/RNBQKBNR b KQkq - 0 1";
    
    std::cout << "Testing FEN: " << testFen << std::endl;
    
    // Try with parseFEN (the new interface)
    Board board1;
    auto result = board1.parseFEN(testFen);
    
    if (result.hasValue()) {
        std::cout << "parseFEN succeeded!" << std::endl;
        std::cout << board1.toString() << std::endl;
    } else {
        std::cout << "parseFEN failed: " << result.error().message << std::endl;
        std::cout << "Error code: " << static_cast<int>(result.error().error) << std::endl;
        
        // The board should still be in its pre-parse state since we use temp board
        std::cout << "\nBoard state after failed parse:" << std::endl;
        std::cout << board1.toString() << std::endl;
    }
    
    // Try with fromFEN (the legacy interface)
    std::cout << "\n=== Testing with fromFEN ===" << std::endl;
    Board board2;
    bool success = board2.fromFEN(testFen);
    
    std::cout << "fromFEN returned: " << (success ? "true" : "false") << std::endl;
    std::cout << "\nBoard state:" << std::endl;
    std::cout << board2.toString() << std::endl;
    
    // Let's test the starting position
    std::cout << "\n=== Testing starting position ===" << std::endl;
    Board board3;
    board3.setStartingPosition();
    std::cout << board3.toString() << std::endl;
    
    // Now let's set to our test position a different way
    std::cout << "\n=== Testing partial FEN (just the pieces) ===" << std::endl;
    Board board4;
    // Try a simpler version first
    success = board4.fromFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    std::cout << "Starting position fromFEN: " << (success ? "SUCCESS" : "FAIL") << std::endl;
    
    if (success) {
        // Now try with d4 pawn
        Board board5;
        success = board5.fromFEN("rnbqkbnr/pppppppp/8/8/3P4/8/PPP1PPPP/RNBQKBNR b KQkq - 0 1");
        std::cout << "After 1.d4 fromFEN: " << (success ? "SUCCESS" : "FAIL") << std::endl;
        
        if (success) {
            // Now with c5 added
            Board board6;
            success = board6.fromFEN("rnbqkbnr/pp1ppppp/8/2p5/3P4/8/PPP1PPPP/RNBQKBNR w KQkq - 0 1");
            std::cout << "After 1.d4 c5 fromFEN: " << (success ? "SUCCESS" : "FAIL") << std::endl;
            
            if (success) {
                // Finally the full position
                Board board7;
                success = board7.fromFEN("rnbqkbnr/ppp1pppp/8/2p1p3/3P4/8/PPP1PPPP/RNBQKBNR b KQkq - 0 1");
                std::cout << "After 1.d4 c5 2.? e5 fromFEN: " << (success ? "SUCCESS" : "FAIL") << std::endl;
                
                if (!success) {
                    std::cout << "Failed at final position!" << std::endl;
                }
            }
        }
    }
    
    return 0;
}