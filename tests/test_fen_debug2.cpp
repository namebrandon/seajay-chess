#include <iostream>
#include <string>
#include "../src/core/board.h"
#include "../src/core/move_generation.h"
#include "../src/core/move_list.h"

using namespace seajay;

int main() {
    // Test a simpler known working FEN first
    std::string testFen1 = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
    std::cout << "Testing standard starting position:" << std::endl;
    Board board1;
    if (board1.fromFEN(testFen1)) {
        std::cout << "SUCCESS - Starting position parsed correctly" << std::endl;
    } else {
        std::cout << "FAILED - Starting position failed to parse" << std::endl;
    }
    
    // Now test our problematic FEN
    std::string testFen2 = "4k3/8/8/8/8/8/8/4R3 b - - 0 1";
    std::cout << "\nTesting problematic FEN: " << testFen2 << std::endl;
    Board board2;
    
    // Try a fresh board
    board2.clear();
    if (board2.fromFEN(testFen2)) {
        std::cout << "SUCCESS - FEN parsed correctly" << std::endl;
        std::cout << board2.toString() << std::endl;
        
        // Generate legal moves to test
        MoveList moves;
        MoveGenerator::generateLegalMoves(board2, moves);
        std::cout << "Generated " << moves.size() << " legal moves" << std::endl;
        
        // Check if specific moves are generated
        for (size_t i = 0; i < moves.size(); ++i) {
            Move move = moves[i];
            Square from = moveFrom(move);
            Square to = moveTo(move);
            if (from == E8) {  // King moves
                std::cout << "King move: " << squareToString(from) << " -> " << squareToString(to) << std::endl;
            }
        }
    } else {
        std::cout << "FAILED - FEN failed to parse" << std::endl;
        
        // Let's parse it step by step
        FenResult result = board2.parseFEN(testFen2);
        if (!result.hasValue()) {
            std::cout << "Parse error: " << result.error().message << std::endl;
        }
    }
    
    return 0;
}