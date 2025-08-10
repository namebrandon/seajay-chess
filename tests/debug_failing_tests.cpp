// Debug the 2 failing test cases
#include <iostream>
#include "../src/core/board.h"
#include "../src/core/move_generation.h"
#include "../src/core/move_list.h"
#include "../src/core/types.h"

using namespace seajay;

void debugPosition(const std::string& fen, const std::string& description) {
    std::cout << "\n========================================\n";
    std::cout << description << "\n";
    std::cout << "FEN: " << fen << "\n";
    std::cout << "========================================\n\n";
    
    Board board;
    board.fromFEN(fen);
    
    std::cout << board.toString() << "\n";
    
    // Generate all legal moves
    MoveList moves;
    MoveGenerator::generateLegalMoves(board, moves);
    
    std::cout << "Legal moves (" << moves.size() << "):\n";
    for (size_t i = 0; i < moves.size(); i++) {
        Move move = moves[i];
        std::cout << "  " << squareToString(moveFrom(move)) 
                  << squareToString(moveTo(move));
        if (isPromotion(move)) {
            std::cout << " [PROMOTION]";
        }
        std::cout << "\n";
    }
}

int main() {
    // Test #2: Expected 9, got 7
    debugPosition("rnbqkbnr/P7/8/8/8/8/8/4K3 w kq - 0 1",
                  "Test #2: Pawn a7 with full black back rank");
    
    std::cout << "\nAnalysis:\n";
    std::cout << "Pawn on a7 can capture knight on b8 (4 promotions)\n";
    std::cout << "King on e1 can move to d1, f1, d2, e2, f2 (5 moves)\n";
    std::cout << "Expected total: 9 moves\n";
    std::cout << "Actually got: 7 moves\n";
    std::cout << "Missing: 2 king moves (probably d1 and f1 due to castling rights?)\n\n";
    
    // Test #4: Expected 9, got 5
    debugPosition("n3k3/P7/8/8/8/8/8/4K3 w - - 0 1",
                  "Test #4: Pawn a7, knight a8 blocks forward");
    
    std::cout << "\nAnalysis:\n";
    std::cout << "Knight is on a8, not b8!\n";
    std::cout << "Pawn on a7 cannot:\n";
    std::cout << "  - Move forward (blocked by knight)\n";
    std::cout << "  - Capture a8 (straight ahead, not diagonal)\n";
    std::cout << "  - Capture b8 (empty square)\n";
    std::cout << "So only 5 king moves are correct!\n";
    
    return 0;
}