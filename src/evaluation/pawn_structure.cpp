#include "pawn_structure.h"
#include <cstring>

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
        return entry;
    }
    
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

}