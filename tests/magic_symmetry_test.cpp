/**
 * Magic Bitboards Symmetry and Consistency Tests
 * Stage 10 - Phase 4B: Symmetry and Consistency Validation
 * 
 * This test suite validates:
 * 1. Attack symmetry (if A attacks B, then B is attacked by A)
 * 2. Empty board attacks
 * 3. Full board attacks  
 * 4. Random position consistency
 */

#include <iostream>
#include <iomanip>
#include <random>
#include <vector>
#include <cassert>
#include "../src/core/bitboard.h"
#include "../src/core/magic_bitboards.h"

using namespace seajay;

/**
 * Test that if square A attacks square B, then B is attacked from A
 * This is a fundamental property that must hold for all piece types
 */
bool testAttackSymmetry() {
    std::cout << "Testing attack symmetry..." << std::endl;
    
    // Initialize magic bitboards
    magic::initMagics();
    
    int testCount = 0;
    int passCount = 0;
    
    // Test with various occupancy patterns
    std::vector<Bitboard> testOccupancies = {
        0ULL,                          // Empty board
        ~0ULL,                         // Full board
        0xFF00000000000000ULL,         // Rank 8
        0x00000000000000FFULL,         // Rank 1
        0x8181818181818181ULL,         // Files A and H
        0x0042240000244200ULL,         // Some pieces
        0x00003C3C3C3C0000ULL,         // Center squares
    };
    
    // Add some random occupancies
    std::mt19937 rng(42);
    std::uniform_int_distribution<uint64_t> dist(0, UINT64_MAX);
    for (int i = 0; i < 20; ++i) {
        testOccupancies.push_back(dist(rng) & dist(rng));  // Sparse random
    }
    
    for (const auto& occupied : testOccupancies) {
        // Test rook attacks
        for (Square from = A1; from <= H8; ++from) {
            Bitboard attacks = magicRookAttacks(from, occupied);
            
            // For each attacked square, verify reverse attack
            Bitboard attacksCopy = attacks;
            while (attacksCopy) {
                Square to = static_cast<Square>(__builtin_ctzll(attacksCopy));
                attacksCopy &= attacksCopy - 1;  // Clear LSB
                Bitboard reverseAttacks = magicRookAttacks(to, occupied);
                
                testCount++;
                if (reverseAttacks & squareBB(from)) {
                    passCount++;
                } else {
                    std::cout << "  FAIL: Rook asymmetry detected!" << std::endl;
                    std::cout << "    From: " << squareToString(from) 
                              << " attacks " << squareToString(to) << std::endl;
                    std::cout << "    But " << squareToString(to) 
                              << " doesn't attack " << squareToString(from) << std::endl;
                    return false;
                }
            }
        }
        
        // Test bishop attacks
        for (Square from = A1; from <= H8; ++from) {
            Bitboard attacks = magicBishopAttacks(from, occupied);
            
            // For each attacked square, verify reverse attack
            Bitboard attacksCopy = attacks;
            while (attacksCopy) {
                Square to = static_cast<Square>(__builtin_ctzll(attacksCopy));
                attacksCopy &= attacksCopy - 1;  // Clear LSB
                Bitboard reverseAttacks = magicBishopAttacks(to, occupied);
                
                testCount++;
                if (reverseAttacks & squareBB(from)) {
                    passCount++;
                } else {
                    std::cout << "  FAIL: Bishop asymmetry detected!" << std::endl;
                    std::cout << "    From: " << squareToString(from) 
                              << " attacks " << squareToString(to) << std::endl;
                    std::cout << "    But " << squareToString(to) 
                              << " doesn't attack " << squareToString(from) << std::endl;
                    return false;
                }
            }
        }
    }
    
    std::cout << "  Symmetry tests: " << passCount << "/" << testCount << " passed" << std::endl;
    return passCount == testCount;
}

/**
 * Test attacks on empty board
 * Pieces should be able to attack to board edges
 */
