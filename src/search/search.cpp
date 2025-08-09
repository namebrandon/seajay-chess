#include "search.h"
#include "../core/board.h"
#include "../core/move_generation.h"
#include "../core/move_list.h"
#include "../evaluation/evaluate.h"
#include <random>
#include <limits>

namespace seajay::search {

Move selectBestMove(Board& board) {
    MoveList moves = generateLegalMoves(board);
    
    if (moves.empty()) {
        return Move();  // Invalid move
    }
    
    // If only one legal move, return it immediately
    if (moves.size() == 1) {
        return moves[0];
    }
    
    Move bestMove = moves[0];  // Default to first move
    eval::Score bestScore = eval::Score::minus_infinity();
    
    // Evaluate each move
    for (Move move : moves) {
        // Make the move
        Board::UndoInfo undo;
        board.makeMove(move, undo);
        
        // Evaluate from opponent's perspective and negate
        // (we want the score from our perspective)
        eval::Score score = -board.evaluate();
        
        // Unmake the move
        board.unmakeMove(move, undo);
        
        // Track best move
        if (score > bestScore) {
            bestScore = score;
            bestMove = move;
        }
    }
    
    return bestMove;
}

Move selectRandomMove(Board& board) {
    MoveList moves = generateLegalMoves(board);
    
    if (moves.empty()) {
        return Move();  // Invalid move
    }
    
    // Select random move
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, moves.size() - 1);
    
    return moves[dis(gen)];
}

} // namespace seajay::search