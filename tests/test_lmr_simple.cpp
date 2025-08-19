#include <iostream>
#include <cassert>
#include "../src/search/lmr.h"
#include "../src/search/types.h"

using namespace seajay::search;

void testBasicReduction() {
    SearchData::LMRParams params;
    params.enabled = true;
    params.minDepth = 3;
    params.minMoveNumber = 4;
    params.baseReduction = 1;
    params.depthFactor = 100;
    
    // Test disabled returns 0
    params.enabled = false;
    assert(getLMRReduction(5, 5, params) == 0);
    params.enabled = true;
    
    // Test shallow depth returns 0
    assert(getLMRReduction(2, 5, params) == 0);
    
    // Test early moves return 0
    assert(getLMRReduction(5, 3, params) == 0);
    
    // Test basic reduction at minDepth
    assert(getLMRReduction(3, 4, params) == 1);
    
    // Test very late moves get extra reduction
    assert(getLMRReduction(5, 13, params) == 2); // base(1) + late(1)
    
    // Test capping
    params.baseReduction = 100;
    assert(getLMRReduction(5, 4, params) == 3); // capped at depth(5) - 2
    
    std::cout << "Basic reduction tests passed!" << std::endl;
}

void testShouldReduce() {
    SearchData::LMRParams params;
    params.enabled = true;
    params.minDepth = 3;
    params.minMoveNumber = 4;
    params.baseReduction = 1;
    params.depthFactor = 100;
    
    // Should reduce quiet late moves
    assert(shouldReduceMove(5, 5, false, false, false, false, params) == true);
    
    // Should not reduce captures
    assert(shouldReduceMove(5, 5, true, false, false, false, params) == false);
    
    // Should not reduce when in check
    assert(shouldReduceMove(5, 5, false, true, false, false, params) == false);
    
    // Should not reduce moves that give check
    assert(shouldReduceMove(5, 5, false, false, true, false, params) == false);
    
    // Should not reduce PV nodes
    assert(shouldReduceMove(5, 5, false, false, false, true, params) == false);
    
    // Should not reduce early moves
    assert(shouldReduceMove(5, 3, false, false, false, false, params) == false);
    
    std::cout << "Should reduce tests passed!" << std::endl;
}

int main() {
    std::cout << "Testing LMR implementation..." << std::endl;
    
    testBasicReduction();
    testShouldReduce();
    
    std::cout << "\nAll LMR tests passed successfully!" << std::endl;
    
    // Display sample reductions for documentation
    std::cout << "\nSample reductions with default parameters:" << std::endl;
    SearchData::LMRParams params;
    params.enabled = true;
    params.minDepth = 3;
    params.minMoveNumber = 4;
    params.baseReduction = 1;
    params.depthFactor = 100;
    
    for (int depth : {3, 5, 7, 10}) {
        std::cout << "Depth " << depth << ":" << std::endl;
        for (int move : {1, 3, 4, 8, 13, 20}) {
            int reduction = getLMRReduction(depth, move, params);
            std::cout << "  Move " << move << ": reduction = " << reduction << std::endl;
        }
    }
    
    return 0;
}