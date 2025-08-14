#include <iostream>
#include <iomanip>
#include <cmath>
#include "../../src/core/board.h"
#include "../../src/search/negamax.h"
#include "../../src/search/iterative_search_data.h"
#include "../../src/core/transposition_table.h"

using namespace seajay;
using namespace seajay::search;

// Test that EBF tracking structure is working
void testEBFStructure() {
    std::cout << "Testing EBF Tracking Structure...\n";
    
    Board board;  // Starting position
    SearchLimits limits;
    limits.maxDepth = 6;
    limits.movetime = std::chrono::milliseconds(1000);
    
    TranspositionTable tt(16);
    
    // Custom search that exposes iteration data
    SearchInfo searchInfo;
    searchInfo.clear();
    searchInfo.setRootHistorySize(board.gameHistorySize());
    
    IterativeSearchData info;
    TimeLimits timeLimits = calculateTimeLimits(limits, board, 1.0);
    info.timeLimit = timeLimits.optimum;
    
    std::cout << "\nDepth | Nodes This Iter | Total Nodes | EBF\n";
    std::cout << "------|----------------|-------------|------\n";
    
    for (int depth = 1; depth <= 5; depth++) {
        info.depth = depth;
        board.setSearchMode(true);
        
        uint64_t nodesBeforeIteration = info.nodes;
        
        eval::Score score = negamax(board, depth, 0,
                                   eval::Score::minus_infinity(),
                                   eval::Score::infinity(),
                                   searchInfo, info, &tt);
        
        board.setSearchMode(false);
        
        if (!info.stopped) {
            uint64_t iterNodes = info.nodes - nodesBeforeIteration;
            
            // Create iteration info
            IterationInfo iter;
            iter.depth = depth;
            iter.nodes = iterNodes;
            iter.score = score;
            iter.bestMove = info.bestMove;
            
            // Calculate branching factor
            if (depth > 1 && info.hasIterations()) {
                const IterationInfo& prevIter = info.getLastIteration();
                if (prevIter.nodes > 0) {
                    iter.branchingFactor = static_cast<double>(iter.nodes) / prevIter.nodes;
                }
            }
            
            info.recordIteration(iter);
            
            std::cout << std::setw(5) << depth << " | "
                     << std::setw(14) << iterNodes << " | "
                     << std::setw(11) << info.nodes << " | ";
            
            if (depth > 1) {
                std::cout << std::fixed << std::setprecision(2) 
                         << iter.branchingFactor;
            } else {
                std::cout << "  N/A";
            }
            std::cout << "\n";
        }
    }
    
    // Verify node count array and EBF field exist
    std::cout << "\n✓ Node count array exists (iter.nodes field)\n";
    std::cout << "✓ EBF field exists (iter.branchingFactor field)\n";
    std::cout << "✓ Compile test passed\n";
}

int main() {
    std::cout << "=== Stage 13, Deliverable 4.1a: EBF Tracking Structure Test ===\n\n";
    
    testEBFStructure();
    
    std::cout << "\n=== Test Complete ===\n";
    return 0;
}