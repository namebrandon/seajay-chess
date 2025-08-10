// Direct debugging tool for Bug #003
// This program shows EXACTLY what's happening with the blocked pawn position

#include <iostream>
#include <iomanip>
#include <bitset>
#include "../src/core/board.h"
#include "../src/core/move_generation.h"
#include "../src/core/move_list.h"
#include "../src/core/bitboard.h"

using namespace seajay;

void showBitboard(Bitboard bb, const std::string& name) {
    std::cout << "\n" << name << ":\n";
    std::cout << "  Hex: 0x" << std::hex << std::setfill('0') << std::setw(16) << bb << std::dec << "\n";
    std::cout << "  Binary:\n";
    
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
    std::cout << "Bug #003 Detailed Debugging\n";
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
    std::cout << "Board visualization:\n";
    std::cout << board.toString() << "\n";
    
    // Critical squares
    Square a7 = static_cast<Square>(48);  // White pawn location
    Square a8 = static_cast<Square>(56);  // Black rook location
    
    std::cout << "Critical Squares:\n";
    std::cout << "  a7 (index " << a7 << "): " << pieceToChar(board.pieceAt(a7)) 
              << " (should be 'P')\n";
    std::cout << "  a8 (index " << a8 << "): " << pieceToChar(board.pieceAt(a8)) 
              << " (should be 'r')\n\n";
    
    // Show relevant bitboards
    Bitboard occupied = board.occupied();
    Bitboard whitePawns = board.pieces(WHITE, PAWN);
    Bitboard blackPieces = board.pieces(BLACK);
    Bitboard a8Bit = squareBB(a8);
    
    showBitboard(occupied, "All Occupied Squares");
    showBitboard(whitePawns, "White Pawns");
    showBitboard(blackPieces, "Black Pieces");
    showBitboard(a8Bit, "Square a8 Bit");
    
    // Critical check
    std::cout << "\n========================================\n";
    std::cout << "CRITICAL CHECK: Is a8 blocked?\n";
    std::cout << "========================================\n";
    
    bool isA8Occupied = (occupied & a8Bit) != 0;
    std::cout << "occupied & squareBB(a8) = 0x" << std::hex << (occupied & a8Bit) << std::dec << "\n";
    std::cout << "Result: a8 is " << (isA8Occupied ? "OCCUPIED" : "EMPTY") << "\n";
    
    if (isA8Occupied) {
        std::cout << "✓ Correct: a8 is occupied by black rook\n";
    } else {
        std::cout << "✗ BUG: a8 appears empty but should have black rook!\n";
    }
    
    // Now simulate what the move generator does
    std::cout << "\n========================================\n";
    std::cout << "SIMULATING MOVE GENERATION\n";
    std::cout << "========================================\n";
    
    Color us = WHITE;
    int forward = 8;  // White moves up
    Square from = a7;
    int toInt = static_cast<int>(from) + forward;
    
    std::cout << "Pawn at square " << from << " (a7)\n";
    std::cout << "Trying to move to square " << toInt << " (";
    
    if (toInt >= 0 && toInt < 64) {
        Square to = static_cast<Square>(toInt);
        std::cout << squareToString(to) << ")\n";
        
        bool blocked = (occupied & squareBB(to)) != 0;
        std::cout << "Is destination blocked? " << (blocked ? "YES" : "NO") << "\n";
        
        if (!blocked) {
            std::cout << "\n✗ BUG CONFIRMED: Move generator would add promotion moves!\n";
            std::cout << "The pawn on a7 thinks a8 is empty and would generate:\n";
            std::cout << "  - a7-a8=Q\n";
            std::cout << "  - a7-a8=R\n";
            std::cout << "  - a7-a8=B\n";
            std::cout << "  - a7-a8=N\n";
        } else {
            std::cout << "\n✓ GOOD: Move generator correctly sees a8 is blocked\n";
            std::cout << "No promotion moves would be generated.\n";
        }
    } else {
        std::cout << "out of bounds)\n";
    }
    
    // Actually generate moves and check
    std::cout << "\n========================================\n";
    std::cout << "ACTUAL MOVE GENERATION\n";
    std::cout << "========================================\n";
    
    MoveList moves;
    MoveGenerator::generateAllMoves(board, moves);
    
    std::cout << "Total moves generated: " << moves.size() << "\n";
    std::cout << "Expected: 5 (king moves only)\n\n";
    
    int promotionCount = 0;
    std::cout << "Generated moves:\n";
    for (size_t i = 0; i < moves.size(); i++) {
        Move move = moves[i];
        Square moveFromSq = moveFrom(move);
        Square moveToSq = moveTo(move);
        
        std::cout << "  " << (i+1) << ". " << squareToString(moveFromSq) 
                  << "-" << squareToString(moveToSq);
        
        if (moveFlags(move) & PROMOTION) {
            promotionCount++;
            PieceType promo = promotionType(move);
            std::cout << "=";
            switch (promo) {
                case QUEEN:  std::cout << "Q"; break;
                case ROOK:   std::cout << "R"; break;
                case BISHOP: std::cout << "B"; break;
                case KNIGHT: std::cout << "N"; break;
                default: std::cout << "?"; break;
            }
            std::cout << " [ILLEGAL PROMOTION!]";
        }
        std::cout << "\n";
    }
    
    std::cout << "\n========================================\n";
    std::cout << "DIAGNOSIS\n";
    std::cout << "========================================\n";
    
    if (promotionCount > 0) {
        std::cout << "✗ BUG CONFIRMED: Generated " << promotionCount 
                  << " illegal promotion moves!\n";
        std::cout << "\nThe bug is in move_generation.cpp around line 234.\n";
        std::cout << "The occupancy check is not working correctly.\n";
        std::cout << "\nPossible causes:\n";
        std::cout << "1. The occupied() bitboard is not properly maintained\n";
        std::cout << "2. The squareBB() function returns wrong bit for a8\n";
        std::cout << "3. The bitwise AND operation is incorrect\n";
    } else if (moves.size() != 5) {
        std::cout << "✗ Wrong move count: expected 5, got " << moves.size() << "\n";
    } else {
        std::cout << "✓ No bug detected - correct move generation!\n";
    }
    
    return 0;
}