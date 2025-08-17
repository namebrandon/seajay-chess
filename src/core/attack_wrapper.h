#pragma once

/**
 * Attack Generation Wrapper
 * 
 * This file provides wrapper functions that can switch between ray-based
 * and magic bitboard implementations based on runtime configuration.
 * This allows for A/B testing and UCI-controlled feature toggling.
 */

#include "types.h"
#include "bitboard.h"
#include "magic_bitboards.h"      // Magic bitboards implementation
#include "engine_config.h"       // Runtime configuration
#include <cassert>

namespace seajay {

// Magic bitboard functions are now available from magic_bitboards_v2.h

// Ray-based attack functions (current implementation)
inline Bitboard rayRookAttacks(Square sq, Bitboard occupied) {
    return ::seajay::rookAttacks(sq, occupied);
}

inline Bitboard rayBishopAttacks(Square sq, Bitboard occupied) {
    return ::seajay::bishopAttacks(sq, occupied);
}

inline Bitboard rayQueenAttacks(Square sq, Bitboard occupied) {
    return ::seajay::queenAttacks(sq, occupied);
}

// Wrapper functions that switch between implementations based on runtime config
    inline Bitboard getRookAttacks(Square sq, Bitboard occupied) {
        if (getConfig().useMagicBitboards) {
            #ifdef DEBUG_MAGIC
                // In debug mode, validate magic against ray-based
                Bitboard magic = magicRookAttacks(sq, occupied);
                Bitboard ray = rayRookAttacks(sq, occupied);
                assert(magic == ray && "Rook attack mismatch between magic and ray-based");
                return magic;
            #else
                return magicRookAttacks(sq, occupied);
            #endif
        } else {
            return rayRookAttacks(sq, occupied);
        }
    }
    
    inline Bitboard getBishopAttacks(Square sq, Bitboard occupied) {
        if (getConfig().useMagicBitboards) {
            #ifdef DEBUG_MAGIC
                // In debug mode, validate magic against ray-based
                Bitboard magic = magicBishopAttacks(sq, occupied);
                Bitboard ray = rayBishopAttacks(sq, occupied);
                assert(magic == ray && "Bishop attack mismatch between magic and ray-based");
                return magic;
            #else
                return magicBishopAttacks(sq, occupied);
            #endif
        } else {
            return rayBishopAttacks(sq, occupied);
        }
    }
    
    inline Bitboard getQueenAttacks(Square sq, Bitboard occupied) {
        if (getConfig().useMagicBitboards) {
            #ifdef DEBUG_MAGIC
                // In debug mode, validate magic against ray-based
                Bitboard magic = magicQueenAttacks(sq, occupied);
                Bitboard ray = rayQueenAttacks(sq, occupied);
                assert(magic == ray && "Queen attack mismatch between magic and ray-based");
                return magic;
            #else
                return magicQueenAttacks(sq, occupied);
            #endif
        } else {
            return rayQueenAttacks(sq, occupied);
        }
    }


} // namespace seajay