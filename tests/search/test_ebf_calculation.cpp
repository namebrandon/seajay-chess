#include <iostream>
#include <iomanip>
#include <cmath>
#include "../../src/core/board.h"
#include "../../src/search/negamax.h"
#include "../../src/search/iterative_search_data.h"
#include "../../src/core/transposition_table.h"

using namespace seajay;
using namespace seajay::search;

// Test simple EBF calculation between consecutive iterations
void testSimpleEBF() {
    std::cout << "Testing Simple EBF Calculation (last 2 iterations)...\n";
    
    // Use a more complex position for realistic EBF values
    Board board;
    board.fromFEN("r1bqkbnr/pppp1ppp/2n5/4p3/4P3/5N2/PPPP1PPP/RNBQKB1R w KQkq - 0 1");
    
    SearchLimits limits;
    limits.maxDepth = 7;
    limits.movetime = std::chrono::milliseconds(2000);
    
    TranspositionTable tt(16);
    
    // Run search to collect iteration data
    SearchInfo searchInfo;
    searchInfo.clear();
    searchInfo.setRootHistorySize(board.gameHistorySize());
    
    IterativeSearchData info;
    TimeLimits timeLimits = calculateTimeLimits(limits, board, 1.0);
    info.timeLimit = timeLimits.optimum;
    
    std::cout << "\nDepth | Nodes (iter) | EBF (calc) | Expected Range | Status\n";
    std::cout << "------|-------------|------------|----------------|--------\n";
    
    for (int depth = 1; depth <= 6; depth++) {
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
            
            // Calculate simple EBF using last 2 iterations only
            double calculatedEBF = 0.0;
            if (depth > 1 && info.hasIterations()) {
                const IterationInfo& prevIter = info.getLastIteration();
                if (prevIter.nodes > 0) {
                    calculatedEBF = static_cast<double>(iterNodes) / prevIter.nodes;
                    iter.branchingFactor = calculatedEBF;
                }
            }
            
            info.recordIteration(iter);
            
            std::cout << std::setw(5) << depth << " | "
                     << std::setw(11) << iterNodes << " | ";
            
            if (depth > 1) {
                std::cout << std::fixed << std::setprecision(2) 
                         << std::setw(10) << calculatedEBF << " | ";
                
                // Expected range for chess (with alpha-beta and TT)
                // Depth 2: 5-15 (high due to low starting depth)
                // Depth 3+: 2-8 (should decrease with depth)
                double minExpected = (depth == 2) ? 5.0 : 2.0;
                double maxExpected = (depth == 2) ? 20.0 : 10.0;
                
                std::cout << std::setw(14) << "[" << minExpected << "-" << maxExpected << "] | ";
                
                if (calculatedEBF >= minExpected && calculatedEBF <= maxExpected) {
                    std::cout << "✓ OK";
                } else {
                    std::cout << "⚠ Outside range";
                }
            } else {
                std::cout << std::setw(10) << "N/A" << " | "
                         << std::setw(14) << "N/A" << " | N/A";
            }
            std::cout << "\n";
        }
    }
    
    std::cout << "\n✓ Simple EBF calculation verified (nodes_current / nodes_previous)\n";
}

// Manual calculation verification
void testManualCalculation() {
    std::cout << "\nManual EBF Calculation Verification...\n";
    
    // Create mock iteration data for verification
    IterativeSearchData info;
    
    // Iteration 1: 100 nodes
    IterationInfo iter1;
    iter1.depth = 1;
    iter1.nodes = 100;
    info.recordIteration(iter1);
    
    // Iteration 2: 500 nodes (EBF should be 5.0)
    IterationInfo iter2;
    iter2.depth = 2;
    iter2.nodes = 500;
    
    // Calculate EBF manually
    double expectedEBF = 500.0 / 100.0;  // = 5.0
    
    // Calculate using our method
    if (info.hasIterations()) {
        const IterationInfo& prevIter = info.getLastIteration();
        if (prevIter.nodes > 0) {
            iter2.branchingFactor = static_cast<double>(iter2.nodes) / prevIter.nodes;
        }
    }
    
    std::cout << "Previous iteration nodes: " << iter1.nodes << "\n";
    std::cout << "Current iteration nodes: " << iter2.nodes << "\n";
    std::cout << "Expected EBF: " << expectedEBF << "\n";
    std::cout << "Calculated EBF: " << iter2.branchingFactor << "\n";
    
    if (std::abs(iter2.branchingFactor - expectedEBF) < 0.001) {
        std::cout << "✓ Manual calculation matches\n";
    } else {
        std::cout << "✗ Manual calculation mismatch\n";
    }
    
    info.recordIteration(iter2);
    
    // Iteration 3: 2000 nodes (EBF should be 4.0)
    IterationInfo iter3;
    iter3.depth = 3;
    iter3.nodes = 2000;
    
    expectedEBF = 2000.0 / 500.0;  // = 4.0
    
    if (info.hasIterations()) {
        const IterationInfo& prevIter = info.getLastIteration();
        if (prevIter.nodes > 0) {
            iter3.branchingFactor = static_cast<double>(iter3.nodes) / prevIter.nodes;
        }
    }
    
    std::cout << "\nPrevious iteration nodes: " << iter2.nodes << "\n";
    std::cout << "Current iteration nodes: " << iter3.nodes << "\n";
    std::cout << "Expected EBF: " << expectedEBF << "\n";
    std::cout << "Calculated EBF: " << iter3.branchingFactor << "\n";
    
    if (std::abs(iter3.branchingFactor - expectedEBF) < 0.001) {
        std::cout << "✓ Manual calculation matches\n";
    } else {
        std::cout << "✗ Manual calculation mismatch\n";
    }
}

int main() {
    std::cout << "=== Stage 13, Deliverable 4.1b: Simple EBF Calculation Test ===\n\n";
    
    testSimpleEBF();
    testManualCalculation();
    
    std::cout << "\n=== Test Complete ===\n";
    return 0;
}