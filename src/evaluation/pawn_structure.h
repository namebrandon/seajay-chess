#ifndef PAWN_STRUCTURE_H
#define PAWN_STRUCTURE_H

#include "../core/types.h"
#include "../core/bitboard.h"
#include <cstdint>

namespace seajay {

struct PawnEntry {
    uint64_t key;
    Bitboard passedPawns[2];
    Bitboard candidatePassers[2];
    Bitboard isolatedPawns[2];  // Track isolated pawns for both colors
    Bitboard doubledPawns[2];   // DP1: Track doubled pawns for both colors
    int16_t score;
    bool valid;
    
    PawnEntry() : key(0), passedPawns{0, 0}, candidatePassers{0, 0}, isolatedPawns{0, 0}, doubledPawns{0, 0}, score(0), valid(false) {}
};

class PawnStructure {
public:
    static constexpr size_t PAWN_HASH_SIZE = 16384;
    
#ifdef DEBUG
    mutable size_t m_cacheHits = 0;
    mutable size_t m_cacheMisses = 0;
#endif
    
    PawnStructure();
    ~PawnStructure();
    
    static void initPassedPawnMasks();
    
    static inline int relativeRank(Color c, int rank) {
        return c == WHITE ? rank : 7 - rank;
    }
    
    static inline int relativeRank(Color c, Square sq) {
        int rank = rankOf(sq);
        return c == WHITE ? rank : 7 - rank;
    }
    
    static bool isPassed(Color us, Square sq, Bitboard theirPawns);
    
    static bool isCandidate(Color us, Square sq, Bitboard ourPawns, Bitboard theirPawns);
    
    PawnEntry* probe(uint64_t pawnKey);
    
    void store(uint64_t pawnKey, const PawnEntry& entry);
    
    void clear();
    
    Bitboard getPassedPawns(Color c, Bitboard ourPawns, Bitboard theirPawns);
    
    Bitboard getCandidatePassers(Color c, Bitboard ourPawns, Bitboard theirPawns);
    
    // Isolated pawn detection
    static bool isIsolated(Square sq, Bitboard ourPawns);
    
    Bitboard getIsolatedPawns(Color c, Bitboard ourPawns);
    
    // DP1: Doubled pawn detection infrastructure
    // Check if a pawn at the given square is doubled (has another pawn on same file)
    static bool isDoubled(Square sq, Bitboard ourPawns);
    
    // Get bitboard of all doubled pawns for a color
    // Note: Returns ALL pawns on files with multiple pawns (except the base pawn)
    Bitboard getDoubledPawns(Color c, Bitboard ourPawns);
    
    // Count doubled pawns on a file (returns count - 1, so 3 pawns = 2 doubled)
    static int countDoubledOnFile(int file, Bitboard ourPawns);

private:
    static Bitboard m_passedPawnMask[2][64];
    static Bitboard m_candidateMask[2][64];
    
    PawnEntry* m_table;
    size_t m_size;
    
    static void initMasks();
};

extern PawnStructure g_pawnStructure;

}

#endif