#pragma once

#include "types.h"
#include "bitboard.h"
#include <iostream>
#include <cassert>
#include <random>
#include <chrono>

namespace seajay {

/**
 * MagicValidator - Critical validation harness for magic bitboard implementation
 * 
 * This class provides comprehensive validation of magic bitboard attack generation
 * by comparing against our existing ray-based implementation. It must be used
 * throughout the development process to ensure correctness.
 * 
 * MANDATORY: Run validation after every step of implementation!
 */
class MagicValidator {
private:
    // Keep our current ray-based implementation as the reference
    // These wrap the existing functions in bitboard.h
    static Bitboard slowRookAttacks(Square sq, Bitboard occupied) {
        return ::seajay::rookAttacks(sq, occupied);  // Our existing ray-based implementation
    }
    
    static Bitboard slowBishopAttacks(Square sq, Bitboard occupied) {
        return ::seajay::bishopAttacks(sq, occupied);  // Our existing ray-based implementation
    }
    
    // Convert index to occupancy pattern
    // This function reconstructs an occupancy bitboard from an index
    static Bitboard indexToOccupancy(int index, Bitboard mask) {
        Bitboard occupied = 0;
        int bits = ::seajay::popCount(mask);
        
        // Iterate through set bits in mask
        Bitboard tempMask = mask;
        for (int i = 0; i < bits; i++) {
            // Get the least significant bit position
            Square sq = lsb(tempMask);
            
            // If the corresponding bit in index is set, set this square in occupied
            if (index & (1 << i)) {
                occupied |= squareBB(sq);
            }
            
            // Clear this bit from the mask
            tempMask &= tempMask - 1;
        }
        
        return occupied;
    }
    
    // Random bitboard generator for testing
    static Bitboard randomBitboard() {
        static std::mt19937_64 rng(std::chrono::steady_clock::now().time_since_epoch().count());
        return rng();
    }
    
public:
    // Forward declarations for functions that will be implemented later
    // These will be provided when we implement magic bitboards
    static Bitboard computeRookMask(Square sq);
    static Bitboard computeBishopMask(Square sq);
    static Bitboard magicRookAttacks(Square sq, Bitboard occupied);
    static Bitboard magicBishopAttacks(Square sq, Bitboard occupied);
    
    /**
     * Validate a single square for all possible occupancies
     * This is the core validation function that tests every possible
     * occupancy pattern for a given square
     */
    static bool validateSquare(Square sq, bool isRook) {
        const char* piece = isRook ? "Rook" : "Bishop";
        
        // These functions will be implemented in Phase 1
        // For now, we'll create stub implementations for compilation
        Bitboard mask = isRook ? computeRookMask(sq) : computeBishopMask(sq);
        int bits = ::seajay::popCount(mask);
        int size = 1 << bits;
        
        std::cout << "Validating " << piece << " on square " << static_cast<int>(sq) 
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
                std::cerr << piece << " on square " << static_cast<int>(sq) << std::endl;
                std::cerr << "Occupancy index: " << i << std::endl;
                std::cerr << "Occupied squares:" << std::endl;
                std::cerr << bitboardToString(occupied) << std::endl;
                std::cerr << "Magic attack result:" << std::endl;
                std::cerr << bitboardToString(magicResult) << std::endl;
                std::cerr << "Expected (ray-based) result:" << std::endl;
                std::cerr << bitboardToString(slowResult) << std::endl;
                std::cerr << "XOR difference:" << std::endl;
                std::cerr << bitboardToString(magicResult ^ slowResult) << std::endl;
                return false;
            }
        }
        
        return true;
    }
    
    /**
     * Complete validation of all squares
     * Tests all 64 squares for both rooks and bishops
     */
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
    
    /**
     * Symmetry test - if A attacks B, then B must attack A
     * This is a critical invariant that must hold for all positions
     */
    static bool validateSymmetry(Bitboard occupied) {
        for (Square s1 = A1; s1 <= H8; s1++) {
            // Test rook symmetry
            Bitboard rook1 = magicRookAttacks(s1, occupied);
            for (Square s2 = A1; s2 <= H8; s2++) {
                if (rook1 & squareBB(s2)) {
                    Bitboard rook2 = magicRookAttacks(s2, occupied);
                    if (!(rook2 & squareBB(s1))) {
                        std::cerr << "Rook symmetry violation: " 
                                  << "Square " << static_cast<int>(s1) << " attacks " 
                                  << static_cast<int>(s2) << " but not vice versa!"
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
                                  << "Square " << static_cast<int>(s1) << " attacks "
                                  << static_cast<int>(s2) << " but not vice versa!"
                                  << std::endl;
                        return false;
                    }
                }
            }
        }
        return true;
    }
    
    /**
     * Edge case tests
     * Tests various boundary conditions and special cases
     */
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
                std::cerr << bitboardToString(random) << std::endl;
                return false;
            }
        }
        
        return true;
    }
    
    /**
     * Quick smoke test for rapid validation during development
     * Should complete in under 1 second
     */
    static bool quickValidation() {
        auto start = std::chrono::high_resolution_clock::now();
        
        // Test 10 random positions per square
        for (Square sq = A1; sq <= H8; sq++) {
            for (int i = 0; i < 10; i++) {
                Bitboard occ = randomBitboard();
                
                // Test rook attacks
                Bitboard magicR = magicRookAttacks(sq, occ);
                Bitboard rayR = slowRookAttacks(sq, occ);
                if (magicR != rayR) {
                    std::cerr << "Quick validation failed for rook on square " 
                              << static_cast<int>(sq) << std::endl;
                    return false;
                }
                
                // Test bishop attacks
                Bitboard magicB = magicBishopAttacks(sq, occ);
                Bitboard rayB = slowBishopAttacks(sq, occ);
                if (magicB != rayB) {
                    std::cerr << "Quick validation failed for bishop on square " 
                              << static_cast<int>(sq) << std::endl;
                    return false;
                }
            }
        }
        
        auto elapsed = std::chrono::high_resolution_clock::now() - start;
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed);
        
        std::cout << "Smoke test passed in " << ms.count() << "ms" << std::endl;
        return ms.count() < 1000;  // Must complete in <1 second
    }
};

// Temporary stub implementations - will be replaced in Phase 1
// These are here just to allow compilation
inline Bitboard MagicValidator::computeRookMask(Square sq) {
    // Temporary: return empty mask
    return 0;
}

inline Bitboard MagicValidator::computeBishopMask(Square sq) {
    // Temporary: return empty mask
    return 0;
}

inline Bitboard MagicValidator::magicRookAttacks(Square sq, Bitboard occupied) {
    // Temporary: use ray-based implementation
    return ::seajay::rookAttacks(sq, occupied);
}

inline Bitboard MagicValidator::magicBishopAttacks(Square sq, Bitboard occupied) {
    // Temporary: use ray-based implementation
    return ::seajay::bishopAttacks(sq, occupied);
}

} // namespace seajay