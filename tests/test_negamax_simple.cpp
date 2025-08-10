#include "../src/core/board.h"
#include "../src/core/board_safety.h"
#include "../src/core/move_generation.h"
#include "../src/search/negamax.h"
#include "../src/search/types.h"
#include <iostream>

using namespace seajay;

int main() {
    std::cout << "Testing basic negamax functionality...\n";
    
    // Test 1: Simple position with 1-ply search
    {
        Board board;
        std::cout << "Starting position - 1 ply search\n";
        
        search::SearchInfo info;
        info.timeLimit = std::chrono::milliseconds(1000);
        
        eval::Score score = search::negamax(board, 1, 0,
                                           eval::Score::minus_infinity(),
                                           eval::Score::infinity(),
                                           info);
        
        std::cout << "Score: " << score.to_cp() << " cp\n";
        std::cout << "Nodes: " << info.nodes << "\n";
        std::cout << "Best move: " << SafeMoveExecutor::moveToString(info.bestMove) << "\n\n";
    }
    
    // Test 2: 2-ply search
    {
        Board board;
        std::cout << "Starting position - 2 ply search\n";
        
        search::SearchInfo info;
        info.timeLimit = std::chrono::milliseconds(2000);
        
        eval::Score score = search::negamax(board, 2, 0,
                                           eval::Score::minus_infinity(),
                                           eval::Score::infinity(),
                                           info);
        
        std::cout << "Score: " << score.to_cp() << " cp\n";
        std::cout << "Nodes: " << info.nodes << "\n";
        std::cout << "Best move: " << SafeMoveExecutor::moveToString(info.bestMove) << "\n\n";
    }
    
    // Test 3: Iterative deepening to depth 3
    {
        Board board;
        std::cout << "Starting position - ID to depth 3\n";
        
        search::SearchLimits limits;
        limits.maxDepth = 3;
        limits.movetime = std::chrono::milliseconds(3000);
        
        Move bestMove = search::search(board, limits);
        
        std::cout << "Best move: " << SafeMoveExecutor::moveToString(bestMove) << "\n\n";
    }
    
    std::cout << "All tests completed!\n";
    return 0;
}