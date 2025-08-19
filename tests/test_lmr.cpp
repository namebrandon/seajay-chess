#include <gtest/gtest.h>
#include "../src/search/lmr.h"
#include "../src/search/types.h"

using namespace seajay::search;

class LMRTest : public ::testing::Test {
protected:
    SearchData::LMRParams defaultParams;
    
    void SetUp() override {
        // Set default parameters matching Phase 2 requirements
        defaultParams.enabled = true;
        defaultParams.minDepth = 3;
        defaultParams.minMoveNumber = 4;
        defaultParams.baseReduction = 1;
        defaultParams.depthFactor = 100;
    }
};

// ============================================================================
// getLMRReduction() tests
// ============================================================================

TEST_F(LMRTest, DisabledLMRReturnsZero) {
    defaultParams.enabled = false;
    EXPECT_EQ(0, getLMRReduction(5, 5, defaultParams));
    EXPECT_EQ(0, getLMRReduction(10, 10, defaultParams));
}

TEST_F(LMRTest, ShallowDepthReturnsZero) {
    // depth < minDepth should return 0
    EXPECT_EQ(0, getLMRReduction(1, 5, defaultParams));
    EXPECT_EQ(0, getLMRReduction(2, 5, defaultParams));
    
    // depth == minDepth should allow reduction
    EXPECT_GT(getLMRReduction(3, 5, defaultParams), 0);
}

TEST_F(LMRTest, EarlyMovesReturnZero) {
    // Moves before minMoveNumber should return 0
    EXPECT_EQ(0, getLMRReduction(5, 1, defaultParams));
    EXPECT_EQ(0, getLMRReduction(5, 2, defaultParams));
    EXPECT_EQ(0, getLMRReduction(5, 3, defaultParams));
    
    // Move at minMoveNumber should get reduction
    EXPECT_GT(getLMRReduction(5, 4, defaultParams), 0);
}

TEST_F(LMRTest, InvalidInputsReturnZero) {
    // Negative depth
    EXPECT_EQ(0, getLMRReduction(-1, 5, defaultParams));
    
    // Zero or negative move number
    EXPECT_EQ(0, getLMRReduction(5, 0, defaultParams));
    EXPECT_EQ(0, getLMRReduction(5, -1, defaultParams));
}

TEST_F(LMRTest, BasicFormulaCalculation) {
    // At minDepth, only base reduction
    EXPECT_EQ(1, getLMRReduction(3, 4, defaultParams));
    
    // depth=103 (3 + 100), should add 1 from depth factor
    EXPECT_EQ(2, getLMRReduction(103, 4, defaultParams));
    
    // depth=203, should add 2 from depth factor
    EXPECT_EQ(3, getLMRReduction(203, 4, defaultParams));
}

TEST_F(LMRTest, VeryLateMovesGetExtraReduction) {
    // Move 8: no extra reduction
    int reduction8 = getLMRReduction(5, 8, defaultParams);
    
    // Move 9: should start getting extra reduction (but (9-8)/4 = 0)
    int reduction9 = getLMRReduction(5, 9, defaultParams);
    EXPECT_EQ(reduction8, reduction9); // Same because integer division
    
    // Move 12: (12-8)/4 = 1, should get +1 extra
    int reduction12 = getLMRReduction(5, 12, defaultParams);
    EXPECT_EQ(reduction8, reduction12); // Still same due to integer division
    
    // Move 13: (13-8)/4 = 1, should get +1 extra
    int reduction13 = getLMRReduction(5, 13, defaultParams);
    EXPECT_EQ(reduction8 + 1, reduction13);
    
    // Move 20: (20-8)/4 = 3, should get +3 extra
    int reduction20 = getLMRReduction(5, 20, defaultParams);
    EXPECT_EQ(reduction8 + 3, reduction20);
}

TEST_F(LMRTest, ReductionCapping) {
    // Large depth but should be capped at depth-2
    defaultParams.baseReduction = 100; // Force large reduction
    
    // depth=10, max reduction should be 8 (10-2)
    EXPECT_EQ(8, getLMRReduction(10, 4, defaultParams));
    
    // depth=5, max reduction should be 3 (5-2)
    EXPECT_EQ(3, getLMRReduction(5, 4, defaultParams));
    
    // depth=3, max reduction should be 1 (3-2)
    EXPECT_EQ(1, getLMRReduction(3, 4, defaultParams));
}

TEST_F(LMRTest, DifferentParameterConfigurations) {
    // Test with more aggressive parameters
    defaultParams.baseReduction = 2;
    defaultParams.depthFactor = 50; // More aggressive depth scaling
    
    // depth=53 (3 + 50), should be base(2) + 1
    EXPECT_EQ(3, getLMRReduction(53, 4, defaultParams));
    
    // Test with less aggressive parameters
    defaultParams.baseReduction = 0;
    defaultParams.depthFactor = 200;
    
    // depth=3, should be 0 (no base, no depth bonus yet)
    EXPECT_EQ(0, getLMRReduction(3, 4, defaultParams));
    
    // depth=203, should be 1 (200 ply gives 1 reduction)
    EXPECT_EQ(1, getLMRReduction(203, 4, defaultParams));
}

