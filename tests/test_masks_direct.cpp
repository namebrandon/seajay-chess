#include <iostream>
#include "core/types.h"
#include "core/bitboard.h"

using namespace seajay;

// Direct implementation of computeRookMask
Bitboard computeRookMask(Square sq) {
    Bitboard mask = 0;
    int f = static_cast<int>(fileOf(sq));
    int r = static_cast<int>(rankOf(sq));
    
    // North ray (exclude rank 8)
    for (int r2 = r + 1; r2 < 7; ++r2) {
        mask |= squareBB(makeSquare(static_cast<File>(f), static_cast<Rank>(r2)));
    }
    
    // South ray (exclude rank 1)
    for (int r2 = r - 1; r2 > 0; --r2) {
        mask |= squareBB(makeSquare(static_cast<File>(f), static_cast<Rank>(r2)));
    }
    
    // East ray (exclude file H)
    for (int f2 = f + 1; f2 < 7; ++f2) {
        mask |= squareBB(makeSquare(static_cast<File>(f2), static_cast<Rank>(r)));
    }
    
    // West ray (exclude file A)
    for (int f2 = f - 1; f2 > 0; --f2) {
        mask |= squareBB(makeSquare(static_cast<File>(f2), static_cast<Rank>(r)));
    }
    
    return mask;
}

int main() {
    std::cout << "Testing mask computation directly\n";
    
    // Test rook mask for D4
    Square sq = D4;
    std::cout << "\nTesting Rook mask for D4:\n";
    
    Bitboard mask = computeRookMask(sq);
    std::cout << "Mask = 0x" << std::hex << mask << std::dec << "\n";
    std::cout << "Bit count = " << popCount(mask) << "\n";
    std::cout << "Expected = 12\n";
    
    // Test corner
    sq = A1;
    std::cout << "\nTesting Rook mask for A1:\n";
    mask = computeRookMask(sq);
    std::cout << "Bit count = " << popCount(mask) << "\n";
    std::cout << "Expected = 12\n";
    
    // Test edge
    sq = A4;
    std::cout << "\nTesting Rook mask for A4:\n";
    mask = computeRookMask(sq);
    std::cout << "Bit count = " << popCount(mask) << "\n";
    std::cout << "Expected = 11\n";
    
    return 0;
}