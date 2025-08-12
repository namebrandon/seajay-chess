/**
 * Test program for Step 1D: MagicEntry structure
 * Part of Stage 10, Phase 1, Step 1D
 */

#include <iostream>
#include <iomanip>
#include "core/magic_bitboards.h"
#include "core/magic_constants.h"

using namespace seajay;

int main() {
    std::cout << "=== STEP 1D: MAGICENTRY STRUCTURE TEST ===\n\n";
    
    // Initialize the magic system
    magic::initMagics();
    
    std::cout << "\n=== STRUCTURE DETAILS ===\n";
    std::cout << "sizeof(MagicEntry): " << sizeof(magic::MagicEntry) << " bytes\n";
    std::cout << "alignof(MagicEntry): " << alignof(magic::MagicEntry) << " bytes\n";
    
    // Check cache alignment
    if (alignof(magic::MagicEntry) == 64) {
        std::cout << "✓ MagicEntry is cache-line aligned (64 bytes)\n";
    } else {
        std::cout << "⚠ MagicEntry alignment is " << alignof(magic::MagicEntry) 
                  << " bytes (64 recommended for cache performance)\n";
    }
    
    // Verify structure contents for a few squares
    std::cout << "\n=== SAMPLE ENTRIES ===\n";
    
    Square testSquares[] = {A1, D4, H8};
    const char* names[] = {"A1", "D4", "H8"};
    
    for (int i = 0; i < 3; ++i) {
        Square sq = testSquares[i];
        std::cout << "\n" << names[i] << " Rook Entry:\n";
        std::cout << "  Mask bits: " << popCount(magic::rookMagics[sq].mask) << "\n";
        std::cout << "  Magic: 0x" << std::hex << magic::rookMagics[sq].magic << std::dec << "\n";
        std::cout << "  Shift: " << (int)magic::rookMagics[sq].shift << "\n";
        std::cout << "  Expected shift: " << (64 - popCount(magic::rookMagics[sq].mask)) << "\n";
        
        if (magic::rookMagics[sq].shift == 64 - popCount(magic::rookMagics[sq].mask)) {
            std::cout << "  ✓ Shift matches mask\n";
        } else {
            std::cout << "  ✗ Shift mismatch!\n";
        }
    }
    
    for (int i = 0; i < 3; ++i) {
        Square sq = testSquares[i];
        std::cout << "\n" << names[i] << " Bishop Entry:\n";
        std::cout << "  Mask bits: " << popCount(magic::bishopMagics[sq].mask) << "\n";
        std::cout << "  Magic: 0x" << std::hex << magic::bishopMagics[sq].magic << std::dec << "\n";
        std::cout << "  Shift: " << (int)magic::bishopMagics[sq].shift << "\n";
        std::cout << "  Expected shift: " << (64 - popCount(magic::bishopMagics[sq].mask)) << "\n";
        
        if (magic::bishopMagics[sq].shift == 64 - popCount(magic::bishopMagics[sq].mask)) {
            std::cout << "  ✓ Shift matches mask\n";
        } else {
            std::cout << "  ✗ Shift mismatch!\n";
        }
    }
    
    // Verify all entries are initialized correctly
    std::cout << "\n=== FULL VALIDATION ===\n";
    
    bool allValid = true;
    
    for (Square sq = 0; sq < 64; ++sq) {
        // Check rook entries
        if (magic::rookMagics[sq].magic != magic::ROOK_MAGICS[sq]) {
            std::cout << "ERROR: Rook magic mismatch at square " << sq << "\n";
            allValid = false;
        }
        if (magic::rookMagics[sq].shift != magic::ROOK_SHIFTS[sq]) {
            std::cout << "ERROR: Rook shift mismatch at square " << sq << "\n";
            allValid = false;
        }
        if (magic::rookMagics[sq].mask != magic::computeRookMask(sq)) {
            std::cout << "ERROR: Rook mask mismatch at square " << sq << "\n";
            allValid = false;
        }
        
        // Check bishop entries
        if (magic::bishopMagics[sq].magic != magic::BISHOP_MAGICS[sq]) {
            std::cout << "ERROR: Bishop magic mismatch at square " << sq << "\n";
            allValid = false;
        }
        if (magic::bishopMagics[sq].shift != magic::BISHOP_SHIFTS[sq]) {
            std::cout << "ERROR: Bishop shift mismatch at square " << sq << "\n";
            allValid = false;
        }
        if (magic::bishopMagics[sq].mask != magic::computeBishopMask(sq)) {
            std::cout << "ERROR: Bishop mask mismatch at square " << sq << "\n";
            allValid = false;
        }
    }
    
    if (allValid) {
        std::cout << "✓ All 128 MagicEntry structures initialized correctly\n";
    } else {
        std::cout << "✗ Some entries have initialization errors\n";
    }
    
    // Memory layout information
    std::cout << "\n=== MEMORY LAYOUT ===\n";
    std::cout << "Rook magics array: " << sizeof(magic::rookMagics) << " bytes\n";
    std::cout << "Bishop magics array: " << sizeof(magic::bishopMagics) << " bytes\n";
    std::cout << "Total static storage: " 
              << (sizeof(magic::rookMagics) + sizeof(magic::bishopMagics)) << " bytes\n";
    
    // Final result
    std::cout << "\n=== PHASE 1 COMPLETION STATUS ===\n";
    if (allValid) {
        std::cout << "✓ Step 1A Complete: Blocker mask generation\n";
        std::cout << "✓ Step 1B Complete: Magic numbers imported with ULL\n";
        std::cout << "✓ Step 1C Complete: Magic validation function\n";
        std::cout << "✓ Step 1D Complete: MagicEntry structure created\n";
        std::cout << "\n✓✓✓ PHASE 1 COMPLETE! ✓✓✓\n";
        std::cout << "Ready for Phase 2: Attack Table Generation\n";
        return 0;
    } else {
        std::cout << "✗ Phase 1 has errors that need fixing\n";
        return 1;
    }
}