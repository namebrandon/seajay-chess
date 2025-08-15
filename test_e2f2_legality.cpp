#include <iostream>
#include "src/core/board.h"
#include "src/core/move_generation.h"
#include "src/core/types.h"

using namespace seajay;

int main() {
    std::cout << "============================================\n";
    std::cout << "Testing e2f2 and e2f3 Legality\n";
    std::cout << "============================================\n\n";
    
    // Set up the exact position from move 97
    Board board;
    board.fromFEN("7k/6p1/7p/3p1p2/8/2q5/4K3/1R6 w - - 0 49");
    
    std::cout << "Position after 97 moves:\n";
    std::cout << board.toString() << "\n";
    std::cout << "FEN: " << board.toFEN() << "\n\n";
    
    // Generate legal moves
    MoveList legalMoves;
    MoveGenerator::generateLegalMoves(board, legalMoves);
    
    std::cout << "Total legal moves: " << legalMoves.size() << "\n\n";
    
    // Find the white king
    Square whiteKing = board.kingSquare(WHITE);
    Square f2 = makeSquare(File(5), Rank(1));  // f2
    Square f3 = makeSquare(File(5), Rank(2));  // f3
    
    std::cout << "White King: " << squareToString(whiteKing) << "\n";
    std::cout << "Checking f2 and f3:\n";
    std::cout << "  f2 attacked: " << (MoveGenerator::isSquareAttacked(board, f2, BLACK) ? "YES" : "NO") << "\n";
    std::cout << "  f3 attacked: " << (MoveGenerator::isSquareAttacked(board, f3, BLACK) ? "YES" : "NO") << "\n\n";
    
    // Check if e2f2 and e2f3 are in legal moves
    bool hasE2F2 = false;
    bool hasE2F3 = false;
    
    std::cout << "King moves in legal move list:\n";
    for (const Move& move : legalMoves) {
        if (from(move) == whiteKing) {
            Square to = ::seajay::to(move);
            std::cout << "  " << squareToString(whiteKing) << squareToString(to) << "\n";
            
            if (to == f2) hasE2F2 = true;
            if (to == f3) hasE2F3 = true;
        }
    }
    
    std::cout << "\n============================================\n";
    std::cout << "RESULTS:\n";
    std::cout << "============================================\n";
    std::cout << "e2f2 in legal moves: " << (hasE2F2 ? "YES" : "NO") << "\n";
    std::cout << "e2f3 in legal moves: " << (hasE2F3 ? "YES" : "NO") << "\n\n";
    
    std::cout << "Expected (per Stockfish):\n";
    std::cout << "  e2f2 should be: LEGAL (in list)\n";
    std::cout << "  e2f3 should be: ILLEGAL (not in list)\n\n";
    
    // Determine if there's a bug
    bool bug = false;
    if (!hasE2F2) {
        std::cout << "❌ BUG: e2f2 should be legal but isn't!\n";
        bug = true;
    }
    if (hasE2F3) {
        std::cout << "❌ BUG: e2f3 should be illegal but is in the list!\n";
        bug = true;
    }
    
    if (!bug) {
        std::cout << "✅ NO BUG: Move generation is correct!\n";
        std::cout << "    e2f2 is correctly legal\n";
        std::cout << "    e2f3 is correctly illegal\n";
    }
    
    return bug ? 1 : 0;
}