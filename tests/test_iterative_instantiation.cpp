// Simple test to verify IterativeSearchData can be instantiated
// Part of Stage 13, Deliverable 1.1b

#include "../src/search/iterative_search_data.h"
#include <iostream>
#include <cassert>

using namespace seajay::search;

int main() {
    // Test instantiation
    IterativeSearchData searchData;
    
    // Verify initial state
    assert(searchData.m_iterationCount == 0);
    assert(searchData.m_softLimit == 0);
    assert(searchData.m_hardLimit == 0);
    assert(searchData.m_optimumTime == 0);
    assert(searchData.m_stableBestMove == seajay::NO_MOVE);
    assert(searchData.m_stabilityCount == 0);
    
    // Verify we can access base class members
    assert(searchData.nodes == 0);
    assert(searchData.depth == 0);
    
    // Test reset
    searchData.nodes = 100;
    searchData.m_iterationCount = 5;
    searchData.reset();
    assert(searchData.nodes == 0);
    assert(searchData.m_iterationCount == 0);
    
    std::cout << "IterativeSearchData instantiation test PASSED\n";
    std::cout << "Size of IterativeSearchData: " << sizeof(IterativeSearchData) << " bytes\n";
    std::cout << "Size of IterationInfo: " << sizeof(IterationInfo) << " bytes\n";
    std::cout << "Total iteration array size: " << sizeof(IterationInfo) * IterativeSearchData::MAX_ITERATIONS << " bytes\n";
    
    return 0;
}