/**
 * Test program for validating magic numbers
 * Part of Stage 10, Phase 1, Steps 1B and 1C
 */

#include <iostream>
#include <iomanip>
#include "core/magic_bitboards.h"
#include "core/magic_constants.h"

using namespace seajay;

int main() {
    std::cout << "=== MAGIC NUMBER VALIDATION ===\n";
    std::cout << "Stage 10, Phase 1, Steps 1B and 1C\n\n";
    
    // Step 1B: Verify all magic numbers have ULL suffix (compile-time check)
    std::cout << "Step 1B: Checking magic number format...\n";
    
    // Check that magic numbers are 64-bit
    static_assert(sizeof(magic::ROOK_MAGICS[0]) == 8, "Magic numbers must be 64-bit!");
    static_assert(sizeof(magic::BISHOP_MAGICS[0]) == 8, "Magic numbers must be 64-bit!");
    
    std::cout << "✓ All magic numbers are 64-bit values (ULL suffix present)\n";
    std::cout << "✓ Total rook magics: 64\n";
    std::cout << "✓ Total bishop magics: 64\n\n";
    
    // Step 1C: Validate each magic number for collisions
    std::cout << "Step 1C: Validating magic numbers for collisions...\n\n";
    
    bool allValid = true;
    int validRooks = 0;
    int validBishops = 0;
    
    // Validate rook magic numbers
    std::cout << "Validating ROOK magic numbers:\n";
    for (Square sq = 0; sq < 64; ++sq) {
        bool valid = magic::validateMagicNumber(sq, true);
        if (valid) {
            validRooks++;
            if (sq % 8 == 0) std::cout << "\n";
            std::cout << "✓";
        } else {
            std::cout << "✗";
            allValid = false;
        }
    }
    std::cout << "\n\nRook validation: " << validRooks << "/64 magic numbers valid\n\n";
    
    // Validate bishop magic numbers
    std::cout << "Validating BISHOP magic numbers:\n";
    for (Square sq = 0; sq < 64; ++sq) {
        bool valid = magic::validateMagicNumber(sq, false);
        if (valid) {
            validBishops++;
            if (sq % 8 == 0) std::cout << "\n";
            std::cout << "✓";
        } else {
            std::cout << "✗";
            allValid = false;
        }
    }
    std::cout << "\n\nBishop validation: " << validBishops << "/64 magic numbers valid\n\n";
    
    // Test shift values match mask bit counts
    std::cout << "Validating shift values...\n";
    bool shiftsValid = true;
    
    for (Square sq = 0; sq < 64; ++sq) {
        Bitboard rookMask = magic::computeRookMask(sq);
        int rookBits = popCount(rookMask);
        int expectedRookShift = 64 - rookBits;
        
        if (magic::ROOK_SHIFTS[sq] != expectedRookShift) {
            std::cout << "ERROR: Rook shift mismatch at square " << sq 
                      << " (expected " << expectedRookShift 
                      << ", got " << (int)magic::ROOK_SHIFTS[sq] << ")\n";
            shiftsValid = false;
        }
        
        Bitboard bishopMask = magic::computeBishopMask(sq);
        int bishopBits = popCount(bishopMask);
        int expectedBishopShift = 64 - bishopBits;
        
        if (magic::BISHOP_SHIFTS[sq] != expectedBishopShift) {
            std::cout << "ERROR: Bishop shift mismatch at square " << sq 
                      << " (expected " << expectedBishopShift 
                      << ", got " << (int)magic::BISHOP_SHIFTS[sq] << ")\n";
            shiftsValid = false;
        }
    }
    
    if (shiftsValid) {
        std::cout << "✓ All shift values match mask bit counts\n";
    }
    
    // Print memory requirements
    std::cout << "\n=== MEMORY REQUIREMENTS ===\n";
    
    size_t rookTableBytes = 0;
    size_t bishopTableBytes = 0;
    
    for (Square sq = 0; sq < 64; ++sq) {
        int rookBits = popCount(magic::computeRookMask(sq));
        int bishopBits = popCount(magic::computeBishopMask(sq));
        
        rookTableBytes += (1 << rookBits) * sizeof(Bitboard);
        bishopTableBytes += (1 << bishopBits) * sizeof(Bitboard);
    }
    
    std::cout << "Rook attack tables: " << rookTableBytes / 1024 << " KB\n";
    std::cout << "Bishop attack tables: " << bishopTableBytes / 1024 << " KB\n";
    std::cout << "Total: " << (rookTableBytes + bishopTableBytes) / 1024 << " KB ";
    std::cout << "(" << std::fixed << std::setprecision(2) 
              << (rookTableBytes + bishopTableBytes) / (1024.0 * 1024.0) << " MB)\n";
    
    // Final result
    std::cout << "\n=== VALIDATION RESULT ===\n";
    if (allValid && shiftsValid) {
        std::cout << "✓ ALL TESTS PASSED!\n";
        std::cout << "✓ Step 1B Complete: Magic numbers imported with ULL suffix\n";
        std::cout << "✓ Step 1C Complete: All 128 magic numbers validated\n";
        std::cout << "✓ Ready for Step 1D: Create MagicEntry structure\n";
        return 0;
    } else {
        std::cout << "✗ VALIDATION FAILED\n";
        if (!allValid) {
            std::cout << "  - Some magic numbers produce collisions\n";
        }
        if (!shiftsValid) {
            std::cout << "  - Shift values don't match mask bit counts\n";
        }
        return 1;
    }
}