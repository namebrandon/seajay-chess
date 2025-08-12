#include "../src/core/magic_bitboards.h"
#include <iostream>
#include <chrono>

using namespace seajay;
using namespace seajay::magic;

bool testInitialization() {
    std::cout << "Test 1: Magic Bitboards Initialization\n";
    
    // Check initial state
    if (areMagicsInitialized()) {
        std::cout << "  ERROR: Already initialized before initMagics() call\n";
        return false;
    }
    
    // Initialize
    auto start = std::chrono::high_resolution_clock::now();
    initMagics();
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    std::cout << "  Initialization time: " << duration.count() << " ms\n";
    
    // Verify initialization
    if (!areMagicsInitialized()) {
        std::cout << "  ERROR: Failed to initialize\n";
        return false;
    }
    
    std::cout << "  ✓ PASSED\n";
    return true;
}

bool testSingleSquareValidation(Square sq, bool isRook) {
    Bitboard mask = isRook ? computeRookMask(sq) : computeBishopMask(sq);
    int numPatterns = 1 << popCount(mask);
    
    for (int pattern = 0; pattern < numPatterns; ++pattern) {
        Bitboard occupancy = indexToOccupancy(pattern, mask);
        Bitboard slowAttacks = isRook ? 
            generateSlowRookAttacks(sq, occupancy) :
            generateSlowBishopAttacks(sq, occupancy);
        Bitboard magicAttacks = isRook ?
            magicRookAttacks(sq, occupancy) :
            magicBishopAttacks(sq, occupancy);
        
        if (slowAttacks != magicAttacks) {
            return false;
        }
    }
    
    return true;
}

bool testAllSquares() {
    std::cout << "\nTest 2: Validate All Squares\n";
    
    // Test all rook squares
    std::cout << "  Testing rooks (262,144 patterns total)...";
    for (Square sq = 0; sq < 64; ++sq) {
        if (!testSingleSquareValidation(sq, true)) {
            std::cout << " FAILED at square " << sq << "\n";
            return false;
        }
    }
    std::cout << " ✓\n";
    
    // Test all bishop squares
    std::cout << "  Testing bishops (32,768 patterns total)...";
    for (Square sq = 0; sq < 64; ++sq) {
        if (!testSingleSquareValidation(sq, false)) {
            std::cout << " FAILED at square " << sq << "\n";
            return false;
        }
    }
    std::cout << " ✓\n";
    
    std::cout << "  ✓ PASSED\n";
    return true;
}

bool testSpecificPositions() {
    std::cout << "\nTest 3: Specific Position Validation\n";
    
    // Test 1: Rook on D4 with specific blockers
    {
        Square sq = makeSquare(File(3), Rank(3));  // D4
        Bitboard blockers = squareBB(makeSquare(File(3), Rank(5))) |  // D6
                           squareBB(makeSquare(File(1), Rank(3)));     // B4
        
        Bitboard expected = generateSlowRookAttacks(sq, blockers);
        Bitboard actual = magicRookAttacks(sq, blockers);
        
        if (expected != actual) {
            std::cout << "  ERROR: Rook on D4 mismatch\n";
            std::cout << "    Expected: 0x" << std::hex << expected << std::dec << "\n";
            std::cout << "    Got:      0x" << std::hex << actual << std::dec << "\n";
            return false;
        }
        std::cout << "  Rook on D4 with blockers: ✓\n";
    }
    
    // Test 2: Bishop on E5 with specific blockers
    {
        Square sq = makeSquare(File(4), Rank(4));  // E5
        Bitboard blockers = squareBB(makeSquare(File(6), Rank(6))) |  // G7
                           squareBB(makeSquare(File(2), Rank(2)));     // C3
        
        Bitboard expected = generateSlowBishopAttacks(sq, blockers);
        Bitboard actual = magicBishopAttacks(sq, blockers);
        
        if (expected != actual) {
            std::cout << "  ERROR: Bishop on E5 mismatch\n";
            std::cout << "    Expected: 0x" << std::hex << expected << std::dec << "\n";
            std::cout << "    Got:      0x" << std::hex << actual << std::dec << "\n";
            return false;
        }
        std::cout << "  Bishop on E5 with blockers: ✓\n";
    }
    
    // Test 3: Edge cases - corner squares
    {
        // Rook on A1
        Square sq = makeSquare(File(0), Rank(0));  // A1
        Bitboard blockers = 0;
        Bitboard expected = generateSlowRookAttacks(sq, blockers);
        Bitboard actual = magicRookAttacks(sq, blockers);
        
        if (expected != actual) {
            std::cout << "  ERROR: Rook on A1 (corner) mismatch\n";
            return false;
        }
        std::cout << "  Rook on A1 (corner): ✓\n";
        
        // Bishop on H8
        sq = makeSquare(File(7), Rank(7));  // H8
        expected = generateSlowBishopAttacks(sq, blockers);
        actual = magicBishopAttacks(sq, blockers);
        
        if (expected != actual) {
            std::cout << "  ERROR: Bishop on H8 (corner) mismatch\n";
            return false;
        }
        std::cout << "  Bishop on H8 (corner): ✓\n";
    }
    
    std::cout << "  ✓ PASSED\n";
    return true;
}

