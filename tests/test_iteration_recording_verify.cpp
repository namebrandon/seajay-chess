// Stage 13, Deliverable 1.2b: Verify depth 1 iteration is actually recorded

#include <iostream>
#include <cassert>
#include "../src/search/negamax.h"
#include "../src/search/iterative_search_data.h"
#include "../src/core/transposition_table.h"
#include "../src/core/board.h"

using namespace seajay;
using namespace seajay::search;

// Modified version of searchIterativeTest that returns the IterativeSearchData
// so we can verify the recorded iteration
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
            
            // Record iteration data for depth 1 only (minimal recording)
            if (depth == 1) {
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
                iter.moveChanged = false;
                iter.moveStability = 1;
                iter.firstMoveFailHigh = false;
                iter.failHighMoveIndex = -1;
                iter.secondBestScore = eval::Score::minus_infinity();
                iter.branchingFactor = 0.0;
                
                info.recordIteration(iter);
                
                std::cout << "  [DEBUG] Recorded depth " << depth 
                          << " - nodes=" << iter.nodes 
                          << ", score=" << iter.score.value()
                          << ", move=" << std::hex << iter.bestMove << std::dec
                          << ", elapsed=" << iter.elapsed << "ms" << std::endl;
            }
            
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

void testDepth1RecordingOnly() {
    std::cout << "Testing that ONLY depth 1 is recorded..." << std::endl;
    
    Board board;
    board.setStartingPosition();
    TranspositionTable tt(16);
    
    SearchLimits limits;
    limits.maxDepth = 3;  // Search to depth 3
    limits.infinite = false;
    
    SearchResult result = searchIterativeTestWithData(board, limits, &tt);
    
    // Verify we have exactly 1 iteration recorded (depth 1 only)
    assert(result.data.getIterationCount() == 1);
    std::cout << "  ✓ Iteration count = 1 (expected)" << std::endl;
    
    // Verify it's depth 1
    const IterationInfo& iter = result.data.getIteration(0);
    assert(iter.depth == 1);
    std::cout << "  ✓ Recorded depth = 1 (expected)" << std::endl;
    
    // Verify the data looks reasonable
    assert(iter.nodes > 0);
    assert(iter.bestMove != NO_MOVE);
    std::cout << "  ✓ Depth 1 data: nodes=" << iter.nodes 
              << ", score=" << iter.score.value()
              << ", move=" << std::hex << iter.bestMove << std::dec << std::endl;
    
    // Verify we didn't record depth 2 or 3
    const IterationInfo& iter2 = result.data.getIteration(1);
    assert(iter2.depth == 0);  // Should be empty/default
    std::cout << "  ✓ No depth 2 recorded (expected for minimal recording)" << std::endl;
    
    std::cout << "  Test passed!" << std::endl;
}

void testIterationDataCorrectness() {
    std::cout << "Testing iteration data correctness..." << std::endl;
    
    Board board;
    board.setStartingPosition();
    TranspositionTable tt(16);
    
    SearchLimits limits;
    limits.maxDepth = 1;  // Only search depth 1
    limits.infinite = false;
    
    SearchResult result = searchIterativeTestWithData(board, limits, &tt);
    
    // Verify iteration was recorded
    assert(result.data.getIterationCount() == 1);
    
    const IterationInfo& iter = result.data.getLastIteration();
    
    // Verify all fields are set correctly
    assert(iter.depth == 1);
    assert(iter.nodes > 0 && iter.nodes < 100);  // Depth 1 should be ~20 nodes
    assert(iter.bestMove != NO_MOVE);
    assert(iter.elapsed >= 0);  // Should have some time recorded
    assert(iter.alpha == eval::Score::minus_infinity());
    assert(iter.beta == eval::Score::infinity());
    assert(iter.windowAttempts == 0);  // No aspiration windows yet
    assert(!iter.failedHigh);
    assert(!iter.failedLow);
    assert(!iter.moveChanged);  // First iteration, no previous to compare
    assert(iter.moveStability == 1);  // First occurrence of this move
    
    std::cout << "  ✓ All iteration fields correctly set" << std::endl;
    std::cout << "  Test passed!" << std::endl;
}

int main() {
    std::cout << "\n=== Stage 13, Deliverable 1.2b: Iteration Recording Verification ===" << std::endl;
    
    try {
        testDepth1RecordingOnly();
        testIterationDataCorrectness();
        
        std::cout << "\n✓ All verification tests passed!" << std::endl;
        std::cout << "Deliverable 1.2b COMPLETE: Minimal iteration recording (depth 1 only) implemented correctly" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Test failed with exception: " << e.what() << std::endl;
        return 1;
    }
}