bool testEmptyBoardAttacks() {
    std::cout << "Testing empty board attacks..." << std::endl;
    
    // Test some known positions on empty board
    struct TestCase {
        Square sq;
        bool isRook;
        int expectedCount;
    };
    
    std::vector<TestCase> testCases = {
        // Rooks
        {D4, true, 14},  // Center rook attacks 14 squares
        {A1, true, 14},  // Corner rook attacks 14 squares
        {H8, true, 14},  // Corner rook attacks 14 squares
        {E1, true, 14},  // Edge rook attacks 14 squares
        
        // Bishops
        {D4, false, 13}, // Center bishop attacks 13 squares
        {A1, false, 7},  // Corner bishop attacks 7 squares
        {H8, false, 7},  // Corner bishop attacks 7 squares
        {E1, false, 7},  // Edge bishop attacks 7 squares
    };
    
    for (const auto& test : testCases) {
        Bitboard attacks = test.isRook ? 
            magicRookAttacks(test.sq, 0) : 
            magicBishopAttacks(test.sq, 0);
        
        int count = __builtin_popcountll(attacks);
        if (count != test.expectedCount) {
            std::cout << "  FAIL: " << (test.isRook ? "Rook" : "Bishop") 
                      << " on " << squareToString(test.sq) 
                      << " attacks " << count << " squares, expected " 
                      << test.expectedCount << std::endl;
            return false;
        }
    }
    
    std::cout << "  All empty board tests passed" << std::endl;
    return true;
}

/**
 * Test attacks on full board
 * Pieces should only attack adjacent squares
 */
bool testFullBoardAttacks() {
    std::cout << "Testing full board attacks..." << std::endl;
    
    Bitboard fullBoard = ~0ULL;
    
    // On a full board, sliding pieces can only attack adjacent squares
    
    // Test rook on D4 - should only attack C4, E4, D3, D5
    Bitboard rookD4 = magicRookAttacks(D4, fullBoard);
    Bitboard expectedRookD4 = squareBB(C4) | squareBB(E4) | squareBB(D3) | squareBB(D5);
    if (rookD4 != expectedRookD4) {
        std::cout << "  FAIL: Rook on D4 with full board" << std::endl;
        std::cout << "    Expected: " << std::hex << expectedRookD4 << std::endl;
        std::cout << "    Got:      " << std::hex << rookD4 << std::endl;
        return false;
    }
    
    // Test bishop on D4 - should only attack C3, C5, E3, E5
    Bitboard bishopD4 = magicBishopAttacks(D4, fullBoard);
    Bitboard expectedBishopD4 = squareBB(C3) | squareBB(C5) | squareBB(E3) | squareBB(E5);
    if (bishopD4 != expectedBishopD4) {
        std::cout << "  FAIL: Bishop on D4 with full board" << std::endl;
        std::cout << "    Expected: " << std::hex << expectedBishopD4 << std::endl;
        std::cout << "    Got:      " << std::hex << bishopD4 << std::endl;
        return false;
    }
    
    // Test corner positions
    Bitboard rookA1 = magicRookAttacks(A1, fullBoard);
    Bitboard expectedRookA1 = squareBB(A2) | squareBB(B1);
    if (rookA1 != expectedRookA1) {
        std::cout << "  FAIL: Rook on A1 with full board" << std::endl;
        return false;
    }
    
    Bitboard bishopA1 = magicBishopAttacks(A1, fullBoard);
    Bitboard expectedBishopA1 = squareBB(B2);
    if (bishopA1 != expectedBishopA1) {
        std::cout << "  FAIL: Bishop on A1 with full board" << std::endl;
        return false;
    }
    
    std::cout << "  All full board tests passed" << std::endl;
    return true;
}

/**
 * Test consistency with random positions
 * Magic attacks should always match ray-based attacks
 */
bool testRandomPositions() {
    std::cout << "Testing 1000 random positions..." << std::endl;
    
    std::mt19937 rng(12345);
    std::uniform_int_distribution<uint64_t> dist(0, UINT64_MAX);
    
    int testCount = 0;
    int passCount = 0;
    
    for (int i = 0; i < 1000; ++i) {
        // Generate random occupancy
        Bitboard occupied = dist(rng);
        
        // Sometimes make it sparser
        if (i % 3 == 0) {
            occupied &= dist(rng);
        }
        
        // Test all squares
        for (Square sq = A1; sq <= H8; ++sq) {
            // Compare magic vs ray-based for rooks
            Bitboard magicRook = magicRookAttacks(sq, occupied);
            Bitboard rayRook = rookAttacks(sq, occupied);
            
            testCount++;
            if (magicRook == rayRook) {
                passCount++;
            } else {
                std::cout << "  FAIL: Rook mismatch at " << squareToString(sq) << std::endl;
                std::cout << "    Occupied: " << std::hex << occupied << std::endl;
                std::cout << "    Magic:    " << std::hex << magicRook << std::endl;
                std::cout << "    Ray:      " << std::hex << rayRook << std::endl;
                return false;
            }
            
            // Compare magic vs ray-based for bishops
            Bitboard magicBishop = magicBishopAttacks(sq, occupied);
            Bitboard rayBishop = bishopAttacks(sq, occupied);
            
            testCount++;
            if (magicBishop == rayBishop) {
                passCount++;
            } else {
                std::cout << "  FAIL: Bishop mismatch at " << squareToString(sq) << std::endl;
                std::cout << "    Occupied: " << std::hex << occupied << std::endl;
                std::cout << "    Magic:    " << std::hex << magicBishop << std::endl;
                std::cout << "    Ray:      " << std::hex << rayBishop << std::endl;
                return false;
            }
        }
    }
    
    std::cout << "  Random position tests: " << passCount << "/" << testCount << " passed" << std::endl;
    return true;
}

