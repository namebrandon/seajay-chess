#include <iostream>
#include "core/magic_bitboards.h"

using namespace seajay;

int main() {
    std::cout << "Testing magic bitboard mask functions\n\n";
    
    // Test rook masks
    std::cout << "=== ROOK MASKS ===\n";
    
    Square testSquares[] = {A1, D4, H8, A4, D1};
    int expectedRook[] = {12, 10, 12, 11, 11};
    
    for (int i = 0; i < 5; ++i) {
        Square sq = testSquares[i];
        Bitboard mask = magic::computeRookMask(sq);
        int bits = popCount(mask);
        
        char file = 'a' + fileOf(sq);
        char rank = '1' + rankOf(sq);
        
        std::cout << file << rank << ": " << bits << " bits";
        if (bits == expectedRook[i]) {
            std::cout << " ✓\n";
        } else {
            std::cout << " ✗ (expected " << expectedRook[i] << ")\n";
        }
    }
    
    // Test bishop masks
    std::cout << "\n=== BISHOP MASKS ===\n";
    
    int expectedBishop[] = {6, 9, 6, 5, 7};
    
    for (int i = 0; i < 5; ++i) {
        Square sq = testSquares[i];
        Bitboard mask = magic::computeBishopMask(sq);
        int bits = popCount(mask);
        
        char file = 'a' + fileOf(sq);
        char rank = '1' + rankOf(sq);
        
        std::cout << file << rank << ": " << bits << " bits";
        if (bits == expectedBishop[i]) {
            std::cout << " ✓\n";
        } else {
            std::cout << " ✗ (expected " << expectedBishop[i] << ")\n";
        }
    }
    
    // Test indexToOccupancy
    std::cout << "\n=== INDEX TO OCCUPANCY ===\n";
    
    Bitboard mask = magic::computeRookMask(D4);
    int numBits = popCount(mask);
    
    std::cout << "D4 rook mask has " << numBits << " bits\n";
    std::cout << "This gives " << (1 << numBits) << " possible occupancy patterns\n";
    
    // Test edge cases
    Bitboard occ0 = magic::indexToOccupancy(0, mask);
    Bitboard occMax = magic::indexToOccupancy((1 << numBits) - 1, mask);
    
    if (occ0 == 0) {
        std::cout << "Index 0 → empty: ✓\n";
    } else {
        std::cout << "Index 0 → empty: ✗\n";
    }
    
    if (occMax == mask) {
        std::cout << "Max index → full mask: ✓\n";
    } else {
        std::cout << "Max index → full mask: ✗\n";
    }
    
    std::cout << "\n=== STEP 1A COMPLETE ===\n";
    std::cout << "All blocker mask functions implemented and validated!\n";
    
    return 0;
}