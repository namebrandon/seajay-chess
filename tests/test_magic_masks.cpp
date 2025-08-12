/**
 * Test program for validating magic bitboard mask generation
 * Part of Stage 10, Phase 1, Step 1A validation
 */

#include <iostream>
#include "core/magic_bitboards.h"

using namespace seajay;

int main() {
    std::cout << "=== MAGIC BITBOARD MASK VALIDATION ===\n";
    std::cout << "Stage 10, Phase 1, Step 1A\n\n";
    
    // Print detailed mask information
    magic::printMaskInfo();
    
    // Additional validation checks
    std::cout << "\n=== VALIDATION CHECKS ===\n";
    
    // Test specific known values
    bool allPassed = true;
    
    // Rook mask tests
    {
        Bitboard d4Mask = magic::computeRookMask(D4);
        int d4Bits = popCount(d4Mask);
        if (d4Bits != 10) {
            std::cout << "ERROR: Rook D4 mask has " << d4Bits << " bits, expected 10\n";
            allPassed = false;
        } else {
            std::cout << "PASS: Rook D4 mask has correct bit count (10)\n";
        }
        
        Bitboard a1Mask = magic::computeRookMask(A1);
        int a1Bits = popCount(a1Mask);
        if (a1Bits != 12) {
            std::cout << "ERROR: Rook A1 mask has " << a1Bits << " bits, expected 12\n";
            allPassed = false;
        } else {
            std::cout << "PASS: Rook A1 mask has correct bit count (12)\n";
        }
        
        Bitboard h8Mask = magic::computeRookMask(H8);
        int h8Bits = popCount(h8Mask);
        if (h8Bits != 12) {
            std::cout << "ERROR: Rook H8 mask has " << h8Bits << " bits, expected 12\n";
            allPassed = false;
        } else {
            std::cout << "PASS: Rook H8 mask has correct bit count (12)\n";
        }
    }
    
    // Bishop mask tests
    {
        Bitboard d4Mask = magic::computeBishopMask(D4);
        int d4Bits = popCount(d4Mask);
        if (d4Bits != 9) {
            std::cout << "ERROR: Bishop D4 mask has " << d4Bits << " bits, expected 9\n";
            allPassed = false;
        } else {
            std::cout << "PASS: Bishop D4 mask has correct bit count (9)\n";
        }
        
        Bitboard e5Mask = magic::computeBishopMask(E5);
        int e5Bits = popCount(e5Mask);
        if (e5Bits != 9) {
            std::cout << "ERROR: Bishop E5 mask has " << e5Bits << " bits, expected 9\n";
            allPassed = false;
        } else {
            std::cout << "PASS: Bishop E5 mask has correct bit count (9)\n";
        }
        
        Bitboard a1Mask = magic::computeBishopMask(A1);
        int a1Bits = popCount(a1Mask);
        if (a1Bits != 6) {
            std::cout << "ERROR: Bishop A1 mask has " << a1Bits << " bits, expected 6\n";
            allPassed = false;
        } else {
            std::cout << "PASS: Bishop A1 (corner) mask has correct bit count (6)\n";
        }
        
        Bitboard h8Mask = magic::computeBishopMask(H8);
        int h8Bits = popCount(h8Mask);
        if (h8Bits != 6) {
            std::cout << "ERROR: Bishop H8 mask has " << h8Bits << " bits, expected 6\n";
            allPassed = false;
        } else {
            std::cout << "PASS: Bishop H8 (corner) mask has correct bit count (6)\n";
        }
    }
    
    // Test that edges are excluded
    std::cout << "\n=== EDGE EXCLUSION TEST ===\n";
    
    // For a rook on D4, the mask should NOT include rank 1, rank 8, file A, or file H
    Bitboard d4RookMask = magic::computeRookMask(D4);
    if (d4RookMask & RANK_1_BB) {
        std::cout << "ERROR: Rook D4 mask includes rank 1 (should be excluded)\n";
        allPassed = false;
    } else {
        std::cout << "PASS: Rook D4 mask excludes rank 1\n";
    }
    
    if (d4RookMask & RANK_8_BB) {
        std::cout << "ERROR: Rook D4 mask includes rank 8 (should be excluded)\n";
        allPassed = false;
    } else {
        std::cout << "PASS: Rook D4 mask excludes rank 8\n";
    }
    
    if (d4RookMask & FILE_A_BB) {
        std::cout << "ERROR: Rook D4 mask includes file A (should be excluded)\n";
        allPassed = false;
    } else {
        std::cout << "PASS: Rook D4 mask excludes file A\n";
    }
    
    if (d4RookMask & FILE_H_BB) {
        std::cout << "ERROR: Rook D4 mask includes file H (should be excluded)\n";
        allPassed = false;
    } else {
        std::cout << "PASS: Rook D4 mask excludes file H\n";
    }
    
    // Test indexToOccupancy function
    std::cout << "\n=== INDEX TO OCCUPANCY TEST ===\n";
    
    Bitboard testMask = magic::computeRookMask(D4);
    int numBits = popCount(testMask);
    std::cout << "Rook D4 mask has " << numBits << " bits\n";
    std::cout << "This means " << (1 << numBits) << " possible occupancy patterns\n";
    
    // Test a few patterns
    Bitboard occ0 = magic::indexToOccupancy(0, testMask);
    Bitboard occMax = magic::indexToOccupancy((1 << numBits) - 1, testMask);
    
    if (occ0 != 0) {
        std::cout << "ERROR: Index 0 should produce empty occupancy\n";
        allPassed = false;
    } else {
        std::cout << "PASS: Index 0 produces empty occupancy\n";
    }
    
    if (occMax != testMask) {
        std::cout << "ERROR: Max index should produce full mask\n";
        allPassed = false;
    } else {
        std::cout << "PASS: Max index produces full mask\n";
    }
    
    // Final result
    std::cout << "\n=== FINAL RESULT ===\n";
    if (allPassed) {
        std::cout << "✓ ALL TESTS PASSED - Step 1A Complete!\n";
        std::cout << "✓ Blocker masks correctly exclude edge squares\n";
        std::cout << "✓ Bit counts match expected values for all squares\n";
        std::cout << "✓ Ready to proceed to Step 1B (Import Magic Numbers)\n";
    } else {
        std::cout << "✗ SOME TESTS FAILED - Please fix before proceeding\n";
    }
    
    return allPassed ? 0 : 1;
}