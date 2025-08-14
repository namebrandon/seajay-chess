// Stage 13, Deliverable 2.1e: Test stability detection logic

#include "../../src/search/iterative_search_data.h"
#include "../../src/search/iteration_info.h"
#include "../../src/core/types.h"
#include <iostream>
#include <cassert>

// Simple test assert macro
#define TEST_ASSERT(condition, message) \
    do { \
        if (!(condition)) { \
            std::cerr << "FAILED: " << message << std::endl; \
            std::cerr << "  at " << __FILE__ << ":" << __LINE__ << std::endl; \
            throw std::runtime_error(message); \
        } \
    } while(0)

using namespace seajay;
using namespace seajay::search;

// Helper to create iteration info
IterationInfo createIteration(int depth, Move move, eval::Score score) {
    IterationInfo iter;
    iter.depth = depth;
    iter.bestMove = move;
    iter.score = score;
    iter.nodes = 1000 * depth;  // Dummy nodes
    iter.elapsed = 100 * depth;  // Dummy time
    return iter;
}

void testMoveStability() {
    std::cout << "Testing move stability detection..." << std::endl;
    
    IterativeSearchData data;
    data.reset();
    
    // Create some test moves
    Move move1 = static_cast<Move>((12) | (28 << 6));  // e2-e4
    Move move2 = static_cast<Move>((11) | (27 << 6));  // d2-d4
    
    // Iteration 1: First move
    IterationInfo iter1 = createIteration(1, move1, eval::Score(50));
    data.recordIteration(iter1);
    data.updateStability(iter1);
    
    TEST_ASSERT(!data.isPositionStable(), "Position should not be stable after 1 iteration");
    TEST_ASSERT(data.m_stabilityCount == 1, "Stability count should be 1");
    
    // Iteration 2: Same move
    IterationInfo iter2 = createIteration(2, move1, eval::Score(55));
    data.recordIteration(iter2);
    data.updateStability(iter2);
    
    TEST_ASSERT(data.isPositionStable(), "Position should be stable after 2 iterations with same move");
    TEST_ASSERT(data.m_stabilityCount == 2, "Stability count should be 2");
    
    // Iteration 3: Different move - should reset stability
    IterationInfo iter3 = createIteration(3, move2, eval::Score(60));
    data.recordIteration(iter3);
    data.updateStability(iter3);
    
    TEST_ASSERT(!data.isPositionStable(), "Position should not be stable after move change");
    TEST_ASSERT(data.m_stabilityCount == 1, "Stability count should reset to 1");
    TEST_ASSERT(data.m_stableBestMove == move2, "Stable best move should be updated");
    
    // Iteration 4: Back to stable with new move
    IterationInfo iter4 = createIteration(4, move2, eval::Score(62));
    data.recordIteration(iter4);
    data.updateStability(iter4);
    
    TEST_ASSERT(data.isPositionStable(), "Position should be stable again");
    TEST_ASSERT(data.m_stabilityCount == 2, "Stability count should be 2");
    
    std::cout << "Move stability tests passed!" << std::endl;
}

void testScoreStability() {
    std::cout << "Testing score stability detection..." << std::endl;
    
    IterativeSearchData data;
    data.reset();
    
    Move move1 = static_cast<Move>((12) | (28 << 6));  // e2-e4
    
    // Iteration 1: Initial score
    IterationInfo iter1 = createIteration(1, move1, eval::Score(100));
    data.recordIteration(iter1);
    data.updateStability(iter1);
    
    TEST_ASSERT(data.m_scoreStabilityCount == 1, "Score stability should be 1 initially");
    
    // Iteration 2: Score within window (100 -> 105, window is 10)
    IterationInfo iter2 = createIteration(2, move1, eval::Score(105));
    data.recordIteration(iter2);
    data.updateStability(iter2);
    
    TEST_ASSERT(data.m_scoreStabilityCount == 2, "Score stability should increment");
    TEST_ASSERT(data.isPositionStable(), "Position should be stable (move and score stable)");
    
    // Iteration 3: Score outside window (105 -> 120)
    IterationInfo iter3 = createIteration(3, move1, eval::Score(120));
    data.recordIteration(iter3);
    data.updateStability(iter3);
    
    TEST_ASSERT(data.m_scoreStabilityCount == 1, "Score stability should reset");
    TEST_ASSERT(!data.isPositionStable(), "Position should not be stable (score unstable)");
    
    // Iteration 4: Score stable again
    IterationInfo iter4 = createIteration(4, move1, eval::Score(122));
    data.recordIteration(iter4);
    data.updateStability(iter4);
    
    TEST_ASSERT(data.m_scoreStabilityCount == 2, "Score stability should increment");
    
    std::cout << "Score stability tests passed!" << std::endl;
}

