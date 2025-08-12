#include <iostream>
#include "core/types.h"
#include "core/bitboard.h"

using namespace seajay;

int main() {
    std::cout << "Testing simple mask computation\n";
    
    // Test squareBB function first
    Bitboard bb = squareBB(D4);
    std::cout << "squareBB(D4) = 0x" << std::hex << bb << std::dec << "\n";
    
    // Test file and rank functions
    File f = fileOf(D4);
    Rank r = rankOf(D4);
    std::cout << "D4: file=" << (int)f << ", rank=" << (int)r << "\n";
    
    // Simple mask computation
    Bitboard mask = 0;
    for (int r2 = r + 1; r2 < 7; ++r2) {
        Square sq = makeSquare(f, r2);
        std::cout << "Adding square: " << (int)sq << "\n";
        mask |= squareBB(sq);
    }
    
    std::cout << "Mask = 0x" << std::hex << mask << std::dec << "\n";
    std::cout << "Bit count = " << popCount(mask) << "\n";
    
    return 0;
}