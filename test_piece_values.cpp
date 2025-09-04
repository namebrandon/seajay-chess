#include <iostream>
#include "src/evaluation/material.h"
#include "src/core/types.h"

int main() {
    using namespace seajay;
    using namespace seajay::eval;
    
    std::cout << "Initial PIECE_VALUES:" << std::endl;
    std::cout << "  Pawn: " << PIECE_VALUES[PAWN].value() << std::endl;
    std::cout << "  Knight: " << PIECE_VALUES[KNIGHT].value() << std::endl;
    std::cout << "  Bishop: " << PIECE_VALUES[BISHOP].value() << std::endl;
    std::cout << "  Rook: " << PIECE_VALUES[ROOK].value() << std::endl;
    std::cout << "  Queen: " << PIECE_VALUES[QUEEN].value() << std::endl;
    
    std::cout << "\nSetting Pawn value to 200..." << std::endl;
    setPieceValue(PAWN, 200);
    
    std::cout << "\nAfter setPieceValue:" << std::endl;
    std::cout << "  Pawn: " << PIECE_VALUES[PAWN].value() << std::endl;
    std::cout << "  Knight: " << PIECE_VALUES[KNIGHT].value() << std::endl;
    
    std::cout << "\nSetting all values to double..." << std::endl;
    setPieceValue(PAWN, 200);
    setPieceValue(KNIGHT, 640);
    setPieceValue(BISHOP, 660);
    setPieceValue(ROOK, 1020);
    setPieceValue(QUEEN, 1900);
    
    std::cout << "\nAfter doubling all values:" << std::endl;
    std::cout << "  Pawn: " << PIECE_VALUES[PAWN].value() << std::endl;
    std::cout << "  Knight: " << PIECE_VALUES[KNIGHT].value() << std::endl;
    std::cout << "  Bishop: " << PIECE_VALUES[BISHOP].value() << std::endl;
    std::cout << "  Rook: " << PIECE_VALUES[ROOK].value() << std::endl;
    std::cout << "  Queen: " << PIECE_VALUES[QUEEN].value() << std::endl;
    
    return 0;
}
