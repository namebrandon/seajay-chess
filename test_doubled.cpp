#include <iostream>
#include <bitset>
#include <bit>
#include <cstdint>

using Bitboard = uint64_t;
using Square = int;

enum Color { WHITE, BLACK };

// File masks
constexpr Bitboard FILE_A_BB = 0x0101010101010101ULL;
constexpr Bitboard FILE_B_BB = 0x0202020202020202ULL;
constexpr Bitboard FILE_C_BB = 0x0404040404040404ULL;
constexpr Bitboard FILE_D_BB = 0x0808080808080808ULL;
constexpr Bitboard FILE_E_BB = 0x1010101010101010ULL;
constexpr Bitboard FILE_F_BB = 0x2020202020202020ULL;
constexpr Bitboard FILE_G_BB = 0x4040404040404040ULL;
constexpr Bitboard FILE_H_BB = 0x8080808080808080ULL;

inline Square lsb(Bitboard bb) {
    return static_cast<Square>(std::countr_zero(bb));
}

inline Square msb(Bitboard bb) {
    return static_cast<Square>(63 - std::countl_zero(bb));
}

Bitboard getDoubledPawns(Color c, Bitboard ourPawns) {
    Bitboard doubled = 0ULL;
    
    // Process each file
    for (int file = 0; file < 8; file++) {
        Bitboard fileMask = FILE_A_BB << file;
        Bitboard pawnsOnFile = ourPawns & fileMask;
        
        int pawnCount = std::popcount(pawnsOnFile);
        
        // If more than one pawn on this file, mark all but the base pawn as doubled
        if (pawnCount > 1) {
            // Mark all pawns on this file as doubled except the rearmost one
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

void printBitboard(const std::string& name, Bitboard bb) {
    std::cout << name << ":\n";
    for (int rank = 7; rank >= 0; rank--) {
        std::cout << (rank + 1) << " ";
        for (int file = 0; file < 8; file++) {
            Square sq = rank * 8 + file;
            if (bb & (1ULL << sq)) {
                std::cout << "X ";
            } else {
                std::cout << ". ";
            }
        }
        std::cout << "\n";
    }
    std::cout << "  a b c d e f g h\n\n";
}

int main() {
    // Test 1: White pawns with doubled pawns on e-file (e2, e3)
    std::cout << "Test 1: White doubled pawns on e-file\n";
    Bitboard whitePawns = (1ULL << 12) | (1ULL << 20);  // e2, e3
    printBitboard("White pawns", whitePawns);
    Bitboard doubled = getDoubledPawns(WHITE, whitePawns);
    printBitboard("Doubled pawns", doubled);
    std::cout << "Expected: e3 marked as doubled (e2 is base)\n";
    std::cout << "Count: " << std::popcount(doubled) << " (expected: 1)\n\n";
    
    // Test 2: Black pawns with doubled pawns on d-file (d7, d6)
    std::cout << "Test 2: Black doubled pawns on d-file\n";
    Bitboard blackPawns = (1ULL << 51) | (1ULL << 43);  // d7, d6
    printBitboard("Black pawns", blackPawns);
    doubled = getDoubledPawns(BLACK, blackPawns);
    printBitboard("Doubled pawns", doubled);
    std::cout << "Expected: d6 marked as doubled (d7 is base)\n";
    std::cout << "Count: " << std::popcount(doubled) << " (expected: 1)\n\n";
    
    // Test 3: Triple pawns
    std::cout << "Test 3: Triple white pawns on c-file\n";
    whitePawns = (1ULL << 10) | (1ULL << 18) | (1ULL << 26);  // c2, c3, c4
    printBitboard("White pawns", whitePawns);
    doubled = getDoubledPawns(WHITE, whitePawns);
    printBitboard("Doubled pawns", doubled);
    std::cout << "Expected: c3 and c4 marked as doubled (c2 is base)\n";
    std::cout << "Count: " << std::popcount(doubled) << " (expected: 2)\n\n";
    
    // Test 4: Starting position (no doubled pawns)
    std::cout << "Test 4: Starting position\n";
    whitePawns = 0x000000000000FF00ULL;  // Rank 2
    printBitboard("White pawns", whitePawns);
    doubled = getDoubledPawns(WHITE, whitePawns);
    printBitboard("Doubled pawns", doubled);
    std::cout << "Count: " << std::popcount(doubled) << " (expected: 0)\n\n";
    
    return 0;
}