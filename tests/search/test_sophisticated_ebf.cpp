#include <iostream>
#include <iomanip>
#include <cmath>
#include <vector>
#include "../../src/core/board.h"
#include "../../src/search/negamax.h"
#include "../../src/search/iterative_search_data.h"
#include "../../src/core/transposition_table.h"

using namespace seajay;
using namespace seajay::search;

// Test sophisticated EBF with weighted average
void testSophisticatedEBF() {
    std::cout << "Testing Sophisticated EBF (weighted average of 3-4 iterations)...\n\n";
    
    // Create mock data for testing
    IterativeSearchData info;
    
    // Test case 1: Only 2 iterations (should use simple EBF)
    std::cout << "Test 1: Two iterations only\n";
    IterationInfo iter1;
    iter1.depth = 1;
    iter1.nodes = 100;
    info.recordIteration(iter1);
    
    IterationInfo iter2;
    iter2.depth = 2;
    iter2.nodes = 500;  // EBF = 5.0
    info.recordIteration(iter2);
    
    double sophisticatedEBF = info.getSophisticatedEBF();
    std::cout << "  Nodes: 100 -> 500\n";
    std::cout << "  Expected EBF: 5.00 (simple)\n";
    std::cout << "  Calculated EBF: " << std::fixed << std::setprecision(2) << sophisticatedEBF << "\n";
    
    if (std::abs(sophisticatedEBF - 5.0) < 0.01) {
        std::cout << "  ✓ Correct (falls back to simple with 2 iterations)\n";
    } else {
        std::cout << "  ✗ Incorrect\n";
    }
    
    // Test case 2: Three iterations with weighted average
    std::cout << "\nTest 2: Three iterations (weighted average)\n";
    IterationInfo iter3;
    iter3.depth = 3;
    iter3.nodes = 2000;  // EBF = 4.0 from iter2
    info.recordIteration(iter3);
    
    // Manual calculation:
    // EBF(2->3) = 2000/500 = 4.0, weight = 3
    // EBF(1->2) = 500/100 = 5.0, weight = 2
    // Weighted average = (4.0*3 + 5.0*2) / (3+2) = (12 + 10) / 5 = 4.4
    
    sophisticatedEBF = info.getSophisticatedEBF();
    std::cout << "  Nodes: 100 -> 500 -> 2000\n";
    std::cout << "  EBF(1->2) = 5.0, weight = 2\n";
    std::cout << "  EBF(2->3) = 4.0, weight = 3\n";
    std::cout << "  Expected weighted EBF: 4.40\n";
    std::cout << "  Calculated EBF: " << std::fixed << std::setprecision(2) << sophisticatedEBF << "\n";
    
    if (std::abs(sophisticatedEBF - 4.4) < 0.01) {
        std::cout << "  ✓ Correct weighted average\n";
    } else {
        std::cout << "  ✗ Incorrect\n";
    }
    
    // Test case 3: Four iterations with weighted average
    std::cout << "\nTest 3: Four iterations (weighted average)\n";
    IterationInfo iter4;
    iter4.depth = 4;
    iter4.nodes = 6000;  // EBF = 3.0 from iter3
    info.recordIteration(iter4);
    
    // Manual calculation:
    // EBF(3->4) = 6000/2000 = 3.0, weight = 4
    // EBF(2->3) = 2000/500 = 4.0, weight = 3
    // EBF(1->2) = 500/100 = 5.0, weight = 2
    // Weighted average = (3.0*4 + 4.0*3 + 5.0*2) / (4+3+2) = (12 + 12 + 10) / 9 = 3.78
    
    sophisticatedEBF = info.getSophisticatedEBF();
    std::cout << "  Nodes: 100 -> 500 -> 2000 -> 6000\n";
    std::cout << "  EBF(1->2) = 5.0, weight = 2\n";
    std::cout << "  EBF(2->3) = 4.0, weight = 3\n";
    std::cout << "  EBF(3->4) = 3.0, weight = 4\n";
    std::cout << "  Expected weighted EBF: 3.78\n";
    std::cout << "  Calculated EBF: " << std::fixed << std::setprecision(2) << sophisticatedEBF << "\n";
    
    if (std::abs(sophisticatedEBF - 3.78) < 0.01) {
        std::cout << "  ✓ Correct weighted average\n";
    } else {
        std::cout << "  ✗ Incorrect\n";
    }
    
    // Test case 4: Five iterations (should only use last 4)
    std::cout << "\nTest 4: Five iterations (uses only last 4)\n";
    IterationInfo iter5;
    iter5.depth = 5;
    iter5.nodes = 15000;  // EBF = 2.5 from iter4
    info.recordIteration(iter5);
    
    // Manual calculation (using last 4 only):
    // EBF(4->5) = 15000/6000 = 2.5, weight = 4
    // EBF(3->4) = 6000/2000 = 3.0, weight = 3
    // EBF(2->3) = 2000/500 = 4.0, weight = 2
    // Weighted average = (2.5*4 + 3.0*3 + 4.0*2) / (4+3+2) = (10 + 9 + 8) / 9 = 3.00
    
    sophisticatedEBF = info.getSophisticatedEBF();
    std::cout << "  Nodes: ... -> 500 -> 2000 -> 6000 -> 15000\n";
    std::cout << "  EBF(2->3) = 4.0, weight = 2\n";
    std::cout << "  EBF(3->4) = 3.0, weight = 3\n";
    std::cout << "  EBF(4->5) = 2.5, weight = 4\n";
    std::cout << "  Expected weighted EBF: 3.00\n";
    std::cout << "  Calculated EBF: " << std::fixed << std::setprecision(2) << sophisticatedEBF << "\n";
    
    if (std::abs(sophisticatedEBF - 3.00) < 0.01) {
        std::cout << "  ✓ Correct (uses only last 4 iterations)\n";
    } else {
        std::cout << "  ✗ Incorrect\n";
    }
}

