# Stage 10: Magic Bitboards - Critical Validation Harness

**Parent Document:** [`stage10_implementation_steps.md`](./stage10_implementation_steps.md) (MASTER)  
**Purpose:** Detailed validation harness implementation for Step 0A  
**MANDATORY**: Create and use this validation harness BEFORE implementing magic bitboards!

## The Master Validator Class

```cpp
// Save this as src/core/magic_validator.h
#pragma once

#include "types.h"
#include "bitboard.h"
#include <iostream>
#include <cassert>

class MagicValidator {
private:
    // Keep our current ray-based implementation as the reference
    static Bitboard slowRookAttacks(Square sq, Bitboard occupied) {
        return rayRookAttacks(sq, occupied);  // Our existing implementation
    }
    
    static Bitboard slowBishopAttacks(Square sq, Bitboard occupied) {
        return rayBishopAttacks(sq, occupied);  // Our existing implementation
    }
    
    // Convert index to occupancy pattern
    static Bitboard indexToOccupancy(int index, Bitboard mask) {
        Bitboard occupied = 0;
        int bits = popcount(mask);
        for (int i = 0; i < bits; i++) {
            int j = popLsb(mask);
            if (index & (1 << i)) {
                occupied |= (1ULL << j);
            }
        }
        return occupied;
    }
    
public:
    // Validate a single square for all possible occupancies
    static bool validateSquare(Square sq, bool isRook) {
        const char* piece = isRook ? "Rook" : "Bishop";
        
        Bitboard mask = isRook ? computeRookMask(sq) : computeBishopMask(sq);
        int bits = popcount(mask);
        int size = 1 << bits;
        
        std::cout << "Validating " << piece << " on " << squareToString(sq) 
                  << " (" << size << " patterns)..." << std::endl;
        
        for (int i = 0; i < size; i++) {
            Bitboard occupied = indexToOccupancy(i, mask);
            
            // Compare magic result with slow result
            Bitboard magicResult = isRook ? 
                magicRookAttacks(sq, occupied) : 
                magicBishopAttacks(sq, occupied);
                
            Bitboard slowResult = isRook ? 
                slowRookAttacks(sq, occupied) : 
                slowBishopAttacks(sq, occupied);
            
            if (magicResult != slowResult) {
                std::cerr << "VALIDATION FAILED!" << std::endl;
                std::cerr << piece << " on " << squareToString(sq) << std::endl;
                std::cerr << "Occupancy index: " << i << std::endl;
                std::cerr << "Occupied squares:" << std::endl;
                printBitboard(occupied);
                std::cerr << "Magic attack result:" << std::endl;
                printBitboard(magicResult);
                std::cerr << "Expected (ray-based) result:" << std::endl;
                printBitboard(slowResult);
                std::cerr << "XOR difference:" << std::endl;
                printBitboard(magicResult ^ slowResult);
                return false;
            }
        }
        
        return true;
    }
    
    // Complete validation of all squares
    static bool validateAll() {
        std::cout << "Starting complete magic bitboard validation..." << std::endl;
        std::cout << "This will test " << (64*4096 + 64*512) 
                  << " attack patterns." << std::endl;
        
        // Validate all rook squares
        for (Square sq = A1; sq <= H8; sq++) {
            if (!validateSquare(sq, true)) {
                return false;
            }
        }
        
        // Validate all bishop squares
        for (Square sq = A1; sq <= H8; sq++) {
            if (!validateSquare(sq, false)) {
                return false;
            }
        }
        
        std::cout << "SUCCESS! All magic bitboard attacks validated!" << std::endl;
        return true;
    }
    
    // Symmetry test - if A attacks B, then B must attack A
    static bool validateSymmetry(Bitboard occupied) {
        for (Square s1 = A1; s1 <= H8; s1++) {
            // Test rook symmetry
            Bitboard rook1 = magicRookAttacks(s1, occupied);
            for (Square s2 = A1; s2 <= H8; s2++) {
                if (rook1 & squareBB(s2)) {
                    Bitboard rook2 = magicRookAttacks(s2, occupied);
                    if (!(rook2 & squareBB(s1))) {
                        std::cerr << "Rook symmetry violation: " 
                                  << squareToString(s1) << " attacks " 
                                  << squareToString(s2) << " but not vice versa!"
                                  << std::endl;
                        return false;
                    }
                }
            }
            
            // Test bishop symmetry
            Bitboard bishop1 = magicBishopAttacks(s1, occupied);
            for (Square s2 = A1; s2 <= H8; s2++) {
                if (bishop1 & squareBB(s2)) {
                    Bitboard bishop2 = magicBishopAttacks(s2, occupied);
                    if (!(bishop2 & squareBB(s1))) {
                        std::cerr << "Bishop symmetry violation: "
                                  << squareToString(s1) << " attacks "
                                  << squareToString(s2) << " but not vice versa!"
                                  << std::endl;
                        return false;
                    }
                }
            }
        }
        return true;
    }
    
    // Edge case tests
    static bool validateEdgeCases() {
        // Test empty board
        if (!validateSymmetry(0)) {
            std::cerr << "Failed symmetry test on empty board!" << std::endl;
            return false;
        }
        
        // Test full board (except edges)
        Bitboard maxOccupancy = 0x007E7E7E7E7E7E00ULL;
        if (!validateSymmetry(maxOccupancy)) {
            std::cerr << "Failed symmetry test on max occupancy!" << std::endl;
            return false;
        }
        
        // Test random positions
        for (int i = 0; i < 1000; i++) {
            Bitboard random = randomBitboard();
            if (!validateSymmetry(random)) {
                std::cerr << "Failed symmetry test on random position!" << std::endl;
                printBitboard(random);
                return false;
            }
        }
        
        return true;
    }
};
```