/**
 * Test queen attacks (combination of rook and bishop)
 */
bool testQueenAttacks() {
    std::cout << "Testing queen attacks..." << std::endl;
    
    std::mt19937 rng(54321);
    std::uniform_int_distribution<uint64_t> dist(0, UINT64_MAX);
    
    for (int i = 0; i < 100; ++i) {
        Bitboard occupied = dist(rng) & dist(rng);  // Sparse
        
        for (Square sq = A1; sq <= H8; sq = static_cast<Square>(sq + 8)) {  // Sample squares
            Bitboard queenAttacks = magicQueenAttacks(sq, occupied);
            Bitboard expectedQueen = magicRookAttacks(sq, occupied) | magicBishopAttacks(sq, occupied);
            
            if (queenAttacks != expectedQueen) {
                std::cout << "  FAIL: Queen attacks don't match rook|bishop at " 
                          << squareToString(sq) << std::endl;
                return false;
            }
        }
    }
    
    std::cout << "  All queen attack tests passed" << std::endl;
    return true;
}

/**
 * Test specific edge cases
 */
bool testEdgeCases() {
    std::cout << "Testing edge cases..." << std::endl;
    
    // Test sliding piece blocked by its own square (shouldn't happen but test anyway)
    for (Square sq = A1; sq <= H8; sq = static_cast<Square>(sq + 7)) {
        Bitboard occupied = squareBB(sq);  // Only the piece's own square
        
        // Attacks should not include own square
        Bitboard rookAtk = magicRookAttacks(sq, occupied);
        if (rookAtk & squareBB(sq)) {
            std::cout << "  FAIL: Rook attacks include own square" << std::endl;
            return false;
        }
        
        Bitboard bishopAtk = magicBishopAttacks(sq, occupied);
        if (bishopAtk & squareBB(sq)) {
            std::cout << "  FAIL: Bishop attacks include own square" << std::endl;
            return false;
        }
    }
    
    // Test alternating pattern (checkerboard)
    Bitboard checkerboard = 0xAA55AA55AA55AA55ULL;
    for (Square sq = D4; sq <= E5; ++sq) {
        Bitboard magicR = magicRookAttacks(sq, checkerboard);
        Bitboard rayR = rookAttacks(sq, checkerboard);
        if (magicR != rayR) {
            std::cout << "  FAIL: Checkerboard pattern rook mismatch" << std::endl;
            return false;
        }
        
        Bitboard magicB = magicBishopAttacks(sq, checkerboard);
        Bitboard rayB = bishopAttacks(sq, checkerboard);
        if (magicB != rayB) {
            std::cout << "  FAIL: Checkerboard pattern bishop mismatch" << std::endl;
            return false;
        }
    }
    
    std::cout << "  All edge cases passed" << std::endl;
    return true;
}

int main() {
    std::cout << "==========================================" << std::endl;
    std::cout << "  Magic Bitboards Symmetry & Consistency " << std::endl;
    std::cout << "       Stage 10 - Phase 4B               " << std::endl;
    std::cout << "==========================================" << std::endl;
    std::cout << std::endl;
    
    bool allPassed = true;
    
    // Run all test suites
    allPassed &= testAttackSymmetry();
    std::cout << std::endl;
    
    allPassed &= testEmptyBoardAttacks();
    std::cout << std::endl;
    
    allPassed &= testFullBoardAttacks();
    std::cout << std::endl;
    
    allPassed &= testRandomPositions();
    std::cout << std::endl;
    
    allPassed &= testQueenAttacks();
    std::cout << std::endl;
    
    allPassed &= testEdgeCases();
    std::cout << std::endl;
    
    std::cout << "==========================================" << std::endl;
    if (allPassed) {
        std::cout << "✅ ALL SYMMETRY & CONSISTENCY TESTS PASSED" << std::endl;
    } else {
        std::cout << "❌ SOME TESTS FAILED" << std::endl;
    }
    std::cout << "==========================================" << std::endl;
    
    return allPassed ? 0 : 1;
}