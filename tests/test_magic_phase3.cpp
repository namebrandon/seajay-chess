/**
 * Test for Phase 3A - Magic Attack Functions Validation
 * 
 * Tests magic attack functions with known positions to ensure they
 * work correctly before integration with move generation.
 */

#include "../src/core/magic_bitboards.h"
#include "../src/core/bitboard.h"
#include <iostream>
#include <iomanip>

using namespace seajay;

void printBitboard(const std::string& name, Bitboard bb) {
    std::cout << name << ":\n";
    for (int rank = 7; rank >= 0; rank--) {
        std::cout << (rank + 1) << " ";
        for (int file = 0; file < 8; file++) {
            Square sq = makeSquare(static_cast<File>(file), static_cast<Rank>(rank));
            if (bb & squareBB(sq)) {
                std::cout << "X ";
            } else {
                std::cout << ". ";
            }
        }
        std::cout << "\n";
    }
    std::cout << "  a b c d e f g h\n";
    std::cout << "Hex: 0x" << std::hex << bb << std::dec << "\n\n";
}

bool testKnownPositions() {
    std::cout << "=== Phase 3A: Testing Magic Attack Functions ===\n\n";
    
    // Test 1: Rook on D4 with specific blockers
    {
        std::cout << "Test 1: Rook on D4 with blockers\n";
        Square sq = D4;
        Bitboard occupied = squareBB(D2) | squareBB(D6) | squareBB(B4) | squareBB(F4);
        
        std::cout << "Square: D4\n";
        printBitboard("Occupied", occupied);
        
        Bitboard magicAttacks = magicRookAttacks(sq, occupied);
        Bitboard rayAttacks = rookAttacks(sq, occupied);
        
        printBitboard("Magic Attacks", magicAttacks);
        printBitboard("Ray Attacks", rayAttacks);
        
        if (magicAttacks != rayAttacks) {
            std::cerr << "ERROR: Rook attacks mismatch!\n";
            return false;
        }
        std::cout << "✓ Rook attacks match\n\n";
    }
    
    // Test 2: Bishop on E5 with specific blockers
    {
        std::cout << "Test 2: Bishop on E5 with blockers\n";
        Square sq = E5;
        Bitboard occupied = squareBB(C3) | squareBB(G7) | squareBB(B2) | squareBB(H8);
        
        std::cout << "Square: E5\n";
        printBitboard("Occupied", occupied);
        
        Bitboard magicAttacks = magicBishopAttacks(sq, occupied);
        Bitboard rayAttacks = bishopAttacks(sq, occupied);
        
        printBitboard("Magic Attacks", magicAttacks);
        printBitboard("Ray Attacks", rayAttacks);
        
        if (magicAttacks != rayAttacks) {
            std::cerr << "ERROR: Bishop attacks mismatch!\n";
            return false;
        }
        std::cout << "✓ Bishop attacks match\n\n";
    }
    
    // Test 3: Queen on C6 with complex position
    {
        std::cout << "Test 3: Queen on C6 with complex position\n";
        Square sq = C6;
        Bitboard occupied = squareBB(A6) | squareBB(C3) | squareBB(E6) | 
                          squareBB(B7) | squareBB(D5) | squareBB(C8);
        
        std::cout << "Square: C6\n";
        printBitboard("Occupied", occupied);
        
        Bitboard magicAttacks = magicQueenAttacks(sq, occupied);
        Bitboard rayAttacks = queenAttacks(sq, occupied);
        
        printBitboard("Magic Attacks", magicAttacks);
        printBitboard("Ray Attacks", rayAttacks);
        
        if (magicAttacks != rayAttacks) {
            std::cerr << "ERROR: Queen attacks mismatch!\n";
            return false;
        }
        std::cout << "✓ Queen attacks match\n\n";
    }
    
    // Test 4: Edge cases - corner squares
    {
        std::cout << "Test 4: Corner square tests\n";
        
        // Rook on A1
        Square sq = A1;
        Bitboard occupied = squareBB(A4) | squareBB(D1);
        Bitboard magicR = magicRookAttacks(sq, occupied);
        Bitboard rayR = rookAttacks(sq, occupied);
        
        if (magicR != rayR) {
            std::cerr << "ERROR: Rook on A1 mismatch!\n";
            return false;
        }
        std::cout << "✓ Rook on A1 matches\n";
        
        // Bishop on H8
        sq = H8;
        occupied = squareBB(E5) | squareBB(C3);
        Bitboard magicB = magicBishopAttacks(sq, occupied);
        Bitboard rayB = bishopAttacks(sq, occupied);
        
        if (magicB != rayB) {
            std::cerr << "ERROR: Bishop on H8 mismatch!\n";
            return false;
        }
        std::cout << "✓ Bishop on H8 matches\n\n";
    }
    
    // Test 5: Full board coverage
    {
        std::cout << "Test 5: Testing all squares...\n";
        int tested = 0;
        
        for (Square sq = A1; sq <= H8; ++sq) {
            // Test with various occupancy patterns
            Bitboard patterns[] = {
                0ULL,  // Empty board
                ~0ULL, // Full board
                0x00FF00FF00FF00FFULL, // Checkerboard
                0x5555555555555555ULL, // Alternating
                rand() & rand() & rand()  // Random sparse
            };
            
            for (Bitboard occupied : patterns) {
                // Test rook
                Bitboard magicR = magicRookAttacks(sq, occupied);
                Bitboard rayR = rookAttacks(sq, occupied);
                if (magicR != rayR) {
                    std::cerr << "ERROR: Rook mismatch at " << sq << " with occupied 0x" 
                             << std::hex << occupied << std::dec << "\n";
                    return false;
                }
                
                // Test bishop
                Bitboard magicB = magicBishopAttacks(sq, occupied);
                Bitboard rayB = bishopAttacks(sq, occupied);
                if (magicB != rayB) {
                    std::cerr << "ERROR: Bishop mismatch at " << sq << " with occupied 0x" 
                             << std::hex << occupied << std::dec << "\n";
                    return false;
                }
                
                // Test queen
                Bitboard magicQ = magicQueenAttacks(sq, occupied);
                Bitboard rayQ = queenAttacks(sq, occupied);
                if (magicQ != rayQ) {
                    std::cerr << "ERROR: Queen mismatch at " << sq << " with occupied 0x" 
                             << std::hex << occupied << std::dec << "\n";
                    return false;
                }
                
                tested++;
            }
        }
        
        std::cout << "✓ All " << tested << " test patterns passed\n\n";
    }
    
    return true;
}

int main() {
    std::cout << "Phase 3A: Magic Attack Functions Validation\n";
    std::cout << "============================================\n\n";
    
    // Initialize magic bitboards
    magic::initMagics();
    
    if (!magic::areMagicsInitialized()) {
        std::cerr << "ERROR: Failed to initialize magic bitboards!\n";
        return 1;
    }
    
    // Run validation tests
    if (!testKnownPositions()) {
        std::cerr << "\n❌ Phase 3A FAILED: Magic attack functions do not match ray-based\n";
        return 1;
    }
    
    std::cout << "✅ Phase 3A COMPLETE: Magic attack functions validated\n";
    std::cout << "Gate: All magic functions match ray-based for sample positions\n\n";
    
    return 0;
}