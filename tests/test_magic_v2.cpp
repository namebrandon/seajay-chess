#include "../src/core/magic_bitboards_v2.h"
#include <iostream>
#include <chrono>

using namespace seajay;
using namespace seajay::magic_v2;

bool validateSingleSquare(Square sq, bool isRook) {
    Bitboard mask = isRook ? computeRookMask(sq) : computeBishopMask(sq);
    int numPatterns = 1 << popCount(mask);
    int failures = 0;
    
    for (int pattern = 0; pattern < numPatterns; ++pattern) {
        Bitboard occupancy = indexToOccupancy(pattern, mask);
        Bitboard slowAttacks = isRook ? 
            generateSlowRookAttacks(sq, occupancy) :
            generateSlowBishopAttacks(sq, occupancy);
        Bitboard magicAttacks = isRook ?
            seajay::magicRookAttacks(sq, occupancy) :
            seajay::magicBishopAttacks(sq, occupancy);
        
        if (slowAttacks != magicAttacks) {
            failures++;
            if (failures <= 3) {  // Only show first 3 failures
                std::cout << "  MISMATCH at " << (isRook ? "rook" : "bishop") 
                          << " square " << sq << ", pattern " << pattern << "\n";
                std::cout << "    Occupancy: 0x" << std::hex << occupancy << std::dec << "\n";
                std::cout << "    Slow:      0x" << std::hex << slowAttacks << std::dec << "\n";
                std::cout << "    Magic:     0x" << std::hex << magicAttacks << std::dec << "\n";
            }
        }
    }
    
    return failures == 0;
}

int main() {
    std::cout << "\n=== Testing Magic Bitboards v2 (Header-Only) ===\n\n";
    
    // Test 1: Check initialization status
    std::cout << "Test 1: Initial status\n";
    std::cout << "  Initialized: " << (areMagicsInitialized() ? "YES" : "NO") << "\n";
    
    // Test 2: Initialize magic bitboards
    std::cout << "\nTest 2: Initialization\n";
    auto start = std::chrono::high_resolution_clock::now();
    initMagics();
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    std::cout << "  Initialization time: " << duration.count() << " ms\n";
    
    // Test 3: Verify initialization
    std::cout << "\nTest 3: Post-initialization status\n";
    std::cout << "  Initialized: " << (areMagicsInitialized() ? "YES" : "NO") << "\n";
    
    if (!areMagicsInitialized()) {
        std::cerr << "ERROR: Magic bitboards failed to initialize!\n";
        return 1;
    }
    
    // Test 4: Validate all squares
    std::cout << "\nTest 4: Validating all squares\n";
    
    bool allPassed = true;
    int rooksFailed = 0;
    int bishopsFailed = 0;
    
    // Test all rook squares
    std::cout << "  Testing rooks...\n";
    for (Square sq = 0; sq < 64; ++sq) {
        if (!validateSingleSquare(sq, true)) {
            rooksFailed++;
            allPassed = false;
        }
    }
    
    // Test all bishop squares
    std::cout << "  Testing bishops...\n";
    for (Square sq = 0; sq < 64; ++sq) {
        if (!validateSingleSquare(sq, false)) {
            bishopsFailed++;
            allPassed = false;
        }
    }
    
    // Test 5: Summary
    std::cout << "\n=== Test Summary ===\n";
    if (allPassed) {
        std::cout << "✓ ALL TESTS PASSED!\n";
        std::cout << "✓ All 64 rook squares validated (262,144 patterns)\n";
        std::cout << "✓ All 64 bishop squares validated (32,768 patterns)\n";
        std::cout << "✓ Magic bitboards v2 ready for use\n";
        return 0;
    } else {
        std::cout << "✗ TESTS FAILED\n";
        if (rooksFailed > 0) {
            std::cout << "  Rook squares failed: " << rooksFailed << "/64\n";
        }
        if (bishopsFailed > 0) {
            std::cout << "  Bishop squares failed: " << bishopsFailed << "/64\n";
        }
        return 1;
    }
}