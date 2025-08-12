#include "../src/core/board.h"
#include "../src/core/bitboard.h"
#include "../src/core/magic_bitboards.h"
#include <iostream>
#include <iomanip>

using namespace seajay;

void printBitboard(const std::string& name, Bitboard bb) {
    std::cout << name << ":\n";
    for (int rank = 7; rank >= 0; rank--) {
        std::cout << (rank + 1) << " ";
        for (int file = 0; file < 8; file++) {
            Square sq = makeSquare(static_cast<File>(file), static_cast<Rank>(rank));
            if (bb & squareBB(sq)) {
                std::cout << "X ";
            } else {
                std::cout << ". ";
            }
        }
        std::cout << "\n";
    }
    std::cout << "  a b c d e f g h\n";
    std::cout << "Hex: 0x" << std::hex << bb << std::dec << "\n\n";
}

int main() {
    magic::initMagics();
    
    // Test 1: Starting position, rook on A1
    {
        Board board;
        board.fromFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
        Bitboard occupied = board.occupied();
        
        std::cout << "Starting position - Rook A1:\n";
        printBitboard("Occupied", occupied);
        
        Bitboard rookA1 = magicRookAttacks(A1, occupied);
        printBitboard("Rook A1 attacks", rookA1);
        
        Bitboard bishopC1 = magicBishopAttacks(C1, occupied);
        printBitboard("Bishop C1 attacks", bishopC1);
    }
    
    // Test 2: Bishop corner
    {
        Board board;
        board.fromFEN("B7/8/8/8/3p4/8/8/7b w - - 0 1");
        
        std::cout << "Bishop A8 position:\n";
        Bitboard bishopA8 = magicBishopAttacks(A8, board.occupied());
        printBitboard("Bishop A8 attacks", bishopA8);
    }
    
    // Test 3: Rook chain
    {
        Board board;
        board.fromFEN("8/8/8/8/R2r4/8/8/8 w - - 0 1");
        
        std::cout << "Rook chain - Rook A4:\n";
        Bitboard rookA4 = magicRookAttacks(A4, board.occupied());
        printBitboard("Rook A4 attacks", rookA4);
    }
    
    return 0;
}