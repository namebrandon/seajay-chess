#pragma once

#include "types.h"
#include <bit>

namespace seajay {

inline int popCount(Bitboard bb) {
    return std::popcount(bb);
}

inline Square lsb(Bitboard bb) {
    return static_cast<Square>(std::countr_zero(bb));
}

inline Square msb(Bitboard bb) {
    return static_cast<Square>(63 - std::countl_zero(bb));
}

inline Square popLsb(Bitboard& bb) {
    Square s = lsb(bb);
    bb &= bb - 1;
    return s;
}

inline Bitboard shift(Bitboard bb, Direction d) {
    switch (d) {
        case NORTH:      return bb << 8;
        case SOUTH:      return bb >> 8;
        case EAST:       return (bb & ~fileBB(7)) << 1;
        case WEST:       return (bb & ~fileBB(0)) >> 1;
        case NORTH_EAST: return (bb & ~fileBB(7)) << 9;
        case NORTH_WEST: return (bb & ~fileBB(0)) << 7;
        case SOUTH_EAST: return (bb & ~fileBB(7)) >> 7;
        case SOUTH_WEST: return (bb & ~fileBB(0)) >> 9;
        default:         return 0;
    }
}

template<Direction D>
inline Bitboard shift(Bitboard bb) {
    if constexpr (D == NORTH)      return bb << 8;
    if constexpr (D == SOUTH)      return bb >> 8;
    if constexpr (D == EAST)       return (bb & ~fileBB(7)) << 1;
    if constexpr (D == WEST)       return (bb & ~fileBB(0)) >> 1;
    if constexpr (D == NORTH_EAST) return (bb & ~fileBB(7)) << 9;
    if constexpr (D == NORTH_WEST) return (bb & ~fileBB(0)) << 7;
    if constexpr (D == SOUTH_EAST) return (bb & ~fileBB(7)) >> 7;
    if constexpr (D == SOUTH_WEST) return (bb & ~fileBB(0)) >> 9;
    return 0;
}

inline bool moreThanOne(Bitboard bb) {
    return bb & (bb - 1);
}

inline void setBit(Bitboard& bb, Square s) {
    bb |= squareBB(s);
}

inline void clearBit(Bitboard& bb, Square s) {
    bb &= ~squareBB(s);
}

inline bool testBit(Bitboard bb, Square s) {
    return bb & squareBB(s);
}

inline Bitboard between(Square s1, Square s2) {
    const Bitboard m1 = Bitboard(-1);
    const Bitboard a2a7 = 0x0001010101010100ULL;
    const Bitboard b2g7 = 0x0040201008040200ULL;
    const Bitboard h1b7 = 0x0002040810204080ULL;
    
    Bitboard btwn, line, r, f;
    
    btwn = (m1 << s1) ^ (m1 << s2);
    f = fileOf(s2) - fileOf(s1);
    r = rankOf(s2) - rankOf(s1);
    
    if (!f) {
        line = fileBB(fileOf(s1));
    } else if (!r) {
        line = rankBB(rankOf(s1));
    } else if (f == r) {
        line = (((s1 % 9) < 8) ? a2a7 : b2g7) << (s1 / 8 * 8 + s1 % 8);
    } else {
        line = (((s1 % 7) > 0) ? h1b7 : b2g7) << (s1 / 8 * 8 + 7 - s1 % 8);
    }
    
    return line & btwn;
}

inline Bitboard ray(Square s, Direction d) {
    Bitboard bb = 0;
    Bitboard sq = squareBB(s);
    
    // Generate ray in the given direction, stopping at board edges
    for (int i = 0; i < 8; ++i) {  // Maximum 7 steps in any direction
        Bitboard nextSq = shift(sq, d);
        if (!nextSq) break;  // Hit the edge of the board
        sq = nextSq;
        bb |= sq;
    }
    
    return bb;
}

constexpr Bitboard RANK_1_BB = rankBB(0);
constexpr Bitboard RANK_2_BB = rankBB(1);
constexpr Bitboard RANK_3_BB = rankBB(2);
constexpr Bitboard RANK_4_BB = rankBB(3);
constexpr Bitboard RANK_5_BB = rankBB(4);
constexpr Bitboard RANK_6_BB = rankBB(5);
constexpr Bitboard RANK_7_BB = rankBB(6);
constexpr Bitboard RANK_8_BB = rankBB(7);

constexpr Bitboard FILE_A_BB = fileBB(0);
constexpr Bitboard FILE_B_BB = fileBB(1);
constexpr Bitboard FILE_C_BB = fileBB(2);
constexpr Bitboard FILE_D_BB = fileBB(3);
constexpr Bitboard FILE_E_BB = fileBB(4);
constexpr Bitboard FILE_F_BB = fileBB(5);
constexpr Bitboard FILE_G_BB = fileBB(6);
constexpr Bitboard FILE_H_BB = fileBB(7);

constexpr Bitboard LIGHT_SQUARES_BB = 0x55AA55AA55AA55AAULL;
constexpr Bitboard DARK_SQUARES_BB = 0xAA55AA55AA55AA55ULL;

constexpr Bitboard CENTER_BB = 0x0000001818000000ULL;
constexpr Bitboard BIG_CENTER_BB = 0x00003C3C3C3C0000ULL;

// Ray-based sliding piece attack generation (Phase 1 temporary implementation)
// These will be replaced with magic bitboards in Phase 3

inline Bitboard rookAttacks(Square sq, Bitboard occupied) {
    Bitboard attacks = 0;
    
    // Generate attacks in all four rook directions
    Direction directions[] = {NORTH, SOUTH, EAST, WEST};
    
    for (Direction d : directions) {
        Bitboard ray = ::seajay::ray(sq, d);
        
        if (ray & occupied) {
            // Find first blocker in this direction
            Bitboard blockers = ray & occupied;
            Square blocker;
            
            if (d == NORTH || d == EAST) {
                blocker = lsb(blockers);  // First blocker in positive direction
            } else {
                blocker = msb(blockers);  // First blocker in negative direction
            }
            
            // Include squares up to (but not past) the blocker
            Bitboard blockerRay = ::seajay::ray(blocker, d);
            ray &= ~blockerRay;
        }
        
        attacks |= ray;
    }
    
    return attacks;
}

inline Bitboard bishopAttacks(Square sq, Bitboard occupied) {
    Bitboard attacks = 0;
    
    // Generate attacks in all four bishop directions
    Direction directions[] = {NORTH_EAST, NORTH_WEST, SOUTH_EAST, SOUTH_WEST};
    
    for (Direction d : directions) {
        Bitboard ray = ::seajay::ray(sq, d);
        
        if (ray & occupied) {
            // Find first blocker in this direction
            Bitboard blockers = ray & occupied;
            Square blocker;
            
            if (d == NORTH_EAST || d == NORTH_WEST) {
                blocker = lsb(blockers);  // First blocker in positive direction
            } else {
                blocker = msb(blockers);  // First blocker in negative direction
            }
            
            // Include squares up to (but not past) the blocker
            Bitboard blockerRay = ::seajay::ray(blocker, d);
            ray &= ~blockerRay;
        }
        
        attacks |= ray;
    }
    
    return attacks;
}

inline Bitboard queenAttacks(Square sq, Bitboard occupied) {
    // Queen attacks are combination of rook and bishop attacks
    return rookAttacks(sq, occupied) | bishopAttacks(sq, occupied);
}

inline std::string bitboardToString(Bitboard bb) {
    std::string result = "\n  +---+---+---+---+---+---+---+---+\n";
    
    for (int r = 7; r >= 0; --r) {
        result += std::to_string(r + 1) + " |";
        for (File f = 0; f < 8; ++f) {
            Square s = makeSquare(f, r);
            result += testBit(bb, s) ? " X |" : "   |";
        }
        result += "\n  +---+---+---+---+---+---+---+---+\n";
    }
    result += "    a   b   c   d   e   f   g   h\n";
    
    return result;
}

} // namespace seajay