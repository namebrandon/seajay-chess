// Unit test for IterativeSearchData basic methods
// Part of Stage 13, Deliverable 1.1c

#include "../src/search/iterative_search_data.h"
#include <iostream>
#include <cassert>

using namespace seajay;
using namespace seajay::search;

void testRecordIteration() {
    IterativeSearchData data;
    
    // Initially no iterations
    assert(!data.hasIterations());
    assert(data.getIterationCount() == 0);
    
    // Record first iteration
    IterationInfo iter1;
    iter1.depth = 1;
    iter1.nodes = 100;
    iter1.score = eval::Score(50);
    iter1.bestMove = makeMove(E2, E4);
    iter1.elapsed = 10;
    
    data.recordIteration(iter1);
    
    // Check it was recorded
    assert(data.hasIterations());
    assert(data.getIterationCount() == 1);
    
    const auto& last = data.getLastIteration();
    assert(last.depth == 1);
    assert(last.nodes == 100);
    assert(last.score.value() == 50);
    assert(last.bestMove == makeMove(E2, E4));
    assert(last.elapsed == 10);
    
    // Record second iteration
    IterationInfo iter2;
    iter2.depth = 2;
    iter2.nodes = 500;
    iter2.score = eval::Score(30);
    iter2.bestMove = makeMove(D2, D4);
    iter2.elapsed = 25;
    
    data.recordIteration(iter2);
    
    // Check both are recorded
    assert(data.getIterationCount() == 2);
    
    const auto& last2 = data.getLastIteration();
    assert(last2.depth == 2);
    assert(last2.nodes == 500);
    
    // Check we can get specific iterations
    const auto& first = data.getIteration(0);
    assert(first.depth == 1);
    assert(first.nodes == 100);
    
    const auto& second = data.getIteration(1);
    assert(second.depth == 2);
    assert(second.nodes == 500);
    
    std::cout << "testRecordIteration PASSED\n";
}

void testReset() {
    IterativeSearchData data;
    
    // Add some iterations
    for (int i = 1; i <= 5; ++i) {
        IterationInfo iter;
        iter.depth = i;
        iter.nodes = i * 100;
        data.recordIteration(iter);
    }
    
    assert(data.getIterationCount() == 5);
    data.nodes = 1000;  // Set base class field
    
    // Reset
    data.reset();
    
    // Check everything is cleared
    assert(data.getIterationCount() == 0);
    assert(!data.hasIterations());
    assert(data.nodes == 0);  // Base class reset
    
    // Check iterations are cleared
    const auto& last = data.getLastIteration();
    assert(last.depth == 0);  // Should return empty iteration
    
    std::cout << "testReset PASSED\n";
}

void testBoundaryConditions() {
    IterativeSearchData data;
    
    // Test getting from empty
    const auto& empty = data.getLastIteration();
    assert(empty.depth == 0);
    assert(empty.nodes == 0);
    
    const auto& outOfBounds = data.getIteration(100);
    assert(outOfBounds.depth == 0);
    
    // Fill to maximum
    for (size_t i = 0; i < IterativeSearchData::MAX_ITERATIONS; ++i) {
        IterationInfo iter;
        iter.depth = static_cast<int>(i + 1);
        data.recordIteration(iter);
    }
    
    assert(data.getIterationCount() == IterativeSearchData::MAX_ITERATIONS);
    
    // Try to add one more (should be ignored)
    IterationInfo extra;
    extra.depth = 999;
    data.recordIteration(extra);
    
    // Should still be at max
    assert(data.getIterationCount() == IterativeSearchData::MAX_ITERATIONS);
    
    // Last should not be the extra one
    const auto& last = data.getLastIteration();
    assert(last.depth != 999);
    assert(last.depth == static_cast<int>(IterativeSearchData::MAX_ITERATIONS));
    
    std::cout << "testBoundaryConditions PASSED\n";
}

int main() {
    std::cout << "Testing IterativeSearchData basic methods...\n";
    
    testRecordIteration();
    testReset();
    testBoundaryConditions();
    
    std::cout << "\nAll tests PASSED!\n";
    return 0;
}