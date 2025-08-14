// Stage 13, Deliverable 2.1d: Test stability tracking structure

#include <iostream>
#include <cassert>
#include "../src/search/iterative_search_data.h"

using namespace seajay;
using namespace seajay::search;

void testStabilityFieldsExist() {
    std::cout << "Testing stability fields exist..." << std::endl;
    
    IterativeSearchData data;
    
    // Check move stability fields
    assert(data.m_stableBestMove == NO_MOVE);
    assert(data.m_stabilityCount == 0);
    assert(data.m_requiredStability == 2);
    assert(data.m_positionStable == false);
    
    // Check score stability fields
    assert(data.m_stableScore == eval::Score::zero());
    assert(data.m_scoreStabilityCount == 0);
    assert(data.m_scoreWindow == eval::Score(10));
    
    std::cout << "  ✓ All stability fields present with correct defaults" << std::endl;
}

void testStabilityMethodsExist() {
    std::cout << "Testing stability methods exist..." << std::endl;
    
    IterativeSearchData data;
    
    // Test updateStability method (stub)
    IterationInfo iter;
    iter.depth = 1;
    iter.bestMove = makeMove(E2, E4);
    iter.score = eval::Score(100);
    
    // Should compile and not crash (stub implementation)
    data.updateStability(iter);
    
    // Test isPositionStable method (stub)
    bool stable = data.isPositionStable();
    assert(!stable);  // Default should be false
    
    // Test getStabilityFactor method (stub)
    double factor = data.getStabilityFactor();
    assert(factor == 1.0);  // Stub returns 1.0
    
    // Test shouldExtendDueToInstability method (stub)
    bool extend = data.shouldExtendDueToInstability();
    assert(!extend);  // Stub returns false
    
    std::cout << "  ✓ All stability methods callable (stubs)" << std::endl;
}

void testResetClearsStability() {
    std::cout << "Testing reset clears stability fields..." << std::endl;
    
    IterativeSearchData data;
    
    // Modify some fields
    data.m_stabilityCount = 5;
    data.m_positionStable = true;
    data.m_stableBestMove = makeMove(D2, D4);
    data.m_scoreStabilityCount = 3;
    data.m_stableScore = eval::Score(200);
    
    // Reset should clear everything
    data.reset();
    
    assert(data.m_stableBestMove == NO_MOVE);
    assert(data.m_stabilityCount == 0);
    assert(data.m_positionStable == false);
    assert(data.m_stableScore == eval::Score::zero());
    assert(data.m_scoreStabilityCount == 0);
    
    std::cout << "  ✓ Reset properly clears stability fields" << std::endl;
}

void testStabilityIntegrationWithIterations() {
    std::cout << "Testing stability integration with iterations..." << std::endl;
    
    IterativeSearchData data;
    
    // Record some iterations
    for (int depth = 1; depth <= 3; depth++) {
        IterationInfo iter;
        iter.depth = depth;
        iter.bestMove = makeMove(E2, E4);  // Same move each time
        iter.score = eval::Score(50 + depth * 10);  // Slightly different scores
        iter.nodes = 1000 * depth;
        iter.elapsed = 10 * depth;
        iter.moveStability = depth;  // Increasing stability
        
        data.recordIteration(iter);
        
        // Call updateStability (stub for now)
        data.updateStability(iter);
    }
    
    // Check we can access both iteration data and stability methods
    assert(data.getIterationCount() == 3);
    assert(data.getLastIteration().depth == 3);
    
    // Stability methods should still work (even as stubs)
    bool stable = data.isPositionStable();
    double factor = data.getStabilityFactor();
    
    std::cout << "  ✓ Stability and iteration systems coexist properly" << std::endl;
}

void testTimeManagementFields() {
    std::cout << "Testing time management fields in structure..." << std::endl;
    
    IterativeSearchData data;
    
    // Check time limit fields exist
    assert(data.m_softLimit == 0);
    assert(data.m_hardLimit == 0);
    assert(data.m_optimumTime == 0);
    
    // Set some values
    data.m_optimumTime = 1000;
    data.m_softLimit = 1000;
    data.m_hardLimit = 3000;
    
    assert(data.m_optimumTime == 1000);
    assert(data.m_softLimit == 1000);
    assert(data.m_hardLimit == 3000);
    
    // Reset should clear them
    data.reset();
    assert(data.m_softLimit == 0);
    assert(data.m_hardLimit == 0);
    assert(data.m_optimumTime == 0);
    
    std::cout << "  ✓ Time management fields integrated properly" << std::endl;
}

void testStructureSizeReasonable() {
    std::cout << "Testing structure size is reasonable..." << std::endl;
    
    size_t baseSize = sizeof(SearchData);
    size_t enhancedSize = sizeof(IterativeSearchData);
    
    std::cout << "  SearchData size: " << baseSize << " bytes" << std::endl;
    std::cout << "  IterativeSearchData size: " << enhancedSize << " bytes" << std::endl;
    
    // Should not be unreasonably large
    assert(enhancedSize <= 16384);  // 16KB max (we have 64 iterations array)
    
    // Should be larger than base due to additions
    assert(enhancedSize > baseSize);
    
    std::cout << "  ✓ Structure size reasonable" << std::endl;
}

int main() {
    std::cout << "\n=== Stage 13, Deliverable 2.1d: Stability Tracking Structure Test ===" << std::endl;
    
    try {
        testStabilityFieldsExist();
        testStabilityMethodsExist();
        testResetClearsStability();
        testStabilityIntegrationWithIterations();
        testTimeManagementFields();
        testStructureSizeReasonable();
        
        std::cout << "\n✓ All tests passed!" << std::endl;
        std::cout << "Deliverable 2.1d COMPLETE: Stability tracking structure added successfully" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Test failed with exception: " << e.what() << std::endl;
        return 1;
    }
}