TEST_F(LMRTest, ZeroDepthFactor) {
    // depthFactor=0 should only use base reduction (no division by zero)
    defaultParams.depthFactor = 0;
    
    EXPECT_EQ(1, getLMRReduction(3, 4, defaultParams));
    EXPECT_EQ(1, getLMRReduction(10, 4, defaultParams));
    EXPECT_EQ(1, getLMRReduction(100, 4, defaultParams));
}

// ============================================================================
// shouldReduceMove() tests
// ============================================================================

TEST_F(LMRTest, ShouldNotReduceWhenDisabled) {
    defaultParams.enabled = false;
    EXPECT_FALSE(shouldReduceMove(5, 5, false, false, false, false, defaultParams));
}

TEST_F(LMRTest, ShouldNotReduceShallowDepths) {
    EXPECT_FALSE(shouldReduceMove(1, 5, false, false, false, false, defaultParams));
    EXPECT_FALSE(shouldReduceMove(2, 5, false, false, false, false, defaultParams));
    
    // At minDepth, should allow
    EXPECT_TRUE(shouldReduceMove(3, 5, false, false, false, false, defaultParams));
}

TEST_F(LMRTest, ShouldNotReduceEarlyMoves) {
    EXPECT_FALSE(shouldReduceMove(5, 1, false, false, false, false, defaultParams));
    EXPECT_FALSE(shouldReduceMove(5, 2, false, false, false, false, defaultParams));
    EXPECT_FALSE(shouldReduceMove(5, 3, false, false, false, false, defaultParams));
    
    // At minMoveNumber, should allow
    EXPECT_TRUE(shouldReduceMove(5, 4, false, false, false, false, defaultParams));
}

TEST_F(LMRTest, ShouldNotReduceWhenInCheck) {
    // In check = true
    EXPECT_FALSE(shouldReduceMove(5, 5, false, true, false, false, defaultParams));
}

TEST_F(LMRTest, ShouldNotReduceMovesThatGiveCheck) {
    // Gives check = true
    EXPECT_FALSE(shouldReduceMove(5, 5, false, false, true, false, defaultParams));
}

TEST_F(LMRTest, ShouldNotReduceCaptures) {
    // Is capture = true
    EXPECT_FALSE(shouldReduceMove(5, 5, true, false, false, false, defaultParams));
}

TEST_F(LMRTest, ShouldNotReducePVNodes) {
    // Is PV node = true
    EXPECT_FALSE(shouldReduceMove(5, 5, false, false, false, true, defaultParams));
}

TEST_F(LMRTest, ShouldReduceQuietLateMoves) {
    // Perfect candidate: quiet, late move, not in check, not giving check, not PV
    EXPECT_TRUE(shouldReduceMove(5, 5, false, false, false, false, defaultParams));
    EXPECT_TRUE(shouldReduceMove(10, 10, false, false, false, false, defaultParams));
    EXPECT_TRUE(shouldReduceMove(3, 4, false, false, false, false, defaultParams));
}

TEST_F(LMRTest, ComplexScenarios) {
    // Multiple disqualifying factors
    EXPECT_FALSE(shouldReduceMove(5, 5, true, true, false, false, defaultParams));  // capture + in check
    EXPECT_FALSE(shouldReduceMove(5, 5, false, false, true, true, defaultParams));  // gives check + PV
    
    // Edge case: all conditions perfect except one
    EXPECT_FALSE(shouldReduceMove(5, 3, false, false, false, false, defaultParams)); // too early
    EXPECT_FALSE(shouldReduceMove(2, 5, false, false, false, false, defaultParams)); // too shallow
}

// ============================================================================
// Integration test with realistic parameters
// ============================================================================

TEST_F(LMRTest, RealisticScenarioPhase2) {
    // Simulate a real search scenario at depth 7
    int depth = 7;
    
    // First few moves should not be reduced
    for (int move = 1; move <= 3; move++) {
        EXPECT_EQ(0, getLMRReduction(depth, move, defaultParams));
        EXPECT_FALSE(shouldReduceMove(depth, move, false, false, false, false, defaultParams));
    }
    
    // Moves 4-8 should get base reduction
    for (int move = 4; move <= 8; move++) {
        int reduction = getLMRReduction(depth, move, defaultParams);
        EXPECT_EQ(1, reduction); // base=1, depth factor won't kick in at depth 7
        EXPECT_TRUE(shouldReduceMove(depth, move, false, false, false, false, defaultParams));
    }
    
    // Very late moves (13+) should get extra reduction
    int reduction13 = getLMRReduction(depth, 13, defaultParams);
    EXPECT_EQ(2, reduction13); // base(1) + very_late(1)
    
    int reduction20 = getLMRReduction(depth, 20, defaultParams);
    EXPECT_EQ(4, reduction20); // base(1) + very_late(3)
    
    // But capped at depth-2 = 5
    int reduction100 = getLMRReduction(depth, 100, defaultParams);
    EXPECT_EQ(5, reduction100); // capped at depth(7) - 2
}