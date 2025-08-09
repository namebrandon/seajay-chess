#include "src/core/board.h"
#include "src/core/move_generation.h"
#include <iostream>

using namespace seajay;

int main() {
    std::cout << "Testing attack detection directly..." << std::endl;
    
    // Test MoveGenerator functions directly without using Board FEN parsing
    std::cout << "Testing knight attacks for e4..." << std::endl;
    Bitboard knightAttacks = MoveGenerator::getKnightAttacks(E4);
    std::cout << "Knight attacks bitboard: 0x" << std::hex << knightAttacks << std::dec << std::endl;
    
    std::cout << "Testing pawn attacks for e4..." << std::endl;
    Bitboard whitePawnAttacks = MoveGenerator::getPawnAttacks(E4, WHITE);
    std::cout << "White pawn attacks bitboard: 0x" << std::hex << whitePawnAttacks << std::dec << std::endl;
    
    std::cout << "Testing king attacks for e1..." << std::endl;
    Bitboard kingAttacks = MoveGenerator::getKingAttacks(E1);
    std::cout << "King attacks bitboard: 0x" << std::hex << kingAttacks << std::dec << std::endl;
    
    std::cout << "All direct attack function tests completed successfully!" << std::endl;
    return 0;
}