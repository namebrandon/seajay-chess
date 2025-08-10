// Debug test case #9 specifically
#include <iostream>
#include <iomanip>
#include "../src/core/board.h"
#include "../src/core/move_generation.h"
#include "../src/core/move_list.h"
#include "../src/core/bitboard.h"
#include "../src/core/types.h"

using namespace seajay;

void showBitboard(Bitboard bb, const std::string& name) {
    std::cout << "\n" << name << ":\n";
    std::cout << "  Hex: 0x" << std::hex << bb << std::dec << "\n";
    
    for (int rank = 7; rank >= 0; rank--) {
        std::cout << "  " << (rank + 1) << " ";
        for (int file = 0; file < 8; file++) {
            Square sq = static_cast<Square>(rank * 8 + file);
            std::cout << ((bb & squareBB(sq)) ? "1" : ".");
            if (file < 7) std::cout << " ";
        }
        std::cout << "\n";
    }
    std::cout << "    a b c d e f g h\n";
}

int main() {
    std::string fen = "rn2k3/P7/8/8/8/8/8/4K3 w - - 0 1";
    
    std::cout << "========================================\n";
    std::cout << "Debug Test Case #9\n";
    std::cout << "Position: " << fen << "\n";
    std::cout << "========================================\n\n";
    
    Board board;
    board.fromFEN(fen);
    
    std::cout << board.toString() << "\n";
    
    Square a7 = static_cast<Square>(48);
    Square a8 = static_cast<Square>(56);
    Square b8 = static_cast<Square>(57);
    
    std::cout << "Critical squares:\n";
    std::cout << "  a7: piece=" << static_cast<int>(board.pieceAt(a7)) << " (WHITE_PAWN)\n";
    std::cout << "  a8: piece=" << static_cast<int>(board.pieceAt(a8)) << " (BLACK_ROOK)\n";
    std::cout << "  b8: piece=" << static_cast<int>(board.pieceAt(b8)) << " (BLACK_KNIGHT)\n\n";
    
    // Show attack pattern for pawn on a7
    std::cout << "Pawn on a7 attack analysis:\n";
    std::cout << "  File of a7: " << fileOf(a7) << " (0=a-file)\n";
    std::cout << "  Rank of a7: " << rankOf(a7) << " (6=7th rank, 0-indexed)\n\n";
    
    // Manually check what getPawnAttacks should return
    std::cout << "Manual attack calculation for white pawn on a7:\n";
    std::cout << "  file > 0? " << (fileOf(a7) > 0 ? "YES" : "NO") << " -> Can attack North-West\n";
    std::cout << "  file < 7? " << (fileOf(a7) < 7 ? "YES" : "NO") << " -> Can attack North-East\n";
    
    if (fileOf(a7) == 0) {
        std::cout << "  Since file=0 (a-file), pawn CANNOT attack North-West (would be off-board)\n";
        std::cout << "  Pawn CAN attack North-East to b8 (a7+9 = 48+9 = 57)\n";
    }
    
    // Get actual pawn attacks
    Bitboard pawnAttacks = MoveGenerator::getPawnAttacks(a7, WHITE);
    showBitboard(pawnAttacks, "Actual pawn attacks from a7");
    
    // Check if attacks include a8 and b8
    std::cout << "\nAttack coverage:\n";
    std::cout << "  Can attack a8? " << ((pawnAttacks & squareBB(a8)) ? "YES" : "NO") << "\n";
    std::cout << "  Can attack b8? " << ((pawnAttacks & squareBB(b8)) ? "YES" : "NO") << "\n";
    
    // Check enemy pieces
    Bitboard blackPieces = board.pieces(BLACK);
    showBitboard(blackPieces, "Black pieces");
    
    Bitboard validCaptures = pawnAttacks & blackPieces;
    showBitboard(validCaptures, "Valid capture squares (attacks & black pieces)");
    
    std::cout << "\n========================================\n";
    std::cout << "CONCLUSION:\n";
    std::cout << "The pawn on a7 (a-file) can ONLY attack diagonally.\n";
    std::cout << "Since it's on the a-file, it can only attack North-East (b8).\n";
    std::cout << "It CANNOT attack a8 because that's straight ahead, not diagonal!\n";
    std::cout << "The test expectation is WRONG. Pawn cannot capture straight ahead.\n";
    std::cout << "========================================\n";
    
    return 0;
}