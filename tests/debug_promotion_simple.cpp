// Simple debug tool for Bug #003
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
    std::cout << "========================================\n";
    std::cout << "Bug #003 Simple Debug\n";
    std::cout << "Position: r3k3/P7/8/8/8/8/8/4K3 w - - 0 1\n";
    std::cout << "========================================\n\n";
    
    // Set up the position
    Board board;
    std::string fen = "r3k3/P7/8/8/8/8/8/4K3 w - - 0 1";
    
    if (!board.fromFEN(fen)) {
        std::cerr << "Failed to parse FEN!\n";
        return 1;
    }
    
    // Show the board
    std::cout << board.toString() << "\n";
    
    // Critical squares
    Square a7 = static_cast<Square>(48);  // White pawn location
    Square a8 = static_cast<Square>(56);  // Black rook location
    
    std::cout << "Critical Squares:\n";
    std::cout << "  a7 (index " << a7 << "): piece = " << static_cast<int>(board.pieceAt(a7)) << "\n";
    std::cout << "  a8 (index " << a8 << "): piece = " << static_cast<int>(board.pieceAt(a8)) << "\n\n";
    
    // Show relevant bitboards
    Bitboard occupied = board.occupied();
    Bitboard whitePawns = board.pieces(WHITE, PAWN);
    Bitboard blackPieces = board.pieces(BLACK);
    
    showBitboard(occupied, "All Occupied Squares");
    showBitboard(whitePawns, "White Pawns");
    showBitboard(blackPieces, "Black Pieces");
    
    // Critical check
    std::cout << "\n========================================\n";
    std::cout << "CRITICAL CHECK: Is a8 blocked?\n";
    std::cout << "========================================\n";
    
    Bitboard a8Bit = squareBB(a8);
    bool isA8Occupied = (occupied & a8Bit) != 0;
    std::cout << "squareBB(a8) = 0x" << std::hex << a8Bit << std::dec << "\n";
    std::cout << "occupied & squareBB(a8) = 0x" << std::hex << (occupied & a8Bit) << std::dec << "\n";
    std::cout << "Result: a8 is " << (isA8Occupied ? "OCCUPIED" : "EMPTY") << "\n";
    
    // Actually generate moves
    std::cout << "\n========================================\n";
    std::cout << "ACTUAL MOVE GENERATION\n";
    std::cout << "========================================\n";
    
    MoveList moves;
    MoveGenerator::generateLegalMoves(board, moves);
    
    std::cout << "Total moves generated: " << moves.size() << "\n";
    
    int promotionCount = 0;
    for (size_t i = 0; i < moves.size(); i++) {
        Move move = moves[i];
        if (isPromotion(move)) {
            promotionCount++;
            std::cout << "  Promotion move: " 
                      << squareToString(moveFrom(move)) 
                      << squareToString(moveTo(move)) << "\n";
        }
    }
    
    std::cout << "\nPromotion moves found: " << promotionCount << "\n";
    
    if (promotionCount > 0) {
        std::cout << "\n✗ BUG CONFIRMED: Generated illegal promotion moves!\n";
    } else {
        std::cout << "\n✓ GOOD: No illegal promotion moves generated.\n";
    }
    
    return promotionCount > 0 ? 1 : 0;
}