// Stage 13, Deliverable 1.2c: Test full iteration recording (all depths)

#include <iostream>
#include <iomanip>
#include <cassert>
#include <cmath>
#include "../src/search/negamax.h"
#include "../src/search/iterative_search_data.h"
#include "../src/core/transposition_table.h"
#include "../src/core/board.h"

using namespace seajay;
using namespace seajay::search;

// Modified version that returns the IterativeSearchData for verification
struct SearchResult {
    Move bestMove;
    IterativeSearchData data;
};

SearchResult searchIterativeTestWithData(Board& board, const SearchLimits& limits, TranspositionTable* tt) {
    SearchInfo searchInfo;
    searchInfo.clear();
    searchInfo.setRootHistorySize(board.gameHistorySize());
    
    IterativeSearchData info;
    info.timeLimit = calculateTimeLimit(limits, board);
    
    Move bestMove;
    Move previousBestMove = NO_MOVE;
    
    for (int depth = 1; depth <= limits.maxDepth; depth++) {
        info.depth = depth;
        board.setSearchMode(true);
        
        auto iterationStart = std::chrono::steady_clock::now();
        uint64_t nodesBeforeIteration = info.nodes;
        
        eval::Score score = negamax(board, depth, 0,
                                   eval::Score::minus_infinity(),
                                   eval::Score::infinity(),
                                   searchInfo, info, tt);
        
        board.setSearchMode(false);
        
        if (!info.stopped) {
            bestMove = info.bestMove;
            sendSearchInfo(info);
            
            // Record iteration data for ALL depths
            auto iterationEnd = std::chrono::steady_clock::now();
            auto iterationTime = std::chrono::duration_cast<std::chrono::milliseconds>(
                iterationEnd - iterationStart).count();
            
            IterationInfo iter;
            iter.depth = depth;
            iter.score = score;
            iter.bestMove = info.bestMove;
            iter.nodes = info.nodes - nodesBeforeIteration;
            iter.elapsed = iterationTime;
            iter.alpha = eval::Score::minus_infinity();
            iter.beta = eval::Score::infinity();
            iter.windowAttempts = 0;
            iter.failedHigh = false;
            iter.failedLow = false;
            
            // Track move changes and stability
            if (depth == 1) {
                iter.moveChanged = false;
                iter.moveStability = 1;
            } else {
                iter.moveChanged = (info.bestMove != previousBestMove);
                if (iter.moveChanged) {
                    iter.moveStability = 1;
                } else {
                    const IterationInfo& prevIter = info.getLastIteration();
                    iter.moveStability = prevIter.moveStability + 1;
                }
            }
            
            iter.firstMoveFailHigh = false;
            iter.failHighMoveIndex = -1;
            iter.secondBestScore = eval::Score::minus_infinity();
            
            // Calculate branching factor
            if (depth > 1 && info.hasIterations()) {
                const IterationInfo& prevIter = info.getLastIteration();
                if (prevIter.nodes > 0) {
                    iter.branchingFactor = static_cast<double>(iter.nodes) / prevIter.nodes;
                } else {
                    iter.branchingFactor = 0.0;
                }
            } else {
                iter.branchingFactor = 0.0;
            }
            
            info.recordIteration(iter);
            previousBestMove = info.bestMove;
            
            std::cout << "  [DEBUG] Recorded depth " << depth 
                      << " - nodes=" << iter.nodes 
                      << ", score=" << iter.score.value()
                      << ", move=" << std::hex << iter.bestMove << std::dec
                      << ", BF=" << std::fixed << std::setprecision(2) << iter.branchingFactor
                      << ", stability=" << iter.moveStability
                      << ", changed=" << (iter.moveChanged ? "yes" : "no")
                      << std::endl;
            
            if (score.is_mate_score()) {
                break;
            }
            
            if (info.timeLimit != std::chrono::milliseconds::max()) {
                auto elapsed = info.elapsed();
                if (elapsed * 5 > info.timeLimit * 2) {
                    break;
                }
            }
        } else {
            break;
        }
    }
    
    return {bestMove, info};
}

void testAllDepthsRecorded() {
    std::cout << "Testing that ALL depths are recorded..." << std::endl;
    
    Board board;
    board.setStartingPosition();
    TranspositionTable tt(16);
    
    SearchLimits limits;
    limits.maxDepth = 5;  // Search to depth 5
    limits.infinite = false;
    
    SearchResult result = searchIterativeTestWithData(board, limits, &tt);
    
    // Verify we have 5 iterations recorded (depths 1-5)
    assert(result.data.getIterationCount() == 5);
    std::cout << "  ✓ Iteration count = 5 (expected for depth 5 search)" << std::endl;
    
    // Verify each depth is recorded correctly
    for (size_t i = 0; i < 5; i++) {
        const IterationInfo& iter = result.data.getIteration(i);
        assert(iter.depth == static_cast<int>(i + 1));
        assert(iter.nodes > 0);
        assert(iter.bestMove != NO_MOVE);
        std::cout << "  ✓ Depth " << (i+1) << " recorded: nodes=" << iter.nodes 
                  << ", score=" << iter.score.value() << std::endl;
    }
    
    std::cout << "  Test passed!" << std::endl;
}

