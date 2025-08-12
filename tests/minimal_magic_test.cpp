/**
 * Minimal test to isolate magic bitboards issue
 */

#include <iostream>
#include "../src/core/board.h"
#include "../src/core/move_generation.h"
#include "../src/core/move_list.h"
#include "../src/core/magic_bitboards.h"

using namespace seajay;

int main() {
    std::cout << "Initializing magic bitboards..." << std::endl;
    magic::initMagics();
    std::cout << "Magic bitboards initialized." << std::endl;
    
    std::cout << "\nTesting basic attack generation..." << std::endl;
    
    // Test a simple rook attack
    Bitboard occupied = 0;
    Bitboard attacks = magicRookAttacks(D4, occupied);
    std::cout << "Rook on D4 (empty board): " << __builtin_popcountll(attacks) << " squares attacked" << std::endl;
    
    // Test with some blockers
    occupied = squareBB(D6) | squareBB(B4);
    attacks = magicRookAttacks(D4, occupied);
    std::cout << "Rook on D4 (with blockers): " << __builtin_popcountll(attacks) << " squares attacked" << std::endl;
    
    // Test bishop
    attacks = magicBishopAttacks(D4, 0);
    std::cout << "Bishop on D4 (empty board): " << __builtin_popcountll(attacks) << " squares attacked" << std::endl;
    
    std::cout << "\nSetting up starting position..." << std::endl;
    Board board;
    if (!board.fromFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1")) {
        std::cout << "Failed to parse starting FEN!" << std::endl;
        return 1;
    }
    std::cout << "Board initialized." << std::endl;
    
    std::cout << "\nGenerating legal moves..." << std::endl;
    MoveList moves;
    MoveGenerator::generateLegalMoves(board, moves);
    std::cout << "Found " << moves.size() << " legal moves" << std::endl;
    
    if (moves.size() != 20) {
        std::cout << "ERROR: Expected 20 moves in starting position, got " << moves.size() << std::endl;
        return 1;
    }
    
    std::cout << "\n✅ All tests passed!" << std::endl;
    return 0;
}