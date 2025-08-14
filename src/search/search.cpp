#include "search.h"
#include "negamax.h"
#include "../core/board.h"
#include "../core/move_generation.h"
#include "../core/move_list.h"
#include "../evaluation/evaluate.h"
#include <random>
#include <limits>
#include <chrono>

namespace seajay::search {

Move selectBestMove(Board& board) {
    // Quick check for game over
    MoveList moves = generateLegalMoves(board);
    if (moves.empty()) {
        return Move();  // Invalid move - game over
    }
    
    // If only one legal move, return it immediately
    if (moves.size() == 1) {
        return moves[0];
    }
    
    // Use negamax search with a reasonable time limit
    SearchLimits limits;
    limits.maxDepth = 4;  // Default to 4-ply search
    limits.movetime = std::chrono::milliseconds(1000);  // 1 second per move
    
    // Note: passing nullptr for TT - the main search will need to provide TT
    return search(board, limits, nullptr);
}

Move selectRandomMove(Board& board) {
    MoveList moves = generateLegalMoves(board);
    
    if (moves.empty()) {
        return Move();  // Invalid move
    }
    
    // Select random move using lazy initialization to avoid static init hang
    static auto getRandomGenerator = []() -> std::mt19937& {
        static std::mt19937 gen(std::chrono::steady_clock::now().time_since_epoch().count());
        return gen;
    };
    
    std::uniform_int_distribution<> dis(0, moves.size() - 1);
    
    return moves[dis(getRandomGenerator())];
}

} // namespace seajay::search