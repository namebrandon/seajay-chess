#include <gtest/gtest.h>
#include "../src/search/lmr.h"
#include "../src/search/types.h"

using namespace seajay::search;

class LMRTest : public ::testing::Test {
protected:
    SearchData::LMRParams defaultParams;
    
    void SetUp() override {
        // Default UCI parameters
        defaultParams.enabled = true;
        defaultParams.minDepth = 3;
        defaultParams.minMoveNumber = 4;
        defaultParams.baseReduction = 1;
        defaultParams.depthFactor = 3;
    }
};

// Test basic reduction calculation
TEST_F(LMRTest, BasicReduction) {
    // At minimum depth and move number
    EXPECT_EQ(getLMRReduction(3, 4, defaultParams), 1); // base reduction only
    
    // Deeper search should give more reduction
    EXPECT_EQ(getLMRReduction(6, 4, defaultParams), 2); // base + (6-3)/3 = 1 + 1
    EXPECT_EQ(getLMRReduction(9, 4, defaultParams), 3); // base + (9-3)/3 = 1 + 2
}

// Test very late move bonus
TEST_F(LMRTest, VeryLateMoveBonus) {
    // Move 8 - no bonus
    EXPECT_EQ(getLMRReduction(6, 8, defaultParams), 2); // base + depth component
    
    // Move 9 - minimal bonus
    EXPECT_EQ(getLMRReduction(6, 9, defaultParams), 2); // base + depth + (9-8)/4 = 2 + 0
    
    // Move 12 - small bonus
    EXPECT_EQ(getLMRReduction(6, 12, defaultParams), 3); // base + depth + (12-8)/4 = 2 + 1
    
    // Move 20 - larger bonus but capped
    EXPECT_EQ(getLMRReduction(6, 20, defaultParams), 4); // base + depth + (20-8)/4 = 5, capped at 4
}

// Test reduction capping
TEST_F(LMRTest, ReductionCapping) {
    // Should cap at depth-2 (but at least 1)
    EXPECT_EQ(getLMRReduction(3, 20, defaultParams), 1); // capped at depth-2 = 1
    EXPECT_EQ(getLMRReduction(4, 20, defaultParams), 2); // capped at depth-2 = 2
    EXPECT_EQ(getLMRReduction(7, 20, defaultParams), 5); // capped at depth-2 = 5
    
    // Even with huge move numbers, should respect cap
    EXPECT_EQ(getLMRReduction(5, 100, defaultParams), 3); // capped at depth-2 = 3
}

// Test conditions that prevent reduction
TEST_F(LMRTest, NoReductionConditions) {
    // LMR disabled
    SearchData::LMRParams disabledParams = defaultParams;
    disabledParams.enabled = false;
    EXPECT_EQ(getLMRReduction(6, 10, disabledParams), 0);
    
    // Depth too shallow
    EXPECT_EQ(getLMRReduction(2, 10, defaultParams), 0); // depth < minDepth
    EXPECT_EQ(getLMRReduction(1, 10, defaultParams), 0); // very shallow
    
    // Move too early
    EXPECT_EQ(getLMRReduction(6, 1, defaultParams), 0); // first move
    EXPECT_EQ(getLMRReduction(6, 2, defaultParams), 0); // second move
    EXPECT_EQ(getLMRReduction(6, 3, defaultParams), 0); // third move
}

// Test edge cases
TEST_F(LMRTest, EdgeCases) {
    // Zero or negative inputs
    EXPECT_EQ(getLMRReduction(0, 10, defaultParams), 0);
    EXPECT_EQ(getLMRReduction(-1, 10, defaultParams), 0);
    EXPECT_EQ(getLMRReduction(6, 0, defaultParams), 0);
    EXPECT_EQ(getLMRReduction(6, -1, defaultParams), 0);
    
    // Depth 1 should return 0 (can't reduce below depth 1)
    EXPECT_EQ(getLMRReduction(1, 10, defaultParams), 0);
}