void testStabilityFactor() {
    std::cout << "Testing stability factor calculation..." << std::endl;
    
    IterativeSearchData data;
    data.reset();
    
    Move move1 = static_cast<Move>((12) | (28 << 6));  // e2-e4
    
    // Build up stable position
    for (int i = 1; i <= 5; i++) {
        IterationInfo iter = createIteration(i, move1, eval::Score(100 + i));
        data.recordIteration(iter);
        data.updateStability(iter);
    }
    
    // Should be very stable now
    double factor = data.getStabilityFactor();
    TEST_ASSERT(factor < 1.0, "Stable position should have factor < 1.0");
    std::cout << "Stable position factor: " << factor << std::endl;
    
    // Create unstable position
    data.reset();
    IterationInfo iter1 = createIteration(1, move1, eval::Score(100));
    data.recordIteration(iter1);
    data.updateStability(iter1);
    
    IterationInfo iter2 = createIteration(2, move1, eval::Score(100));
    data.recordIteration(iter2);
    data.updateStability(iter2);
    
    IterationInfo iter3 = createIteration(3, move1, eval::Score(100));
    data.recordIteration(iter3);
    data.updateStability(iter3);
    
    // Now change move - should be unstable
    Move move2 = static_cast<Move>((11) | (27 << 6));  // d2-d4
    IterationInfo iter4 = createIteration(4, move2, eval::Score(150));
    data.recordIteration(iter4);
    data.updateStability(iter4);
    
    factor = data.getStabilityFactor();
    TEST_ASSERT(factor > 1.0, "Unstable position should have factor > 1.0");
    std::cout << "Unstable position factor: " << factor << std::endl;
    
    std::cout << "Stability factor tests passed!" << std::endl;
}

void testShouldExtend() {
    std::cout << "Testing extension due to instability..." << std::endl;
    
    IterativeSearchData data;
    data.reset();
    
    Move move1 = static_cast<Move>((12) | (28 << 6));  // e2-e4
    Move move2 = static_cast<Move>((11) | (27 << 6));  // d2-d4
    
    // Build stable history
    for (int i = 1; i <= 4; i++) {
        IterationInfo iter = createIteration(i, move1, eval::Score(100));
        data.recordIteration(iter);
        data.updateStability(iter);
    }
    
    TEST_ASSERT(!data.shouldExtendDueToInstability(), "Should not extend for stable position");
    
    // Add iteration with move change
    IterationInfo iter5 = createIteration(5, move2, eval::Score(100));
    data.recordIteration(iter5);
    data.updateStability(iter5);
    
    TEST_ASSERT(data.shouldExtendDueToInstability(), "Should extend after move change");
    
    // Test score instability
    data.reset();
    for (int i = 1; i <= 4; i++) {
        IterationInfo iter = createIteration(i, move1, eval::Score(100));
        data.recordIteration(iter);
        data.updateStability(iter);
    }
    
    // Large score change
    IterationInfo iterUnstable = createIteration(5, move1, eval::Score(200));
    data.recordIteration(iterUnstable);
    data.updateStability(iterUnstable);
    
    TEST_ASSERT(data.shouldExtendDueToInstability(), "Should extend after score change");
    
    std::cout << "Extension tests passed!" << std::endl;
}

int main() {
    std::cout << "=== Stage 13, Deliverable 2.1e: Stability Detection Tests ===" << std::endl;
    
    try {
        testMoveStability();
        testScoreStability();
        testStabilityFactor();
        testShouldExtend();
        
        std::cout << "\nAll stability detection tests passed!" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Test failed with exception: " << e.what() << std::endl;
        return 1;
    }
}