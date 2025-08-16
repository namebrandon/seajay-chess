#include <iostream>
#include <iomanip>
#include <vector>
#include <algorithm>
#include "src/core/board.h"
#include "src/core/move_generation.h"
#include "src/core/move_list.h"
#include "src/evaluation/evaluate.h"

using namespace seajay;

void testSymmetry() {
    Board board1, board2;
    
    // Test 1: Starting position
    board1.setStartingPosition();
    board2.fromFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR b KQkq - 0 1");
    
    eval::Score eval1 = eval::evaluate(board1);  // White to move
    eval::Score eval2 = eval::evaluate(board2);  // Black to move
    
    std::cout << "Starting position:\n";
    std::cout << "  White to move eval: " << eval1.value() << "\n";
    std::cout << "  Black to move eval: " << eval2.value() << "\n";
    std::cout << "  Sum (should be ~0): " << (eval1.value() + eval2.value()) << "\n\n";
    
    // Test 2: After 1.e4 e5 (symmetric position)
    board1.fromFEN("rnbqkbnr/pppp1ppp/8/4p3/4P3/8/PPPP1PPP/RNBQKBNR w KQkq e6 0 2");
    board2.fromFEN("rnbqkbnr/pppp1ppp/8/4p3/4P3/8/PPPP1PPP/RNBQKBNR b KQkq e3 0 2");
    
    eval1 = eval::evaluate(board1);  // White to move
    eval2 = eval::evaluate(board2);  // Black to move
    
    std::cout << "After 1.e4 e5 (symmetric):\n";
    std::cout << "  White to move eval: " << eval1.value() << "\n";
    std::cout << "  Black to move eval: " << eval2.value() << "\n";
    std::cout << "  Sum (should be ~0): " << (eval1.value() + eval2.value()) << "\n\n";
    
    // Test 3: Check what Black thinks after 1.d4
    board1.fromFEN("rnbqkbnr/pppppppp/8/8/3P4/8/PPP1PPPP/RNBQKBNR b KQkq d3 0 1");
    eval1 = eval::evaluate(board1);
    
    std::cout << "After 1.d4 (Black to move):\n";
    std::cout << "  Black's evaluation: " << eval1.value() << "\n";
    std::cout << "  (Negative means Black is worse, positive means Black is better)\n\n";
    
    // Now check Black's moves and their evaluations
    MoveList moves = generateLegalMoves(board1);
    
    std::cout << "Black's move evaluations after 1.d4:\n";
    struct MoveEval {
        Move move;
        int eval;
        std::string moveStr;
    };
    
    std::vector<MoveEval> moveEvals;
    for (const Move& move : moves) {
        Board::UndoInfo undo;
        board1.makeMove(move, undo);
        eval::Score evalAfter = eval::evaluate(board1);
        board1.unmakeMove(move, undo);
        
        Square from = moveFrom(move);
        Square to = moveTo(move);
        std::string moveStr;
        moveStr += char('a' + (from % 8));
        moveStr += char('1' + (from / 8));
        moveStr += char('a' + (to % 8));
        moveStr += char('1' + (to / 8));
        
        // evalAfter is from White's perspective (after Black's move)
        // We want Black's perspective, so negate it
        moveEvals.push_back({move, -evalAfter.value(), moveStr});
    }
    
    // Sort by evaluation (best first for Black)
    std::sort(moveEvals.begin(), moveEvals.end(), 
              [](const MoveEval& a, const MoveEval& b) { 
                  return a.eval > b.eval; 
              });
    
    // Show top 10 and bottom 10 moves
    std::cout << "  Top 10 moves (best for Black):\n";
    for (int i = 0; i < 10 && i < moveEvals.size(); i++) {
        std::cout << "    " << std::setw(6) << moveEvals[i].moveStr 
                  << " eval: " << std::setw(5) << moveEvals[i].eval << "\n";
    }
    
    std::cout << "\n  Bottom 10 moves (worst for Black):\n";
    for (int i = std::max(0, (int)moveEvals.size() - 10); i < moveEvals.size(); i++) {
        std::cout << "    " << std::setw(6) << moveEvals[i].moveStr 
                  << " eval: " << std::setw(5) << moveEvals[i].eval << "\n";
    }
    
    // Check specifically for a6
    for (const auto& me : moveEvals) {
        if (me.moveStr == "a7a6") {
            std::cout << "\n  a7a6 specifically: eval = " << me.eval 
                      << " (rank " << (&me - &moveEvals[0] + 1) << " of " 
                      << moveEvals.size() << ")\n";
        }
    }
}

int main() {
    std::cout << "=== SeaJay Evaluation Symmetry Test ===\n\n";
    testSymmetry();
    return 0;
}