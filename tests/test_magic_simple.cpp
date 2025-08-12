#include "../src/core/magic_bitboards_simple.h"
#include <iostream>
#include <chrono>

using namespace seajay;
using namespace seajay::magic_simple;

int main() {
    std::cout << "\n=== Testing Simplified Magic Bitboards ===\n\n";
    
    // Test 1: Initialize and get data
    std::cout << "Test 1: Initialization\n";
    auto start = std::chrono::high_resolution_clock::now();
    const auto& data = getMagicData();
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    std::cout << "  Initialization time: " << duration.count() << " ms\n";
    std::cout << "  Initialized: " << (data.initialized ? "YES" : "NO") << "\n";
    
    if (!data.initialized) {
        std::cerr << "ERROR: Failed to initialize!\n";
        return 1;
    }
    
    // Test 2: Validate a few specific positions
    std::cout << "\nTest 2: Specific Position Validation\n";
    
    // Rook on D4 with no blockers
    {
        Square sq = makeSquare(File(3), Rank(3));  // D4
        Bitboard occupied = 0;
        
        Bitboard slowAttacks = generateSlowRookAttacks(sq, occupied);
        Bitboard magicAttacks = magicRookAttacksSimple(sq, occupied);
        
        if (slowAttacks != magicAttacks) {
            std::cout << "  ERROR: Rook on D4 (empty board) mismatch\n";
            std::cout << "    Slow:  0x" << std::hex << slowAttacks << std::dec << "\n";
            std::cout << "    Magic: 0x" << std::hex << magicAttacks << std::dec << "\n";
            return 1;
        }
        std::cout << "  Rook on D4 (empty): ✓\n";
    }
    
    // Rook on D4 with blockers
    {
        Square sq = makeSquare(File(3), Rank(3));  // D4
        Bitboard occupied = squareBB(makeSquare(File(3), Rank(5))) |  // D6
                           squareBB(makeSquare(File(1), Rank(3)));     // B4
        
        Bitboard slowAttacks = generateSlowRookAttacks(sq, occupied);
        Bitboard magicAttacks = magicRookAttacksSimple(sq, occupied);
        
        if (slowAttacks != magicAttacks) {
            std::cout << "  ERROR: Rook on D4 (with blockers) mismatch\n";
            std::cout << "    Occupied: 0x" << std::hex << occupied << std::dec << "\n";
            std::cout << "    Slow:     0x" << std::hex << slowAttacks << std::dec << "\n";
            std::cout << "    Magic:    0x" << std::hex << magicAttacks << std::dec << "\n";
            return 1;
        }
        std::cout << "  Rook on D4 (blockers): ✓\n";
    }
    
    // Bishop on E5
    {
        Square sq = makeSquare(File(4), Rank(4));  // E5
        Bitboard occupied = squareBB(makeSquare(File(6), Rank(6))) |  // G7
                           squareBB(makeSquare(File(2), Rank(2)));     // C3
        
        Bitboard slowAttacks = generateSlowBishopAttacks(sq, occupied);
        Bitboard magicAttacks = magicBishopAttacksSimple(sq, occupied);
        
        if (slowAttacks != magicAttacks) {
            std::cout << "  ERROR: Bishop on E5 mismatch\n";
            std::cout << "    Slow:  0x" << std::hex << slowAttacks << std::dec << "\n";
            std::cout << "    Magic: 0x" << std::hex << magicAttacks << std::dec << "\n";
            return 1;
        }
        std::cout << "  Bishop on E5: ✓\n";
    }
    
    // Test 3: Validate all squares for one pattern each
    std::cout << "\nTest 3: Quick validation of all squares\n";
    
    int rookErrors = 0;
    int bishopErrors = 0;
    
    for (Square sq = 0; sq < 64; ++sq) {
        // Test with a specific occupancy pattern
        Bitboard occupied = 0x5555555555555555ULL;  // Checkerboard pattern
        
        // Test rook
        Bitboard slowRook = generateSlowRookAttacks(sq, occupied);
        Bitboard magicRook = magicRookAttacksSimple(sq, occupied);
        if (slowRook != magicRook) {
            rookErrors++;
        }
        
        // Test bishop
        Bitboard slowBishop = generateSlowBishopAttacks(sq, occupied);
        Bitboard magicBishop = magicBishopAttacksSimple(sq, occupied);
        if (slowBishop != magicBishop) {
            bishopErrors++;
        }
    }
    
    if (rookErrors > 0 || bishopErrors > 0) {
        std::cout << "  Rook errors: " << rookErrors << "/64\n";
        std::cout << "  Bishop errors: " << bishopErrors << "/64\n";
        return 1;
    }
    
    std::cout << "  All 64 squares validated: ✓\n";
    
    // Test 4: Performance test
    std::cout << "\nTest 4: Performance\n";
    
    const int iterations = 1000000;
    Bitboard sum = 0;
    
    start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < iterations; ++i) {
        Square sq = i % 64;
        Bitboard occ = i * 0x123456789ABCDEF;
        sum ^= magicRookAttacksSimple(sq, occ);
    }
    end = std::chrono::high_resolution_clock::now();
    
    auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
    std::cout << "  " << iterations << " rook lookups: " << ns/1000000.0 << " ms";
    std::cout << " (" << ns/iterations << " ns/lookup)\n";
    
    // Prevent optimization
    if (sum == 0) std::cout << "";
    
    std::cout << "\n=== Summary ===\n";
    std::cout << "✓ Simplified magic bitboards working correctly\n";
    std::cout << "✓ Ready for integration\n\n";
    
    return 0;
}