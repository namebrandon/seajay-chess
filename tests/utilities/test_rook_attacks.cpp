#include <iostream>
#include <iomanip>
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
    // Test rook attacks from e1
    Square rookSquare = E1;
    
    // Test 1: Empty board
    Bitboard empty = 0;
    Bitboard attacks1 = seajay::rookAttacks(rookSquare, empty);
    printBitboard(attacks1, "Rook on e1, empty board");
    
    // Test 2: King on d8
    Bitboard withKingD8 = squareBB(D8);
    Bitboard attacks2 = seajay::rookAttacks(rookSquare, withKingD8);
    printBitboard(withKingD8, "Occupied squares (king on d8)");
    printBitboard(attacks2, "Rook on e1, king on d8");
    
    // Check if d8 is in the attack set
    std::cout << "Does rook attack d8? " << ((attacks2 & squareBB(D8)) ? "YES" : "NO") << "\n\n";
    
    // Test 3: King on e8  
    Bitboard withKingE8 = squareBB(E8);
    Bitboard attacks3 = seajay::rookAttacks(rookSquare, withKingE8);
    printBitboard(withKingE8, "Occupied squares (king on e8)");
    printBitboard(attacks3, "Rook on e1, king on e8");
    
    // Check if e8 is in the attack set
    std::cout << "Does rook attack e8? " << ((attacks3 & squareBB(E8)) ? "YES" : "NO") << "\n";
    
    return 0;
}