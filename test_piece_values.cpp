#include <iostream>
#include "src/evaluation/material.h"

using namespace seajay;
using namespace seajay::eval;

int main() {
    std::cout << "Piece Values in SeaJay engine:\n";
    std::cout << "Pawn:   " << PIECE_VALUES[PAWN].value() << " cp\n";
    std::cout << "Knight: " << PIECE_VALUES[KNIGHT].value() << " cp\n";
    std::cout << "Bishop: " << PIECE_VALUES[BISHOP].value() << " cp\n";
    std::cout << "Rook:   " << PIECE_VALUES[ROOK].value() << " cp\n";
    std::cout << "Queen:  " << PIECE_VALUES[QUEEN].value() << " cp\n";
    std::cout << "King:   " << PIECE_VALUES[KING].value() << " cp\n";
    
    std::cout << "\nMaterial calculation for test position:\n";
    std::cout << "White: 5P + 1B + 1R = ";
    std::cout << (5 * PIECE_VALUES[PAWN].value() + PIECE_VALUES[BISHOP].value() + PIECE_VALUES[ROOK].value()) << " cp\n";
    
    std::cout << "Black: 7P + 1N + 1B + 2R + 1Q = ";
    std::cout << (7 * PIECE_VALUES[PAWN].value() + PIECE_VALUES[KNIGHT].value() + 
                  PIECE_VALUES[BISHOP].value() + 2 * PIECE_VALUES[ROOK].value() + 
                  PIECE_VALUES[QUEEN].value()) << " cp\n";
    
    std::cout << "\nDifference (Black - White): ";
    int white_total = 5 * PIECE_VALUES[PAWN].value() + PIECE_VALUES[BISHOP].value() + PIECE_VALUES[ROOK].value();
    int black_total = 7 * PIECE_VALUES[PAWN].value() + PIECE_VALUES[KNIGHT].value() + 
                      PIECE_VALUES[BISHOP].value() + 2 * PIECE_VALUES[ROOK].value() + 
                      PIECE_VALUES[QUEEN].value();
    std::cout << (black_total - white_total) << " cp\n";
    std::cout << "In pawns: " << ((black_total - white_total) / 100.0) << " pawns\n";
    
    // Now let's check what would be reasonable
    std::cout << "\n--- Expected values check ---\n";
    std::cout << "If Black just captured a rook (510cp), expected material difference:\n";
    std::cout << "Should be approximately +510cp (5.1 pawns) for Black\n";
    std::cout << "But we're seeing: " << (black_total - white_total) << " cp (" 
              << ((black_total - white_total) / 100.0) << " pawns)\n";
    
    // Check the specific difference
    std::cout << "\nBreaking down the difference:\n";
    std::cout << "Black extra pawns: 2 * 100 = 200 cp\n";
    std::cout << "Black extra knight: 320 cp\n";
    std::cout << "Black extra rook: 510 cp\n";
    std::cout << "Black queen: 950 cp\n";
    std::cout << "Total Black advantage: 200 + 320 + 510 + 950 = 1980 cp\n";
    
    return 0;
}