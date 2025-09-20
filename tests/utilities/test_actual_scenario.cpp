#include <iostream>
#include <string>
#include "../src/core/board.h"
#include "../src/core/bitboard.h"
#include "../src/core/move_generation.h"

using namespace seajay;

void printBitboard(Bitboard bb, const std::string& label) {
    std::cout << label << ":\n";
    for (int rank = 7; rank >= 0; --rank) {
        for (int file = 0; file < 8; ++file) {
            Square sq = makeSquare(File(file), Rank(rank));
            if (bb & squareBB(sq)) {
                std::cout << "X ";
            } else {
                std::cout << ". ";
            }
        }
        std::cout << "\n";
    }
    std::cout << "\n";
}

int main() {
    // Set up the actual board position after e8->d8
    std::string fen = "3k4/8/8/8/8/8/8/3KR3 w - - 1 2";
    Board board;
    board.fromFEN(fen);
    
    std::cout << "Board position (after e8->d8):\n";
    std::cout << board.toString() << "\n";
    
    // Get the actual occupied squares
    Bitboard occupied = board.occupied();
    printBitboard(occupied, "Occupied squares");
    
    // Generate rook attacks from e1 with this occupancy
    Square rookSquare = E1;
    Bitboard rookAttacks = seajay::rookAttacks(rookSquare, occupied);
    printBitboard(rookAttacks, "Rook attacks from e1");
    
    // Check if d8 is attacked
    Square kingSquare = D8;
    std::cout << "Does rook attack d8? " << ((rookAttacks & squareBB(kingSquare)) ? "YES" : "NO") << "\n";
    
    // Now test with getRookAttacks from MoveGenerator (which goes through wrapper)
    Bitboard mgRookAttacks = MoveGenerator::getRookAttacks(rookSquare, occupied);
    printBitboard(mgRookAttacks, "MoveGenerator::getRookAttacks from e1");
    std::cout << "MoveGenerator: Does rook attack d8? " << ((mgRookAttacks & squareBB(kingSquare)) ? "YES" : "NO") << "\n";
    
    // Check if square is attacked using MoveGenerator
    bool isAttacked = MoveGenerator::isSquareAttacked(board, kingSquare, WHITE);
    std::cout << "MoveGenerator::isSquareAttacked(d8, WHITE): " << (isAttacked ? "YES" : "NO") << "\n";
    
    return 0;
}