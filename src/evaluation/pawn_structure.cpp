#include "pawn_structure.h"
#include <cstring>
#include <bit>  // DP1: For std::popcount

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
    Bitboard passed = 0ULL;
    Bitboard pawns = ourPawns;
    
    while (pawns) {
        Square sq = popLsb(pawns);
        if (isPassed(c, sq, theirPawns)) {
            passed |= (1ULL << sq);
        }
    }
    
    return passed;
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
    Bitboard isolated = 0ULL;
    Bitboard pawns = ourPawns;
    
    while (pawns) {
        Square sq = popLsb(pawns);
        if (isIsolated(sq, ourPawns)) {
            isolated |= (1ULL << sq);
        }
    }
    
    return isolated;
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
    Bitboard doubled = 0ULL;
    
    // Process each file
    for (int file = 0; file < 8; file++) {
        Bitboard fileMask = FILE_A_BB << file;
        Bitboard pawnsOnFile = ourPawns & fileMask;
        
        int pawnCount = std::popcount(pawnsOnFile);
        
        // If more than one pawn on this file, mark all but the base pawn as doubled
        if (pawnCount > 1) {
            // Mark all pawns on this file as doubled except the rearmost one
            // For white, the rearmost is the one with lowest rank
            // For black, the rearmost is the one with highest rank
            Bitboard doublePawns = pawnsOnFile;
            
            if (c == WHITE) {
                // Remove the rearmost (lowest rank) pawn
                Square rearmost = lsb(pawnsOnFile);
                doublePawns &= ~(1ULL << rearmost);
            } else {
                // Remove the rearmost (highest rank) pawn
                Square rearmost = msb(pawnsOnFile);
                doublePawns &= ~(1ULL << rearmost);
            }
            
            doubled |= doublePawns;
        }
    }
    
    return doubled;
}

// PI1: Pawn island counting implementation
int PawnStructure::countPawnIslands(Bitboard ourPawns) {
    if (!ourPawns) return 0;
    
    int islands = 0;
    Bitboard remaining = ourPawns;
    
    // Process files from a to h
    // An island starts when we find pawns after a gap (empty file)
    bool previousFileHadPawns = false;
    
    for (int file = 0; file < 8; ++file) {
        Bitboard fileMask = fileBB(file);
        bool currentFileHasPawns = (remaining & fileMask) != 0;
        
        if (currentFileHasPawns) {
            // If previous file had no pawns, this starts a new island
            if (!previousFileHadPawns) {
                islands++;
            }
            // Otherwise, pawns are connected to previous file's pawns
        }
        
        previousFileHadPawns = currentFileHasPawns;
    }
    
    return islands;
}

int PawnStructure::getPawnIslands(Color c, Bitboard ourPawns) {
    // For Phase PI1, just compute directly without caching
    // Caching will be added when we integrate into evaluation
    return countPawnIslands(ourPawns);
}

}