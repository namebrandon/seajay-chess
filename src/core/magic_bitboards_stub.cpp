/**
 * Magic Bitboards Stub Implementation
 * 
 * Temporary stub implementations that just call ray-based functions.
 * These will be replaced with actual magic bitboard implementation in Phase 1.
 */

#include "types.h"
#include "bitboard.h"

namespace seajay {

// Temporary stub implementations - just use ray-based for now
Bitboard magicRookAttacks(Square sq, Bitboard occupied) {
    return rookAttacks(sq, occupied);
}

Bitboard magicBishopAttacks(Square sq, Bitboard occupied) {
    return bishopAttacks(sq, occupied);
}

Bitboard magicQueenAttacks(Square sq, Bitboard occupied) {
    return queenAttacks(sq, occupied);
}

} // namespace seajay