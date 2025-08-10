#include <iostream>
#include <iomanip>
#include "../src/core/board.h"
#include "../src/search/negamax.h"
#include "../src/search/types.h"

using namespace seajay;
using namespace seajay::search;

int main() {
    std::cout << "\n========================================\n";
    std::cout << "  Alpha-Beta Pruning Validation Test\n";
    std::cout << "========================================\n\n";
    
    Board board;
    board.fromFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    
    std::cout << "Testing from starting position at various depths:\n\n";
    
    for (int depth = 1; depth <= 5; ++depth) {
        SearchLimits limits;
        limits.maxDepth = depth;
        limits.movetime = std::chrono::milliseconds(30000);  // 30 seconds max
        
        std::cout << "Depth " << depth << ":\n";
        
        // Use iterative deepening search which includes alpha-beta
        Move bestMove = seajay::search::search(board, limits);
        
        std::cout << "  Best move: " << SafeMoveExecutor::moveToString(bestMove) << "\n";
        std::cout << "\n";
    }
    
    std::cout << "\n✓ Alpha-Beta Pruning Test Complete!\n";
    std::cout << "\nNOTE: Check the 'ebf' (effective branching factor) and 'moveeff'\n";
    std::cout << "(move ordering efficiency) values in the output above.\n";
    std::cout << "- EBF < 10 indicates good pruning\n";
    std::cout << "- Move efficiency > 50% indicates good move ordering\n\n";
    
    return 0;
}