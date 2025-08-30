/**
 * Magic Bitboards Implementation - Header-Only Version
 * 
 * This header-only implementation avoids static initialization order issues
 * by using inline variables (C++17) and lazy initialization.
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

namespace seajay {
namespace magic_v2 {

// Magic entry structure for each square
struct MagicEntry {
    Bitboard mask;      // Relevant occupancy mask (excludes edges)
    Bitboard magic;     // Magic number for this square
    Bitboard* attacks;  // Pointer to attack table
    uint8_t shift;      // Right shift amount (64 - popcount(mask))
};

// Inline variables ensure single definition across translation units
inline MagicEntry rookMagics[64] = {};
inline MagicEntry bishopMagics[64] = {};

// Attack tables - allocated once and pointed to by MagicEntry
inline std::unique_ptr<Bitboard[]> rookAttackTable;
inline std::unique_ptr<Bitboard[]> bishopAttackTable;

// Initialization flag and mutex
inline std::once_flag initFlag;
inline bool magicsInitialized = false;

/**
 * Compute the blocker mask for a rook on the given square.
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

/**
 * Initialize magic bitboards
 * 
 * Pre-generates all attack tables for rooks and bishops using magic bitboards.
 * This initialization is performed once at startup and enables O(1) attack generation.
 */
inline void initMagicsInternal() {
    // Initialize MagicEntry structures
    for (Square sq = 0; sq < 64; ++sq) {
        // Initialize rook entries
        rookMagics[sq].mask = computeRookMask(sq);
        rookMagics[sq].magic = magic::ROOK_MAGICS[sq];
        rookMagics[sq].shift = magic::ROOK_SHIFTS[sq];
        rookMagics[sq].attacks = nullptr;
        
        // Initialize bishop entries
        bishopMagics[sq].mask = computeBishopMask(sq);
        bishopMagics[sq].magic = magic::BISHOP_MAGICS[sq];
        bishopMagics[sq].shift = magic::BISHOP_SHIFTS[sq];
        bishopMagics[sq].attacks = nullptr;
    }
    
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
    
    // Generate all rook attack tables
    Bitboard* rookTablePtr = rookAttackTable.get();
    
    for (Square sq = 0; sq < 64; ++sq) {
        rookMagics[sq].attacks = rookTablePtr;
        
        size_t tableSize = 1ULL << (64 - rookMagics[sq].shift);
        int numPatterns = 1 << popCount(rookMagics[sq].mask);
        
        // Generate all attack patterns for this square
        for (int pattern = 0; pattern < numPatterns; ++pattern) {
            Bitboard occupancy = indexToOccupancy(pattern, rookMagics[sq].mask);
            Bitboard attacks = generateSlowRookAttacks(sq, occupancy);
            
            uint64_t index = ((uint64_t)(occupancy & rookMagics[sq].mask) * 
                             (uint64_t)rookMagics[sq].magic) >> rookMagics[sq].shift;
            
            // Store attacks (constructive collisions are handled correctly)
            rookTablePtr[index] = attacks;
        }
        
        rookTablePtr += tableSize;
    }
    
    // Generate all bishop attack tables
    Bitboard* bishopTablePtr = bishopAttackTable.get();
    
    for (Square sq = 0; sq < 64; ++sq) {
        bishopMagics[sq].attacks = bishopTablePtr;
        
        size_t tableSize = 1ULL << (64 - bishopMagics[sq].shift);
        int numPatterns = 1 << popCount(bishopMagics[sq].mask);
        
        // Generate all attack patterns for this square
        for (int pattern = 0; pattern < numPatterns; ++pattern) {
            Bitboard occupancy = indexToOccupancy(pattern, bishopMagics[sq].mask);
            Bitboard attacks = generateSlowBishopAttacks(sq, occupancy);
            
            uint64_t index = ((uint64_t)(occupancy & bishopMagics[sq].mask) * 
                             (uint64_t)bishopMagics[sq].magic) >> bishopMagics[sq].shift;
            
            // Store attacks (constructive collisions are handled correctly)
            bishopTablePtr[index] = attacks;
        }
        
        bishopTablePtr += tableSize;
    }
    
    // Mark initialization complete
    magicsInitialized = true;
}

/**
 * Initialize magic bitboards (thread-safe, called once)
 */
inline void initMagics() {
    std::call_once(initFlag, initMagicsInternal);
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

} // namespace magic_v2

// Fast attack generation functions using magic bitboards
// Phase 3.3.a: Force inline and remove redundant initialization checks
// Also add prefetch hints for better cache performance
__attribute__((always_inline)) inline Bitboard magicRookAttacks(Square sq, Bitboard occupied) {
    // Initialization is done once at startup - no need to check every time
    const auto& entry = magic_v2::rookMagics[sq];
    
    // Compute index
    occupied &= entry.mask;
    occupied *= entry.magic;
    occupied >>= entry.shift;
    
    // Prefetch the attack entry for next access (helps with cache)
    #ifdef __builtin_prefetch
    __builtin_prefetch(&entry.attacks[occupied], 0, 1);
    #endif
    
    return entry.attacks[occupied];
}

__attribute__((always_inline)) inline Bitboard magicBishopAttacks(Square sq, Bitboard occupied) {
    // Initialization is done once at startup - no need to check every time
    const auto& entry = magic_v2::bishopMagics[sq];
    
    // Compute index
    occupied &= entry.mask;
    occupied *= entry.magic;
    occupied >>= entry.shift;
    
    // Prefetch the attack entry for next access (helps with cache)
    #ifdef __builtin_prefetch
    __builtin_prefetch(&entry.attacks[occupied], 0, 1);
    #endif
    
    return entry.attacks[occupied];
}

__attribute__((always_inline)) inline Bitboard magicQueenAttacks(Square sq, Bitboard occupied) {
    // Compute both in parallel for better instruction pipelining
    const auto& rookEntry = magic_v2::rookMagics[sq];
    const auto& bishopEntry = magic_v2::bishopMagics[sq];
    
    // Compute rook index
    Bitboard rookOcc = occupied & rookEntry.mask;
    rookOcc *= rookEntry.magic;
    rookOcc >>= rookEntry.shift;
    
    // Compute bishop index
    Bitboard bishopOcc = occupied & bishopEntry.mask;
    bishopOcc *= bishopEntry.magic;
    bishopOcc >>= bishopEntry.shift;
    
    // Return combined attacks
    return rookEntry.attacks[rookOcc] | bishopEntry.attacks[bishopOcc];
}

} // namespace seajay