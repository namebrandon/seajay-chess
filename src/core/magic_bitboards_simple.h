/**
 * Magic Bitboards Implementation - Simplified Header-Only Version
 * 
 * This version uses a simpler initialization pattern to avoid
 * static initialization order issues.
 * 
 * GPL-3.0 License
 */

#pragma once

#include "types.h"
#include "bitboard.h"
#include "magic_constants.h"
#include <memory>
#include <cstring>

namespace seajay {
namespace magic_simple {

// Magic entry structure for each square
struct MagicEntry {
    Bitboard mask;      // Relevant occupancy mask (excludes edges)
    Bitboard magic;     // Magic number for this square
    Bitboard* attacks;  // Pointer to attack table
    uint8_t shift;      // Right shift amount
};

// Global data structure holding all magic bitboard data
struct MagicData {
    MagicEntry rookMagics[64];
    MagicEntry bishopMagics[64];
    std::unique_ptr<Bitboard[]> rookAttackTable;
    std::unique_ptr<Bitboard[]> bishopAttackTable;
    bool initialized = false;
};

// Helper functions
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

inline Bitboard computeBishopMask(Square sq) {
    Bitboard mask = 0;
    int f = static_cast<int>(fileOf(sq));
    int r = static_cast<int>(rankOf(sq));
    
    // North-East diagonal
    for (int f2 = f + 1, r2 = r + 1; f2 < 7 && r2 < 7; ++f2, ++r2) {
        mask |= squareBB(makeSquare(static_cast<File>(f2), static_cast<Rank>(r2)));
    }
    
    // North-West diagonal
    for (int f2 = f - 1, r2 = r + 1; f2 > 0 && r2 < 7; --f2, ++r2) {
        mask |= squareBB(makeSquare(static_cast<File>(f2), static_cast<Rank>(r2)));
    }
    
    // South-East diagonal
    for (int f2 = f + 1, r2 = r - 1; f2 < 7 && r2 > 0; ++f2, --r2) {
        mask |= squareBB(makeSquare(static_cast<File>(f2), static_cast<Rank>(r2)));
    }
    
    // South-West diagonal
    for (int f2 = f - 1, r2 = r - 1; f2 > 0 && r2 > 0; --f2, --r2) {
        mask |= squareBB(makeSquare(static_cast<File>(f2), static_cast<Rank>(r2)));
    }
    
    return mask;
}

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

// Get singleton instance of magic data (Meyer's Singleton)
inline MagicData& getMagicData() {
    static MagicData data;
    
    if (!data.initialized) {
        // Initialize MagicEntry structures
        for (Square sq = 0; sq < 64; ++sq) {
            // Initialize rook entries
            data.rookMagics[sq].mask = computeRookMask(sq);
            data.rookMagics[sq].magic = magic::ROOK_MAGICS[sq];
            data.rookMagics[sq].shift = magic::ROOK_SHIFTS[sq];
            
            // Initialize bishop entries
            data.bishopMagics[sq].mask = computeBishopMask(sq);
            data.bishopMagics[sq].magic = magic::BISHOP_MAGICS[sq];
            data.bishopMagics[sq].shift = magic::BISHOP_SHIFTS[sq];
        }
        
        // Calculate total table sizes
        size_t rookTableTotal = 0;
        size_t bishopTableTotal = 0;
        
        for (Square sq = 0; sq < 64; ++sq) {
            rookTableTotal += (1ULL << (64 - data.rookMagics[sq].shift));
            bishopTableTotal += (1ULL << (64 - data.bishopMagics[sq].shift));
        }
        
        // Allocate memory for attack tables
        data.rookAttackTable = std::make_unique<Bitboard[]>(rookTableTotal);
        data.bishopAttackTable = std::make_unique<Bitboard[]>(bishopTableTotal);
        
        // Zero-initialize
        std::memset(data.rookAttackTable.get(), 0, rookTableTotal * sizeof(Bitboard));
        std::memset(data.bishopAttackTable.get(), 0, bishopTableTotal * sizeof(Bitboard));
        
        // Generate rook attack tables
        Bitboard* rookTablePtr = data.rookAttackTable.get();
        for (Square sq = 0; sq < 64; ++sq) {
            data.rookMagics[sq].attacks = rookTablePtr;
            
            size_t tableSize = 1ULL << (64 - data.rookMagics[sq].shift);
            int numPatterns = 1 << popCount(data.rookMagics[sq].mask);
            
            for (int pattern = 0; pattern < numPatterns; ++pattern) {
                Bitboard occupancy = indexToOccupancy(pattern, data.rookMagics[sq].mask);
                Bitboard attacks = generateSlowRookAttacks(sq, occupancy);
                
                uint64_t index = ((uint64_t)(occupancy & data.rookMagics[sq].mask) * 
                                 (uint64_t)data.rookMagics[sq].magic) >> data.rookMagics[sq].shift;
                
                rookTablePtr[index] = attacks;
            }
            
            rookTablePtr += tableSize;
        }
        
        // Generate bishop attack tables
        Bitboard* bishopTablePtr = data.bishopAttackTable.get();
        for (Square sq = 0; sq < 64; ++sq) {
            data.bishopMagics[sq].attacks = bishopTablePtr;
            
            size_t tableSize = 1ULL << (64 - data.bishopMagics[sq].shift);
            int numPatterns = 1 << popCount(data.bishopMagics[sq].mask);
            
            for (int pattern = 0; pattern < numPatterns; ++pattern) {
                Bitboard occupancy = indexToOccupancy(pattern, data.bishopMagics[sq].mask);
                Bitboard attacks = generateSlowBishopAttacks(sq, occupancy);
                
                uint64_t index = ((uint64_t)(occupancy & data.bishopMagics[sq].mask) * 
                                 (uint64_t)data.bishopMagics[sq].magic) >> data.bishopMagics[sq].shift;
                
                bishopTablePtr[index] = attacks;
            }
            
            bishopTablePtr += tableSize;
        }
        
        data.initialized = true;
    }
    
    return data;
}

} // namespace magic_simple

// Fast attack generation functions
inline Bitboard magicRookAttacksSimple(Square sq, Bitboard occupied) {
    const auto& data = magic_simple::getMagicData();
    const auto& entry = data.rookMagics[sq];
    occupied &= entry.mask;
    occupied *= entry.magic;
    occupied >>= entry.shift;
    return entry.attacks[occupied];
}

inline Bitboard magicBishopAttacksSimple(Square sq, Bitboard occupied) {
    const auto& data = magic_simple::getMagicData();
    const auto& entry = data.bishopMagics[sq];
    occupied &= entry.mask;
    occupied *= entry.magic;
    occupied >>= entry.shift;
    return entry.attacks[occupied];
}

inline Bitboard magicQueenAttacksSimple(Square sq, Bitboard occupied) {
    return magicRookAttacksSimple(sq, occupied) | magicBishopAttacksSimple(sq, occupied);
}

} // namespace seajay