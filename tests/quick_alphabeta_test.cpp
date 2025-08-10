#include <iostream>
#include <chrono>
#include "../src/core/board.h"
#include "../src/core/move_generation.h"
#include "../src/search/negamax.h"
#include "../src/search/types.h"
#include "../src/core/board_safety.h"

using namespace seajay;
using namespace seajay::search;

int main() {
    std::cout << "Alpha-Beta Pruning Quick Validation Test\n";
    std::cout << "=========================================\n\n";
    
    // Test position
    std::string fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
    Board board;
    board.fromFEN(fen);
    
    // Use the search function which properly manages SearchInfo
    SearchLimits limits;
    limits.maxDepth = 3;
    limits.infinite = false;
    limits.movetime = std::chrono::milliseconds(10000);  // 10 seconds max
    
    std::cout << "Searching depth 3 WITH alpha-beta pruning...\n";
    std::cout << "Starting position: " << fen << "\n\n";
    
    // Search using the main search function
    Move bestMove = seajay::search::search(board, limits);
    
    // Now do a direct negamax call to get statistics
    SearchInfo infoWithAB;
    eval::Score scoreAB = negamax(board, 3, 0,
                                  eval::Score::minus_infinity(),
                                  eval::Score::infinity(),
                                  infoWithAB);
    
    std::cout << "Results WITH alpha-beta:\n";
    if (infoWithAB.bestMove != Move()) {
        std::cout << "  Best move: " << SafeMoveExecutor::moveToString(infoWithAB.bestMove) << "\n";
    } else {
        std::cout << "  Best move: (none)\n";
    }
    std::cout << "  Score: " << scoreAB.to_cp() << " cp\n";
    std::cout << "  Nodes: " << infoWithAB.nodes << "\n";
    std::cout << "  Beta cutoffs: " << infoWithAB.betaCutoffs << "\n";
    std::cout << "  First-move cutoffs: " << infoWithAB.betaCutoffsFirst << "\n";
    std::cout << "  Move ordering efficiency: " << infoWithAB.moveOrderingEfficiency() << "%\n";
    std::cout << "  Effective branching factor: " << infoWithAB.effectiveBranchingFactor() << "\n\n";
    
    // Calculate expected nodes without pruning (approximate)
    // At depth 3: ~20 * 20 * 20 = 8000 nodes (rough estimate)
    double reduction = (1.0 - infoWithAB.nodes / 8000.0) * 100.0;
    std::cout << "Estimated node reduction: " << reduction << "%\n\n";
    
    if (infoWithAB.betaCutoffs > 0) {
        std::cout << "✓ Alpha-beta pruning is ACTIVE and working!\n";
        std::cout << "✓ Move ordering efficiency: " << infoWithAB.moveOrderingEfficiency() << "%\n";
        
        if (infoWithAB.moveOrderingEfficiency() > 50) {
            std::cout << "✓ Good move ordering (>50% first-move cutoffs)\n";
        } else {
            std::cout << "⚠ Move ordering could be improved\n";
        }
    } else {
        std::cout << "✗ No beta cutoffs detected - pruning may not be working\n";
    }
    
    return 0;
}