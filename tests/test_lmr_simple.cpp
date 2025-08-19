/**
 * Simple standalone test for LMR reduction formula
 * Can be compiled and run independently without GoogleTest
 */

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
    params.depthFactor = 3;
    
    // Test basic cases
    assert(getLMRReduction(3, 4, params) == 1);  // Minimum case
    assert(getLMRReduction(6, 4, params) == 2);  // Some depth
    assert(getLMRReduction(9, 4, params) == 3);  // More depth
    
    // Test late move bonus
    assert(getLMRReduction(6, 8, params) == 2);  // No bonus yet
    assert(getLMRReduction(6, 12, params) == 3); // Small bonus
    assert(getLMRReduction(6, 20, params) == 4); // Larger bonus (capped at depth-2)
    
    // Test capping
    assert(getLMRReduction(3, 100, params) == 1); // Capped at depth-2
    assert(getLMRReduction(5, 100, params) == 3); // Capped at depth-2
    
    std::cout << "Basic reduction tests passed!" << std::endl;
}

void testShouldReduce() {
    SearchData::LMRParams params;
    params.enabled = true;
    params.minDepth = 3;
    params.minMoveNumber = 4;
    params.baseReduction = 1;
    params.depthFactor = 3;
    
    // Should reduce quiet late moves
    assert(shouldReduceMove(6, 5, false, false, false, false, params) == true);
    
    // Should NOT reduce captures
    assert(shouldReduceMove(6, 5, true, false, false, false, params) == false);
    
    // Should NOT reduce when in check
    assert(shouldReduceMove(6, 5, false, true, false, false, params) == false);
    
    // Should NOT reduce moves that give check
    assert(shouldReduceMove(6, 5, false, false, true, false, params) == false);
    
    // Should NOT reduce early moves
    assert(shouldReduceMove(6, 2, false, false, false, false, params) == false);
    
    std::cout << "Should reduce tests passed!" << std::endl;
}

void printSampleReductions() {
    SearchData::LMRParams params;
    params.enabled = true;
    params.minDepth = 3;
    params.minMoveNumber = 4;
    params.baseReduction = 1;
    params.depthFactor = 3;
    
    std::cout << "\nSample reduction values (depth 7):" << std::endl;
    std::cout << "Move  4: " << getLMRReduction(7, 4, params) << " plies" << std::endl;
    std::cout << "Move  6: " << getLMRReduction(7, 6, params) << " plies" << std::endl;
    std::cout << "Move  8: " << getLMRReduction(7, 8, params) << " plies" << std::endl;
    std::cout << "Move 10: " << getLMRReduction(7, 10, params) << " plies" << std::endl;
    std::cout << "Move 13: " << getLMRReduction(7, 13, params) << " plies" << std::endl;
    std::cout << "Move 20: " << getLMRReduction(7, 20, params) << " plies" << std::endl;
    std::cout << "Move 30: " << getLMRReduction(7, 30, params) << " plies (capped)" << std::endl;
}

int main() {
    std::cout << "Testing LMR reduction formula..." << std::endl;
    
    testBasicReduction();
    testShouldReduce();
    printSampleReductions();
    
    std::cout << "\nAll tests passed successfully!" << std::endl;
    return 0;
}