## Critical Test Positions

```cpp
// Add these specific positions to your test suite
const char* criticalPositions[] = {
    // The "Works for 99.9% of Games" position
    "8/6R1/8/8/8/8/8/8 w - - 0 1",  // Rook on h7
    
    // The "Phantom Blocker" after en passant
    "8/2p5/3p4/KP5r/1R3pPk/8/4P3/8 b - g3 0 1",
    
    // The "Promotion with Discovery Check"
    "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1",
    
    // The "Symmetric Castling"
    "r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1",
    
    // Corner pieces (maximum edge cases)
    "R6R/8/8/8/8/8/8/r6r w - - 0 1",  // Corner rooks
    "B6B/8/8/8/8/8/8/b6b w - - 0 1",  // Corner bishops
    
    // Maximum blockers
    "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
    
    // Slider x-rays
    "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1",
};
```

## Debug Helpers

```cpp
// Add to your debug arsenal
#ifdef DEBUG_MAGIC
    #define TRACE_MAGIC(sq, occ, result) \
        std::cout << "Magic: sq=" << (sq) << " occ=0x" << std::hex << (occ) \
                  << " result=0x" << (result) << std::dec << std::endl
#else
    #define TRACE_MAGIC(sq, occ, result)
#endif

// Binary search helper for debugging perft discrepancies
class PerftDebugger {
    static int moveCounter;
    static int breakAtMove;
    
public:
    static void checkMove(Move move, Square from, Bitboard occupied) {
        if (++moveCounter == breakAtMove) {
            std::cout << "Move #" << moveCounter << ": " << moveToString(move) << "\n";
            std::cout << "From square: " << squareToString(from) << "\n";
            std::cout << "Occupied: 0x" << std::hex << occupied << std::dec << "\n";
            
            // Compare magic vs ray
            Bitboard magic = magicRookAttacks(from, occupied);
            Bitboard ray = rayRookAttacks(from, occupied);
            
            if (magic != ray) {
                std::cout << "MISMATCH DETECTED!\n";
                std::cout << "Magic attacks:\n";
                printBitboard(magic);
                std::cout << "Ray attacks:\n";
                printBitboard(ray);
                assert(false);  // Break here in debugger
            }
        }
    }
};
```

## Validation Checkpoints

Run validation at these critical points:

1. ✅ **After mask generation** - Verify bit counts
2. ✅ **After magic number import** - Test each magic for collisions  
3. ✅ **After table generation** - Complete validation of all patterns
4. ✅ **After integration** - A/B test with ray-based
5. ✅ **Before each commit** - Full validation suite
6. ✅ **In both Debug and Release** - Different optimizations may expose bugs
7. ✅ **After 100 games** - Look for illegal moves
8. ✅ **After 1000 games** - Final validation before removing ray-based

## The Nuclear Option

When all else fails:

```cpp
// Generate complete trace for differential debugging
void generateTrace(const std::string& filename) {
    std::ofstream trace(filename);
    
    for (Square sq = A1; sq <= H8; sq++) {
        for (int pattern = 0; pattern < 100; pattern++) {
            Bitboard occupied = randomBitboard();
            
            Bitboard rookMagic = magicRookAttacks(sq, occupied);
            Bitboard rookRay = rayRookAttacks(sq, occupied);
            
            trace << "R," << sq << ",0x" << std::hex << occupied 
                  << ",0x" << rookMagic << ",0x" << rookRay
                  << "," << (rookMagic == rookRay ? "OK" : "FAIL") << "\n";
                  
            Bitboard bishopMagic = magicBishopAttacks(sq, occupied);
            Bitboard bishopRay = rayBishopAttacks(sq, occupied);
            
            trace << "B," << sq << ",0x" << std::hex << occupied
                  << ",0x" << bishopMagic << ",0x" << bishopRay  
                  << "," << (bishopMagic == bishopRay ? "OK" : "FAIL") << "\n";
        }
    }
    
    trace.close();
    std::cout << "Trace written to " << filename << std::endl;
}

// Then: diff magic_trace.txt reference_trace.txt
```

## Remember

> "Trust your validator more than your implementation. If they disagree, the validator is probably right." - chess-engine-expert

This validation harness is your safety net. Use it religiously and you'll avoid the weeks of debugging that have plagued other implementations.