// Test different parameter configurations
TEST_F(LMRTest, DifferentParameters) {
    // More aggressive parameters
    SearchData::LMRParams aggressiveParams;
    aggressiveParams.enabled = true;
    aggressiveParams.minDepth = 2;
    aggressiveParams.minMoveNumber = 3;
    aggressiveParams.baseReduction = 2;
    aggressiveParams.depthFactor = 2;
    
    EXPECT_EQ(getLMRReduction(4, 3, aggressiveParams), 3); // 2 + (4-2)/2 = 3
    EXPECT_EQ(getLMRReduction(6, 5, aggressiveParams), 4); // 2 + (6-2)/2 = 4
    
    // Conservative parameters
    SearchData::LMRParams conservativeParams;
    conservativeParams.enabled = true;
    conservativeParams.minDepth = 4;
    conservativeParams.minMoveNumber = 6;
    conservativeParams.baseReduction = 0;
    conservativeParams.depthFactor = 4;
    
    EXPECT_EQ(getLMRReduction(4, 6, conservativeParams), 0); // 0 + (4-4)/4 = 0
    EXPECT_EQ(getLMRReduction(8, 6, conservativeParams), 1); // 0 + (8-4)/4 = 1
}

// Test shouldReduceMove function
TEST_F(LMRTest, ShouldReduceMove) {
    // Normal quiet move that should be reduced
    EXPECT_TRUE(shouldReduceMove(6, 5, false, false, false, false, defaultParams));
    
    // Captures should not be reduced
    EXPECT_FALSE(shouldReduceMove(6, 5, true, false, false, false, defaultParams));
    
    // Moves when in check should not be reduced
    EXPECT_FALSE(shouldReduceMove(6, 5, false, true, false, false, defaultParams));
    
    // Moves that give check should not be reduced
    EXPECT_FALSE(shouldReduceMove(6, 5, false, false, true, false, defaultParams));
    
    // Early moves should not be reduced
    EXPECT_FALSE(shouldReduceMove(6, 1, false, false, false, false, defaultParams));
    EXPECT_FALSE(shouldReduceMove(6, 2, false, false, false, false, defaultParams));
    EXPECT_FALSE(shouldReduceMove(6, 3, false, false, false, false, defaultParams));
    
    // Shallow depth should not be reduced
    EXPECT_FALSE(shouldReduceMove(2, 5, false, false, false, false, defaultParams));
    
    // PV nodes (Phase 2: treated same as non-PV for now)
    EXPECT_TRUE(shouldReduceMove(6, 5, false, false, false, true, defaultParams));
}

// Test realistic game scenarios
TEST_F(LMRTest, RealisticScenarios) {
    // Middlegame position, depth 7, various move numbers
    int depth = 7;
    
    // First few moves - no reduction
    EXPECT_EQ(getLMRReduction(depth, 1, defaultParams), 0);
    EXPECT_EQ(getLMRReduction(depth, 2, defaultParams), 0);
    EXPECT_EQ(getLMRReduction(depth, 3, defaultParams), 0);
    
    // Move 4 - minimum reduction
    EXPECT_EQ(getLMRReduction(depth, 4, defaultParams), 2); // 1 + (7-3)/3 = 2
    
    // Move 8 - still moderate
    EXPECT_EQ(getLMRReduction(depth, 8, defaultParams), 2);
    
    // Move 13 - with late move bonus
    EXPECT_EQ(getLMRReduction(depth, 13, defaultParams), 3); // 2 + (13-8)/4 = 3
    
    // Move 20 - significant reduction but capped
    EXPECT_EQ(getLMRReduction(depth, 20, defaultParams), 4); // Would be 5, but capped at 5
    
    // Move 30 - heavily capped
    EXPECT_EQ(getLMRReduction(depth, 30, defaultParams), 5); // Capped at depth-2
}

// Test that formula matches specification
TEST_F(LMRTest, FormulaVerification) {
    // Verify the exact formula from the plan:
    // reduction = baseReduction + (depth - minDepth) / depthFactor
    // if (moveNumber > 8) reduction += (moveNumber - 8) / 4
    // cap at min(reduction, depth - 2)
    
    int depth = 10;
    int moveNumber = 15;
    
    // Manual calculation
    int expected = defaultParams.baseReduction; // 1
    expected += (depth - defaultParams.minDepth) / defaultParams.depthFactor; // + (10-3)/3 = 2
    expected += (moveNumber - 8) / 4; // + (15-8)/4 = 1
    // Total: 1 + 2 + 1 = 4
    expected = std::min(expected, depth - 2); // min(4, 8) = 4
    
    EXPECT_EQ(getLMRReduction(depth, moveNumber, defaultParams), expected);
}