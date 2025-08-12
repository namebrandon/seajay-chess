/**
 * Simple test of mask computation
 */

#include "../src/core/types.h"
#include "../src/core/bitboard.h"
#include "../src/core/magic_bitboards.h"
#include <iostream>

using namespace seajay;

int main() {
    std::cout << "Testing mask computation...\n";
    
    // Test a single square
    Square sq = D4;
    std::cout << "Computing rook mask for D4...\n" << std::flush;
    
    Bitboard mask = magic::computeRookMask(sq);
    
    std::cout << "Mask computed! Popcount: " << popCount(mask) << "\n";
    std::cout << bitboardToString(mask) << "\n";
    
    return 0;
}