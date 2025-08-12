/**
 * Test computeRookMask directly
 */

#include "../src/core/types.h"
#include "../src/core/bitboard.h"
#include "../src/core/magic_bitboards.h"
#include <iostream>

using namespace seajay;

int main() {
    std::cout << "Testing computeRookMask...\n";
    
    Square sq = D4;
    std::cout << "Calling computeRookMask(" << (int)sq << ")...\n" << std::flush;
    
    Bitboard mask = magic::computeRookMask(sq);
    
    std::cout << "Result: " << std::hex << mask << std::dec << "\n";
    std::cout << "Popcount: " << popCount(mask) << "\n";
    
    return 0;
}