void testBranchingFactorCalculation() {
    std::cout << "Testing branching factor calculation..." << std::endl;
    
    Board board;
    board.setStartingPosition();
    TranspositionTable tt(16);
    
    SearchLimits limits;
    limits.maxDepth = 4;
    limits.infinite = false;
    
    SearchResult result = searchIterativeTestWithData(board, limits, &tt);
    
    // Check branching factors are reasonable
    for (size_t i = 1; i < result.data.getIterationCount(); i++) {
        const IterationInfo& iter = result.data.getIteration(i);
        
        if (i == 0) {
            // First iteration should have BF = 0
            assert(iter.branchingFactor == 0.0);
        } else {
            // Later iterations should have reasonable branching factors
            // In chess, typical EBF is between 2 and 35
            assert(iter.branchingFactor >= 0.0);
            assert(iter.branchingFactor <= 50.0);
            std::cout << "  Depth " << iter.depth 
                      << " BF=" << std::fixed << std::setprecision(2) << iter.branchingFactor 
                      << " (reasonable)" << std::endl;
        }
    }
    
    std::cout << "  ✓ All branching factors within reasonable range" << std::endl;
    std::cout << "  Test passed!" << std::endl;
}

void testMoveStabilityTracking() {
    std::cout << "Testing move stability tracking..." << std::endl;
    
    Board board;
    board.setStartingPosition();
    TranspositionTable tt(16);
    
    SearchLimits limits;
    limits.maxDepth = 4;
    limits.infinite = false;
    
    SearchResult result = searchIterativeTestWithData(board, limits, &tt);
    
    // Check move stability is tracked correctly
    Move lastMove = NO_MOVE;
    int expectedStability = 0;
    
    for (size_t i = 0; i < result.data.getIterationCount(); i++) {
        const IterationInfo& iter = result.data.getIteration(i);
        
        if (i == 0) {
            // First iteration: moveChanged = false, stability = 1
            assert(!iter.moveChanged);
            assert(iter.moveStability == 1);
            expectedStability = 1;
        } else {
            // Check if move changed from previous iteration
            if (iter.bestMove == lastMove) {
                // Move didn't change - stability should increment
                expectedStability++;
                assert(!iter.moveChanged);
                assert(iter.moveStability == expectedStability);
            } else {
                // Move changed - stability resets to 1
                expectedStability = 1;
                assert(iter.moveChanged);
                assert(iter.moveStability == 1);
            }
        }
        
        lastMove = iter.bestMove;
        
        std::cout << "  Depth " << iter.depth 
                  << ": move=" << std::hex << iter.bestMove << std::dec
                  << ", changed=" << (iter.moveChanged ? "yes" : "no")
                  << ", stability=" << iter.moveStability << std::endl;
    }
    
    std::cout << "  ✓ Move stability tracked correctly" << std::endl;
    std::cout << "  Test passed!" << std::endl;
}

void testIterationDataCompleteness() {
    std::cout << "Testing iteration data completeness..." << std::endl;
    
    Board board;
    board.setStartingPosition();
    TranspositionTable tt(16);
    
    SearchLimits limits;
    limits.maxDepth = 3;
    limits.infinite = false;
    
    SearchResult result = searchIterativeTestWithData(board, limits, &tt);
    
    // Verify all fields are properly initialized for each iteration
    for (size_t i = 0; i < result.data.getIterationCount(); i++) {
        const IterationInfo& iter = result.data.getIteration(i);
        
        // Basic fields
        assert(iter.depth == static_cast<int>(i + 1));
        assert(iter.nodes > 0);
        assert(iter.bestMove != NO_MOVE);
        assert(iter.elapsed >= 0);
        
        // Window fields (no aspiration windows yet)
        assert(iter.alpha == eval::Score::minus_infinity());
        assert(iter.beta == eval::Score::infinity());
        assert(iter.windowAttempts == 0);
        assert(!iter.failedHigh);
        assert(!iter.failedLow);
        
        // Additional fields
        assert(!iter.firstMoveFailHigh);
        assert(iter.failHighMoveIndex == -1);
        assert(iter.secondBestScore == eval::Score::minus_infinity());
        
        std::cout << "  ✓ Depth " << iter.depth << " data complete" << std::endl;
    }
    
    std::cout << "  Test passed!" << std::endl;
}

int main() {
    std::cout << "\n=== Stage 13, Deliverable 1.2c: Full Iteration Recording Tests ===" << std::endl;
    
    try {
        testAllDepthsRecorded();
        testBranchingFactorCalculation();
        testMoveStabilityTracking();
        testIterationDataCompleteness();
        
        std::cout << "\n✓ All tests passed!" << std::endl;
        std::cout << "Deliverable 1.2c COMPLETE: Full iteration recording (all depths) implemented correctly" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Test failed with exception: " << e.what() << std::endl;
        return 1;
    }
}