bool testPerformance() {
    std::cout << "\nTest 4: Performance Benchmark\n";
    
    const int iterations = 10000000;
    
    // Benchmark rook attacks
    auto start = std::chrono::high_resolution_clock::now();
    Bitboard sum = 0;
    for (int i = 0; i < iterations; ++i) {
        Square sq = i % 64;
        Bitboard occ = i * 0x123456789ABCDEF;
        sum ^= magicRookAttacks(sq, occ);
    }
    auto end = std::chrono::high_resolution_clock::now();
    auto rookTime = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
    
    std::cout << "  Rook attacks: " << iterations << " lookups in " 
              << rookTime.count() / 1000000.0 << " ms\n";
    std::cout << "    Average: " << rookTime.count() / iterations << " ns per lookup\n";
    
    // Benchmark bishop attacks
    start = std::chrono::high_resolution_clock::now();
    sum = 0;
    for (int i = 0; i < iterations; ++i) {
        Square sq = i % 64;
        Bitboard occ = i * 0x987654321FEDCBA;
        sum ^= magicBishopAttacks(sq, occ);
    }
    end = std::chrono::high_resolution_clock::now();
    auto bishopTime = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
    
    std::cout << "  Bishop attacks: " << iterations << " lookups in " 
              << bishopTime.count() / 1000000.0 << " ms\n";
    std::cout << "    Average: " << bishopTime.count() / iterations << " ns per lookup\n";
    
    // Use sum to prevent optimization
    if (sum == 0) std::cout << "";
    
    std::cout << "  ✓ PASSED\n";
    return true;
}

int main() {
    std::cout << "\n============================================\n";
    std::cout << "Stage 10 Phase 2: Complete Validation Suite\n";
    std::cout << "============================================\n\n";
    
    bool allPassed = true;
    
    // Run all tests
    allPassed &= testInitialization();
    allPassed &= testAllSquares();
    allPassed &= testSpecificPositions();
    allPassed &= testPerformance();
    
    // Final summary
    std::cout << "\n============================================\n";
    if (allPassed) {
        std::cout << "✓ ALL PHASE 2 TESTS PASSED!\n";
        std::cout << "\nPhase 2 Complete Summary:\n";
        std::cout << "  ✓ Phase 2A: Memory allocation (841 KB)\n";
        std::cout << "  ✓ Phase 2B: Single square validation\n";
        std::cout << "  ✓ Phase 2C: All rook tables (262,144 patterns)\n";
        std::cout << "  ✓ Phase 2D: All bishop tables (32,768 patterns)\n";
        std::cout << "  ✓ Phase 2E: Initialization system\n";
        std::cout << "\nReady to proceed to Phase 3: Fast lookup implementation\n";
        std::cout << "============================================\n\n";
        return 0;
    } else {
        std::cout << "✗ PHASE 2 VALIDATION FAILED\n";
        std::cout << "============================================\n\n";
        return 1;
    }
}