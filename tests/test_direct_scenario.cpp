#include <iostream>
#include <string>
#include "../src/core/board.h"
#include "../src/core/move_generation.h"

using namespace seajay;

int main() {
    // Test position: Black king in check from rook
    std::string fen = "4k3/8/8/8/8/8/8/K3R3 b - - 0 1";
    Board board;
    board.fromFEN(fen);
    
    std::cout << "Original position:\n" << board.toString() << "\n";
    
    // Make the problematic move e8->d8
    Move e8d8 = makeMove(E8, D8, NORMAL);
    Board::UndoInfo undo;
    board.makeMove(e8d8, undo);
    
    std::cout << "After e8->d8:\n" << board.toString() << "\n";
    
    // Now check if d8 is attacked by white
    bool d8Attacked = MoveGenerator::isSquareAttacked(board, D8, WHITE);
    std::cout << "Is d8 attacked by WHITE? " << (d8Attacked ? "YES" : "NO") << "\n";
    
    // Get the occupied squares and print the rook attacks
    Bitboard occupied = board.occupied();
    Bitboard rookAttacks = seajay::rookAttacks(E1, occupied);
    
    std::cout << "\nRook attacks from e1 with current occupancy:\n";
    for (int rank = 7; rank >= 0; --rank) {
        for (int file = 0; file < 8; ++file) {
            Square sq = makeSquare(File(file), Rank(rank));
            if (sq == E1) {
                std::cout << "R ";
            } else if (sq == D8) {
                std::cout << "k ";
            } else if (sq == A1) {
                std::cout << "K ";
            } else if (rookAttacks & squareBB(sq)) {
                std::cout << "X ";
            } else {
                std::cout << ". ";
            }
        }
        std::cout << "\n";
    }
    
    std::cout << "\nRook can attack squares: ";
    while (rookAttacks) {
        Square sq = popLsb(rookAttacks);
        std::cout << squareToString(sq) << " ";
    }
    std::cout << "\n";
    
    // The actual problem is that d8 should be in the rook's attack set but isn't
    std::cout << "\nExpected: Rook should be able to attack d8 (through empty squares)\n";
    std::cout << "Actual: Rook " << (d8Attacked ? "CAN" : "CANNOT") << " attack d8\n";
    
    return 0;
}