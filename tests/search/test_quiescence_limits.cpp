#include <gtest/gtest.h>
#include "../../src/core/board.h"
#include "../../src/search/quiescence.h"
#include "../../src/search/search_info.h"
#include "../../src/search/types.h"
#include "../../src/core/transposition_table.h"

using namespace seajay;
using namespace seajay::search;

class QuiescenceLimitsTest : public ::testing::Test {
protected:
    Board board;
    SearchInfo searchInfo;
    SearchData searchData;
    TranspositionTable tt{16}; // 16MB TT
};

TEST_F(QuiescenceLimitsTest, NodeLimitEnforcement) {
    // Position with many captures available - should hit node limit in testing mode
    // This is a complex tactical position that would normally search many nodes
    board.setFen("r1bqk2r/pp2nppp/2n1p3/3p4/1bPP4/2N1PN2/PP3PPP/R1BQKB1R w KQkq - 0 8");
    
    // Reset search data
    searchData.reset();
    
    // Run quiescence search from the root
    eval::Score score = quiescence(board, 0, 
                                   eval::Score::minus_infinity(),
                                   eval::Score::plus_infinity(),
                                   searchInfo, searchData, tt);
    
    // In QSEARCH_TESTING mode, we should hit the 10,000 node limit
    // Check that we searched a reasonable number of nodes
#ifdef QSEARCH_TESTING
    // With 10,000 node limit, we might hit it on complex positions
    EXPECT_LE(searchData.qsearchNodes, 10001u) 
        << "Should not exceed node limit + 1 (for entry node)";
    
    // If we searched exactly 10,001 nodes, we likely hit the limit
    if (searchData.qsearchNodes > 10000u) {
        EXPECT_GT(searchData.qsearchNodesLimited, 0u) 
            << "Should track when node limit is hit";
    }
#elif defined(QSEARCH_TUNING)
    // With 100,000 node limit, less likely to hit it
    EXPECT_LE(searchData.qsearchNodes, 100001u) 
        << "Should not exceed tuning node limit";
#else
    // In production, no artificial limit
    // Just verify search completes without crashing
    EXPECT_GT(searchData.qsearchNodes, 0u) 
        << "Should search at least some nodes";
#endif
    
    // Score should be reasonable (not infinite/error value)
    EXPECT_GT(score.value(), -30000);
    EXPECT_LT(score.value(), 30000);
}

TEST_F(QuiescenceLimitsTest, SimplePositionNoLimit) {
    // Simple position with few captures - should not hit limit
    board.setFen("rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq e3 0 1");
    
    searchData.reset();
    
    eval::Score score = quiescence(board, 0,
                                   eval::Score::minus_infinity(),
                                   eval::Score::plus_infinity(),
                                   searchInfo, searchData, tt);
    
    // Simple position should not hit any limits
    EXPECT_EQ(searchData.qsearchNodesLimited, 0u)
        << "Simple position should not hit node limits";
    
    // Should search very few nodes (no captures available)
    EXPECT_LT(searchData.qsearchNodes, 100u)
        << "Simple quiet position should search few nodes";
}

TEST_F(QuiescenceLimitsTest, TrackingAccuracy) {
    // Test that entry nodes are tracked correctly per call
    board.setFen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    
    searchData.reset();
    
    // First call
    uint64_t nodesBefore = searchData.qsearchNodes;
    quiescence(board, 0,
              eval::Score::minus_infinity(),
              eval::Score::plus_infinity(),
              searchInfo, searchData, tt);
    uint64_t nodesAfterFirst = searchData.qsearchNodes;
    
    // Second call - entry nodes should be tracked independently
    quiescence(board, 0,
              eval::Score::minus_infinity(),
              eval::Score::plus_infinity(),
              searchInfo, searchData, tt);
    uint64_t nodesAfterSecond = searchData.qsearchNodes;
    
    // Each call should add roughly the same number of nodes
    uint64_t firstCallNodes = nodesAfterFirst - nodesBefore;
    uint64_t secondCallNodes = nodesAfterSecond - nodesAfterFirst;
    
    // In a deterministic position, node counts should be identical
    EXPECT_EQ(firstCallNodes, secondCallNodes)
        << "Same position should search same number of nodes";
}

// Test to ensure pragma messages work correctly (compile-time test)
#ifdef QSEARCH_TESTING
TEST_F(QuiescenceLimitsTest, TestingModeActive) {
    EXPECT_EQ(NODE_LIMIT_PER_POSITION, 10000u)
        << "Testing mode should have 10,000 node limit";
}
#elif defined(QSEARCH_TUNING)
TEST_F(QuiescenceLimitsTest, TuningModeActive) {
    EXPECT_EQ(NODE_LIMIT_PER_POSITION, 100000u)
        << "Tuning mode should have 100,000 node limit";
}
#else
TEST_F(QuiescenceLimitsTest, ProductionModeActive) {
    EXPECT_EQ(NODE_LIMIT_PER_POSITION, UINT64_MAX)
        << "Production mode should have no artificial limit";
}
#endif