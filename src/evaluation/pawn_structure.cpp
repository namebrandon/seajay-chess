#include "pawn_structure.h"
#include <cstring>
#include <bit>  // DP1: For std::popcount
#include "../core/simd_utils.h"  // Phase 2.5.e-2: SIMD optimizations

namespace seajay {

Bitboard PawnStructure::m_passedPawnMask[2][64] = {};
Bitboard PawnStructure::m_candidateMask[2][64] = {};

PawnStructure g_pawnStructure;

PawnStructure::PawnStructure() : m_table(nullptr), m_size(PAWN_HASH_SIZE) {
    m_table = new PawnEntry[m_size];
    clear();
}

PawnStructure::~PawnStructure() {
    delete[] m_table;
}

void PawnStructure::clear() {
    std::memset(m_table, 0, m_size * sizeof(PawnEntry));
}

void PawnStructure::initPassedPawnMasks() {
    initMasks();
}

void PawnStructure::initMasks() {
    for (int sq = 0; sq < 64; ++sq) {
        int file = fileOf(Square(sq));
        int rank = rankOf(Square(sq));
        
        m_passedPawnMask[WHITE][sq] = 0ULL;
        m_passedPawnMask[BLACK][sq] = 0ULL;
        m_candidateMask[WHITE][sq] = 0ULL;
        m_candidateMask[BLACK][sq] = 0ULL;
        
        for (int r = rank + 1; r <= 7; ++r) {
            if (file > 0) {
                m_passedPawnMask[WHITE][sq] |= (1ULL << (r * 8 + file - 1));
            }
            m_passedPawnMask[WHITE][sq] |= (1ULL << (r * 8 + file));
            if (file < 7) {
                m_passedPawnMask[WHITE][sq] |= (1ULL << (r * 8 + file + 1));
            }
        }
        
        for (int r = rank - 1; r >= 0; --r) {
            if (file > 0) {
                m_passedPawnMask[BLACK][sq] |= (1ULL << (r * 8 + file - 1));
            }
            m_passedPawnMask[BLACK][sq] |= (1ULL << (r * 8 + file));
            if (file < 7) {
                m_passedPawnMask[BLACK][sq] |= (1ULL << (r * 8 + file + 1));
            }
        }
        
        if (rank < 6 && rank > 0) {
            int candidateRank = (sq < SQ_A8) ? rank + 1 : rank;
            if (candidateRank <= 6) {
                for (int r = candidateRank + 1; r <= 7; ++r) {
                    if (file > 0) {
                        m_candidateMask[WHITE][sq] |= (1ULL << (r * 8 + file - 1));
                    }
                    if (file < 7) {
                        m_candidateMask[WHITE][sq] |= (1ULL << (r * 8 + file + 1));
                    }
                }
            }
        }
        
        if (rank > 1 && rank < 7) {
            int candidateRank = (sq > SQ_H1) ? rank - 1 : rank;
            if (candidateRank >= 1) {
                for (int r = candidateRank - 1; r >= 0; --r) {
                    if (file > 0) {
                        m_candidateMask[BLACK][sq] |= (1ULL << (r * 8 + file - 1));
                    }
                    if (file < 7) {
                        m_candidateMask[BLACK][sq] |= (1ULL << (r * 8 + file + 1));
                    }
                }
            }
        }
    }
}

bool PawnStructure::isPassed(Color us, Square sq, Bitboard theirPawns) {
    return !(theirPawns & m_passedPawnMask[us][sq]);
}

bool PawnStructure::isCandidate(Color us, Square sq, Bitboard ourPawns, Bitboard theirPawns) {
    if (isPassed(us, sq, theirPawns)) {
        return false;
    }
    
    Square pushSq = (us == WHITE) ? Square(sq + 8) : Square(sq - 8);
    if (pushSq < SQ_A1 || pushSq > SQ_H8) {
        return false;
    }
    
    Bitboard occupancy = ourPawns | theirPawns;
    if (occupancy & (1ULL << pushSq)) {
        return false;
    }
    
    return !(theirPawns & m_candidateMask[us][sq]);
}

PawnEntry* PawnStructure::probe(uint64_t pawnKey) {
    size_t index = pawnKey % m_size;
    PawnEntry* entry = &m_table[index];
    
    if (entry->key == pawnKey && entry->valid) {
#ifdef DEBUG
        m_cacheHits++;
#endif
        return entry;
    }
    
#ifdef DEBUG
    m_cacheMisses++;
#endif
    return nullptr;
}

void PawnStructure::store(uint64_t pawnKey, const PawnEntry& entry) {
    size_t index = pawnKey % m_size;
    m_table[index] = entry;
    m_table[index].key = pawnKey;
    m_table[index].valid = true;
}

Bitboard PawnStructure::getPassedPawns(Color c, Bitboard ourPawns, Bitboard theirPawns) {
    // Phase 2.5.e-2: Use SIMD-optimized version for better performance
    return simd::getPassedPawnsFast(ourPawns, theirPawns, m_passedPawnMask[c]);
}

Bitboard PawnStructure::getCandidatePassers(Color c, Bitboard ourPawns, Bitboard theirPawns) {
    Bitboard candidates = 0ULL;
    Bitboard pawns = ourPawns;
    
    while (pawns) {
        Square sq = popLsb(pawns);
        if (isCandidate(c, sq, ourPawns, theirPawns)) {
            candidates |= (1ULL << sq);
        }
    }
    
    return candidates;
}

bool PawnStructure::isIsolated(Square sq, Bitboard ourPawns) {
    int file = fileOf(sq);
    Bitboard adjacentFiles = 0ULL;
    
    // Check for friendly pawns on adjacent files
    // CRITICAL: Always check file boundaries
    if (file > 0) {
        adjacentFiles |= FILE_A_BB << (file - 1);
    }
    if (file < 7) {
        adjacentFiles |= FILE_A_BB << (file + 1);
    }
    
    // If no friendly pawns on adjacent files, pawn is isolated
    return !(ourPawns & adjacentFiles);
}

Bitboard PawnStructure::getIsolatedPawns(Color c, Bitboard ourPawns) {
    // Phase 2.5.e-2: Use SIMD-optimized version for better performance
    return simd::getIsolatedPawnsFast(ourPawns);
}

// DP1: Doubled pawn detection implementation
bool PawnStructure::isDoubled(Square sq, Bitboard ourPawns) {
    int file = fileOf(sq);
    Bitboard fileMask = FILE_A_BB << file;
    Bitboard pawnsOnFile = ourPawns & fileMask;
    
    // Remove the pawn at sq from consideration
    pawnsOnFile &= ~(1ULL << sq);
    
    // If there are any other pawns on this file, this pawn is doubled
    return pawnsOnFile != 0;
}

int PawnStructure::countDoubledOnFile(int file, Bitboard ourPawns) {
    Bitboard fileMask = FILE_A_BB << file;
    Bitboard pawnsOnFile = ourPawns & fileMask;
    
    // Count pawns on this file
    int count = std::popcount(pawnsOnFile);
    
    // Return doubled count (count - 1, since the base pawn doesn't count)
    // If 0 or 1 pawns, return 0 (no doubled pawns)
    return (count > 1) ? (count - 1) : 0;
}

Bitboard PawnStructure::getDoubledPawns(Color c, Bitboard ourPawns) {
    // Phase 2.5.e-2: Use SIMD-optimized version for better performance
    return simd::getDoubledPawnsFast(ourPawns, c == WHITE);
}

// PI1: Pawn island counting implementation
int PawnStructure::countPawnIslands(Bitboard ourPawns) {
    // Phase 2.5.e-2: Use SIMD-optimized version for better performance
    return simd::countPawnIslandsFast(ourPawns);
}

int PawnStructure::getPawnIslands(Color c, Bitboard ourPawns) {
    // PI2: Now returns cached value from probe/store cycle in evaluate.cpp
    // The actual caching happens in evaluate.cpp when it computes all pawn features
    return countPawnIslands(ourPawns);
}

// BP1: Backward pawn detection implementation
bool PawnStructure::isBackward(Color us, Square sq, Bitboard ourPawns, Bitboard theirPawns) {
    // A pawn is backward if:
    // 1. It's not on its starting rank (2nd for white, 7th for black)
    // 2. No friendly pawns on adjacent files can support it (are behind or level with it)
    // 3. The square in front is controlled by enemy pawns
    // 4. It cannot safely advance
    // 5. NOT already isolated (to avoid double penalty)
    
    int rank = rankOf(sq);
    int file = fileOf(sq);
    
    // Don't count isolated pawns as backward (they're already penalized)
    if (isIsolated(sq, ourPawns)) {
        return false;
    }
    
    // Not backward if on starting rank
    if ((us == WHITE && rank == 1) || (us == BLACK && rank == 6)) {
        return false;
    }
    
    // Check for support from adjacent files
    bool hasSupport = false;
    Bitboard adjacentFiles = 0ULL;
    
    if (file > 0) adjacentFiles |= fileBB(file - 1);
    if (file < 7) adjacentFiles |= fileBB(file + 1);
    
    Bitboard supportingPawns = ourPawns & adjacentFiles;
    
    // For white, supporting pawns must be on same rank or behind (lower rank)
    // For black, supporting pawns must be on same rank or behind (higher rank)
    while (supportingPawns) {
        Square supportSq = popLsb(supportingPawns);
        int supportRank = rankOf(supportSq);
        
        if (us == WHITE) {
            if (supportRank <= rank) {
                hasSupport = true;
                break;
            }
        } else {
            if (supportRank >= rank) {
                hasSupport = true;
                break;
            }
        }
    }
    
    // If we have support, not backward
    if (hasSupport) {
        return false;
    }
    
    // Check if the square in front is attacked by enemy pawns
    Square frontSq = (us == WHITE) ? Square(sq + 8) : Square(sq - 8);
    
    // Make sure front square is valid (not off board)
    if (frontSq < 0 || frontSq >= 64) {
        return false;  // Pawn at edge, can't advance anyway
    }
    
    // Check if enemy pawns attack the front square
    bool frontSquareAttacked = false;
    
    if (us == WHITE) {
        // Check if black pawns can attack the front square
        // Black pawns attack from northeast (+7) and northwest (+9) relative to their position
        // So from the front square's perspective, black pawns would be at +7 and +9
        if (fileOf(frontSq) > 0 && rankOf(frontSq) < 7) {
            Square attackerSq = Square(frontSq + 7);  // Black pawn that could attack from left
            if ((theirPawns & (1ULL << attackerSq))) {
                frontSquareAttacked = true;
            }
        }
        if (fileOf(frontSq) < 7 && rankOf(frontSq) < 7) {
            Square attackerSq = Square(frontSq + 9);  // Black pawn that could attack from right
            if ((theirPawns & (1ULL << attackerSq))) {
                frontSquareAttacked = true;
            }
        }
    } else {
        // Check if white pawns can attack the front square
        // White pawns attack from southeast (-7) and southwest (-9) relative to their position
        // So from the front square's perspective, white pawns would be at -7 and -9
        if (fileOf(frontSq) < 7 && rankOf(frontSq) > 0) {
            Square attackerSq = Square(frontSq - 7);  // White pawn that could attack from right
            if ((theirPawns & (1ULL << attackerSq))) {
                frontSquareAttacked = true;
            }
        }
        if (fileOf(frontSq) > 0 && rankOf(frontSq) > 0) {
            Square attackerSq = Square(frontSq - 9);  // White pawn that could attack from left
            if ((theirPawns & (1ULL << attackerSq))) {
                frontSquareAttacked = true;
            }
        }
    }
    
    // If front square is not attacked by enemy pawns, not backward
    if (!frontSquareAttacked) {
        return false;
    }
    
    // Additional check: the pawn should not be able to advance safely
    // This means there's an enemy pawn blocking or controlling its advance
    return true;
}

Bitboard PawnStructure::getBackwardPawns(Color c, Bitboard ourPawns, Bitboard theirPawns) {
    // Phase 2.5.e-2: Use SIMD-optimized version for better performance
    // First get isolated pawns to avoid double-penalizing them
    Bitboard isolatedPawns = simd::getIsolatedPawnsFast(ourPawns);
    return simd::getBackwardPawnsFast(ourPawns, theirPawns, c == WHITE, isolatedPawns);
}

}