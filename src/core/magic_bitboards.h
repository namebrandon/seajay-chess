/**
 * Magic Bitboards Implementation - Header-Only Version
 * 
 * This header-only implementation avoids static initialization order issues
 * by using inline variables (C++17) and lazy initialization.
 * 
 * Magic bitboards provide constant-time lookup for sliding piece attacks.
 * 
 * GPL-3.0 License
 */

#pragma once

#include "types.h"
#include "bitboard.h"
#include "magic_constants.h"
#include <array>
#include <memory>
#include <mutex>
#include <cstring>
#include <vector>
#include <iostream>
#include <cassert>

namespace seajay {

// Forward declarations for magic bitboard functions
Bitboard magicRookAttacks(Square sq, Bitboard occupied);
Bitboard magicBishopAttacks(Square sq, Bitboard occupied);
Bitboard magicQueenAttacks(Square sq, Bitboard occupied);

namespace magic {

// Magic entry structure for each square
struct MagicEntry {
    Bitboard mask;      // Relevant occupancy mask (excludes edges)
    Bitboard magic;     // Magic number for this square
    Bitboard* attacks;  // Pointer to attack table
    uint8_t shift;      // Right shift amount (64 - popcount(mask))
};

// Inline variables ensure single definition across translation units (C++17)
inline MagicEntry rookMagics[64] = {};
inline MagicEntry bishopMagics[64] = {};

// Attack tables - allocated once and pointed to by MagicEntry
inline std::unique_ptr<Bitboard[]> rookAttackTable;
inline std::unique_ptr<Bitboard[]> bishopAttackTable;

// Initialization flag and mutex
inline std::mutex initMutex;
inline bool magicsInitialized = false;

/**
 * Compute the blocker mask for a rook on the given square.
 * The mask includes all squares the rook can potentially move to,
 * EXCLUDING the edge squares (as pieces on edges don't affect inner squares).
 */
inline Bitboard computeRookMask(Square sq) {
    Bitboard mask = 0;
    int f = static_cast<int>(fileOf(sq));
    int r = static_cast<int>(rankOf(sq));
    
    // North ray (exclude rank 8)
    for (int r2 = r + 1; r2 < 7; ++r2) {
        mask |= squareBB(makeSquare(static_cast<File>(f), static_cast<Rank>(r2)));
    }
    
    // South ray (exclude rank 1)
    for (int r2 = r - 1; r2 > 0; --r2) {
        mask |= squareBB(makeSquare(static_cast<File>(f), static_cast<Rank>(r2)));
    }
    
    // East ray (exclude file H)
    for (int f2 = f + 1; f2 < 7; ++f2) {
        mask |= squareBB(makeSquare(static_cast<File>(f2), static_cast<Rank>(r)));
    }
    
    // West ray (exclude file A)
    for (int f2 = f - 1; f2 > 0; --f2) {
        mask |= squareBB(makeSquare(static_cast<File>(f2), static_cast<Rank>(r)));
    }
    
    return mask;
}

/**
 * Compute the blocker mask for a bishop on the given square.
 * The mask includes all squares the bishop can potentially move to,
 * EXCLUDING the edge squares.
 */
inline Bitboard computeBishopMask(Square sq) {
    Bitboard mask = 0;
    int f = static_cast<int>(fileOf(sq));
    int r = static_cast<int>(rankOf(sq));
    
    // North-East diagonal (exclude rank 8 and file H)
    for (int f2 = f + 1, r2 = r + 1; f2 < 7 && r2 < 7; ++f2, ++r2) {
        mask |= squareBB(makeSquare(static_cast<File>(f2), static_cast<Rank>(r2)));
    }
    
    // North-West diagonal (exclude rank 8 and file A)
    for (int f2 = f - 1, r2 = r + 1; f2 > 0 && r2 < 7; --f2, ++r2) {
        mask |= squareBB(makeSquare(static_cast<File>(f2), static_cast<Rank>(r2)));
    }
    
    // South-East diagonal (exclude rank 1 and file H)
    for (int f2 = f + 1, r2 = r - 1; f2 < 7 && r2 > 0; ++f2, --r2) {
        mask |= squareBB(makeSquare(static_cast<File>(f2), static_cast<Rank>(r2)));
    }
    
    // South-West diagonal (exclude rank 1 and file A)
    for (int f2 = f - 1, r2 = r - 1; f2 > 0 && r2 > 0; --f2, --r2) {
        mask |= squareBB(makeSquare(static_cast<File>(f2), static_cast<Rank>(r2)));
    }
    
    return mask;
}

/**
 * Convert an index to an occupancy pattern using the given mask.
 */
inline Bitboard indexToOccupancy(int index, Bitboard mask) {
    Bitboard occupancy = 0;
    Bitboard maskCopy = mask;
    int bitCount = popCount(mask);
    
    for (int i = 0; i < bitCount; ++i) {
        Square sq = popLsb(maskCopy);
        if (index & (1 << i)) {
            occupancy |= squareBB(sq);
        }
    }
    
    return occupancy;
}

/**
 * Generate slow attack bitboard for validation purposes.
 */
inline Bitboard generateSlowRookAttacks(Square sq, Bitboard occupied) {
    Bitboard attacks = 0;
    int rank = rankOf(sq);
    int file = fileOf(sq);
    
    // North
    for (int r = rank + 1; r < 8; r++) {
        attacks |= squareBB(makeSquare(static_cast<File>(file), static_cast<Rank>(r)));
        if (occupied & squareBB(makeSquare(static_cast<File>(file), static_cast<Rank>(r)))) break;
    }
    // South
    for (int r = rank - 1; r >= 0; r--) {
        attacks |= squareBB(makeSquare(static_cast<File>(file), static_cast<Rank>(r)));
        if (occupied & squareBB(makeSquare(static_cast<File>(file), static_cast<Rank>(r)))) break;
    }
    // East
    for (int f = file + 1; f < 8; f++) {
        attacks |= squareBB(makeSquare(static_cast<File>(f), static_cast<Rank>(rank)));
        if (occupied & squareBB(makeSquare(static_cast<File>(f), static_cast<Rank>(rank)))) break;
    }
    // West
    for (int f = file - 1; f >= 0; f--) {
        attacks |= squareBB(makeSquare(static_cast<File>(f), static_cast<Rank>(rank)));
        if (occupied & squareBB(makeSquare(static_cast<File>(f), static_cast<Rank>(rank)))) break;
    }
    
    return attacks;
}

inline Bitboard generateSlowBishopAttacks(Square sq, Bitboard occupied) {
    Bitboard attacks = 0;
    int rank = rankOf(sq);
    int file = fileOf(sq);
    
    // North-East
    for (int r = rank + 1, f = file + 1; r < 8 && f < 8; r++, f++) {
        attacks |= squareBB(makeSquare(static_cast<File>(f), static_cast<Rank>(r)));
        if (occupied & squareBB(makeSquare(static_cast<File>(f), static_cast<Rank>(r)))) break;
    }
    // North-West
    for (int r = rank + 1, f = file - 1; r < 8 && f >= 0; r++, f--) {
        attacks |= squareBB(makeSquare(static_cast<File>(f), static_cast<Rank>(r)));
        if (occupied & squareBB(makeSquare(static_cast<File>(f), static_cast<Rank>(r)))) break;
    }
    // South-East
    for (int r = rank - 1, f = file + 1; r >= 0 && f < 8; r--, f++) {
        attacks |= squareBB(makeSquare(static_cast<File>(f), static_cast<Rank>(r)));
        if (occupied & squareBB(makeSquare(static_cast<File>(f), static_cast<Rank>(r)))) break;
    }
    // South-West
    for (int r = rank - 1, f = file - 1; r >= 0 && f >= 0; r--, f--) {
        attacks |= squareBB(makeSquare(static_cast<File>(f), static_cast<Rank>(r)));
        if (occupied & squareBB(makeSquare(static_cast<File>(f), static_cast<Rank>(r)))) break;
    }
    
    return attacks;
}

// Forward declaration for internal initialization
void initMagicsInternal();

/**
 * Initialize magic bitboards - Complete Phase 2 implementation
 */
inline void initMagicsInternal() {
    // Debug output removed for production use
    
    // Phase 1D: Initialize MagicEntry structures
    for (Square sq = 0; sq < 64; ++sq) {
        // Initialize rook entries
        rookMagics[sq].mask = computeRookMask(sq);
        rookMagics[sq].magic = ROOK_MAGICS[sq];
        rookMagics[sq].shift = ROOK_SHIFTS[sq];
        rookMagics[sq].attacks = nullptr;
        
        // Initialize bishop entries
        bishopMagics[sq].mask = computeBishopMask(sq);
        bishopMagics[sq].magic = BISHOP_MAGICS[sq];
        bishopMagics[sq].shift = BISHOP_SHIFTS[sq];
        bishopMagics[sq].attacks = nullptr;
    }
    
    // Phase 2A: Table Memory Allocation
    // Calculate total table sizes
    size_t rookTableTotal = 0;
    size_t bishopTableTotal = 0;
    
    for (Square sq = 0; sq < 64; ++sq) {
        rookTableTotal += (1ULL << (64 - rookMagics[sq].shift));
        bishopTableTotal += (1ULL << (64 - bishopMagics[sq].shift));
    }
    
    // Allocate memory for attack tables
    rookAttackTable = std::make_unique<Bitboard[]>(rookTableTotal);
    bishopAttackTable = std::make_unique<Bitboard[]>(bishopTableTotal);
    
    // Zero-initialize to prevent undefined behavior
    std::memset(rookAttackTable.get(), 0, rookTableTotal * sizeof(Bitboard));
    std::memset(bishopAttackTable.get(), 0, bishopTableTotal * sizeof(Bitboard));
    
    // Phase 2C: Generate All Rook Attack Tables
    Bitboard* rookTablePtr = rookAttackTable.get();
    
    for (Square sq = 0; sq < 64; ++sq) {
        rookMagics[sq].attacks = rookTablePtr;
        
        size_t tableSize = 1ULL << (64 - rookMagics[sq].shift);
        int numPatterns = 1 << popCount(rookMagics[sq].mask);
        
        // Generate all attack patterns
        for (int pattern = 0; pattern < numPatterns; ++pattern) {
            Bitboard occupancy = indexToOccupancy(pattern, rookMagics[sq].mask);
            Bitboard attacks = generateSlowRookAttacks(sq, occupancy);
            
            uint64_t index = ((uint64_t)(occupancy & rookMagics[sq].mask) * 
                             (uint64_t)rookMagics[sq].magic) >> rookMagics[sq].shift;
            
            // Store attacks (constructive collisions are OK)
            rookTablePtr[index] = attacks;
        }
        
        rookTablePtr += tableSize;
    }
    
    // Phase 2D: Generate All Bishop Attack Tables
    Bitboard* bishopTablePtr = bishopAttackTable.get();
    
    for (Square sq = 0; sq < 64; ++sq) {
        bishopMagics[sq].attacks = bishopTablePtr;
        
        size_t tableSize = 1ULL << (64 - bishopMagics[sq].shift);
        int numPatterns = 1 << popCount(bishopMagics[sq].mask);
        
        // Generate all attack patterns
        for (int pattern = 0; pattern < numPatterns; ++pattern) {
            Bitboard occupancy = indexToOccupancy(pattern, bishopMagics[sq].mask);
            Bitboard attacks = generateSlowBishopAttacks(sq, occupancy);
            
            uint64_t index = ((uint64_t)(occupancy & bishopMagics[sq].mask) * 
                             (uint64_t)bishopMagics[sq].magic) >> bishopMagics[sq].shift;
            
            // Store attacks (constructive collisions are OK)
            bishopTablePtr[index] = attacks;
        }
        
        bishopTablePtr += tableSize;
    }
    
    // Phase 2E: Mark Initialization Complete
    magicsInitialized = true;
}

/**
 * Initialize magic bitboards (thread-safe, called once)
 */
inline void initMagics() {
    if (magicsInitialized) return;
    
    std::lock_guard<std::mutex> lock(initMutex);
    if (magicsInitialized) return;  // Double-check
    
    initMagicsInternal();
}

/**
 * Check if magic bitboards are initialized.
 */
inline bool areMagicsInitialized() {
    return magicsInitialized;
}

/**
 * Ensure magic bitboards are initialized.
 */
inline void ensureMagicsInitialized() {
    if (!magicsInitialized) {
        initMagics();
    }
}

/**
 * Validate a magic number (for testing)
 */
inline bool validateMagicNumber(Square sq, bool isRook) {
    Bitboard mask = isRook ? computeRookMask(sq) : computeBishopMask(sq);
    Bitboard magic = isRook ? ROOK_MAGICS[sq] : BISHOP_MAGICS[sq];
    uint8_t shift = isRook ? ROOK_SHIFTS[sq] : BISHOP_SHIFTS[sq];
    
    int numBits = popCount(mask);
    int tableSize = 1 << numBits;
    
    std::vector<bool> usedIndices(1 << (64 - shift), false);
    std::vector<Bitboard> attacksForIndex(1 << (64 - shift), 0);
    
    for (int pattern = 0; pattern < tableSize; ++pattern) {
        Bitboard occupancy = indexToOccupancy(pattern, mask);
        Bitboard attacks = isRook ? 
            generateSlowRookAttacks(sq, occupancy) : 
            generateSlowBishopAttacks(sq, occupancy);
        
        uint64_t index = ((uint64_t)(occupancy & mask) * (uint64_t)magic) >> shift;
        
        if (usedIndices[index]) {
            if (attacksForIndex[index] != attacks) {
                // Destructive collision
                return false;
            }
        } else {
            usedIndices[index] = true;
            attacksForIndex[index] = attacks;
        }
    }
    
    return true;
}

// Placeholder for printMaskInfo - not critical for Phase 2
inline void printMaskInfo() {
    // This function can be implemented if needed for debugging
}

} // namespace magic

// Inline implementations for fast attack generation
inline Bitboard magicRookAttacks(Square sq, Bitboard occupied) {
    #ifdef DEBUG
        assert(sq >= 0 && sq < 64);
        assert(magic::areMagicsInitialized() && "Magic bitboards not initialized!");
    #endif
    
    magic::ensureMagicsInitialized();
    const auto& entry = magic::rookMagics[sq];
    occupied &= entry.mask;
    occupied *= entry.magic;
    occupied >>= entry.shift;
    
    #ifdef DEBUG
        size_t tableSize = 1ULL << (64 - entry.shift);
        assert(occupied < tableSize && "Magic index out of bounds!");
    #endif
    
    return entry.attacks[occupied];
}

inline Bitboard magicBishopAttacks(Square sq, Bitboard occupied) {
    #ifdef DEBUG
        assert(sq >= 0 && sq < 64);
        assert(magic::areMagicsInitialized() && "Magic bitboards not initialized!");
    #endif
    
    magic::ensureMagicsInitialized();
    const auto& entry = magic::bishopMagics[sq];
    occupied &= entry.mask;
    occupied *= entry.magic;
    occupied >>= entry.shift;
    
    #ifdef DEBUG
        size_t tableSize = 1ULL << (64 - entry.shift);
        assert(occupied < tableSize && "Magic index out of bounds!");
    #endif
    
    return entry.attacks[occupied];
}

inline Bitboard magicQueenAttacks(Square sq, Bitboard occupied) {
    #ifdef DEBUG
        assert(sq >= 0 && sq < 64);
    #endif
    return magicRookAttacks(sq, occupied) | magicBishopAttacks(sq, occupied);
}

} // namespace seajay