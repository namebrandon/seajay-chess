#include <iostream>
#include "src/core/board.h"
#include "src/core/move_generation.h"
#include "src/core/board_safety.h"
#include "src/evaluation/evaluate.h"

using namespace seajay;

int main() {
    Board board;
    board.setStartingPosition();
    
    // Flip board to Black's perspective
    board.setColorToMove(BLACK);
    
    MoveList moves;
    MoveGenerator::generateLegalMoves(board, moves);
    
    std::cout << "Legal moves for Black from starting position: " << moves.size() << std::endl;
    std::cout << "\nFirst 10 moves (in generation order):\n";
    for (int i = 0; i < std::min(10, (int)moves.size()); i++) {
        Move move = moves[i];
        std::cout << i+1 << ". " << SafeMoveExecutor::moveToString(move) << std::endl;
    }
    
    // Now test evaluation for each pawn move
    std::cout << "\nEvaluation after each pawn move (from Black's perspective):\n";
    for (Move move : moves) {
        Square from = moveFrom(move);
        Square to = moveTo(move);
        Piece piece = board.pieceAt(from);
        if (typeOf(piece) == PAWN) {
            UndoInfo undo;
            board.makeMove(move, undo);
            Score eval = eval::evaluate(board);
            board.unmakeMove(move, undo);
            std::cout << SafeMoveExecutor::moveToString(move) << ": " << eval << std::endl;
        }
    }
    
    return 0;
}