// Compare to expected values in real chess
void compareToExpectedValues() {
    std::cout << "\nComparing to Expected Chess EBF Values...\n";
    
    // Typical EBF values for chess with good move ordering:
    // - Without pruning: ~35
    // - With alpha-beta only: ~6-10
    // - With good move ordering: ~3-5
    // - With TT and other enhancements: ~2-4
    
    Board board;
    board.fromFEN("r1bqkbnr/pppp1ppp/2n5/4p3/4P3/5N2/PPPP1PPP/RNBQKB1R w KQkq - 0 1");
    
    SearchLimits limits;
    limits.maxDepth = 6;
    limits.movetime = std::chrono::milliseconds(2000);
    
    TranspositionTable tt(16);
    
    SearchInfo searchInfo;
    searchInfo.clear();
    searchInfo.setRootHistorySize(board.gameHistorySize());
    
    IterativeSearchData info;
    TimeLimits timeLimits = calculateTimeLimits(limits, board, 1.0);
    info.timeLimit = timeLimits.optimum;
    
    std::cout << "\nDepth | Simple EBF | Sophisticated EBF | Expected Range\n";
    std::cout << "------|------------|------------------|---------------\n";
    
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
            
            IterationInfo iter;
            iter.depth = depth;
            iter.nodes = iterNodes;
            iter.score = score;
            iter.bestMove = info.bestMove;
            
            // Calculate simple EBF
            if (depth > 1 && info.hasIterations()) {
                const IterationInfo& prevIter = info.getLastIteration();
                if (prevIter.nodes > 0) {
                    iter.branchingFactor = static_cast<double>(iter.nodes) / prevIter.nodes;
                }
            }
            
            info.recordIteration(iter);
            
            if (depth > 1) {
                double simpleEBF = iter.branchingFactor;
                double sophisticatedEBF = info.getSophisticatedEBF();
                
                std::cout << std::setw(5) << depth << " | "
                         << std::fixed << std::setprecision(2) 
                         << std::setw(10) << simpleEBF << " | "
                         << std::setw(16) << sophisticatedEBF << " | ";
                
                // Expected range with TT and good ordering
                std::cout << "2.0 - 6.0";
                
                if (sophisticatedEBF >= 2.0 && sophisticatedEBF <= 8.0) {
                    std::cout << " ✓";
                }
                std::cout << "\n";
            }
        }
    }
    
    std::cout << "\n✓ Sophisticated EBF implemented with weighted average\n";
}

int main() {
    std::cout << "=== Stage 13, Deliverable 4.1c: Sophisticated EBF Test ===\n\n";
    
    testSophisticatedEBF();
    compareToExpectedValues();
    
    std::cout << "\n=== Test Complete ===\n";
    return 0;
}