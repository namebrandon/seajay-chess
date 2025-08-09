#include "src/core/board.h"
#include "src/core/move_generation.h"
#include <iostream>
#include <cassert>

using namespace seajay;

int main() {
    std::cout << "Testing isAttacked() implementation..." << std::endl;
    
    // Use setStartingPosition() instead of FEN parsing
    Board board;
    board.setStartingPosition();
    
    std::cout << "Starting position loaded successfully" << std::endl;
    
    // Test basic pawn attacks from starting position
    std::cout << "Testing pawn attacks..." << std::endl;
    
    // In starting position, white pawns on second rank don't attack anything immediately
    // But we can test that they don't attack squares they shouldn't
    assert(!board.isAttacked(E4, WHITE)); // e4 not attacked by any white piece initially
    assert(!board.isAttacked(D4, WHITE)); // d4 not attacked by any white piece initially
    
    std::cout << "✓ Basic starting position tests passed" << std::endl;
    
    // Let's test by manually setting up a simple position
    board.clear();
    
    // Place a white pawn on e4
    board.setPiece(E4, WHITE_PAWN);
    
    // Now test if it attacks d5 and f5
    std::cout << "Testing white pawn on e4 attacks..." << std::endl;
    assert(board.isAttacked(D5, WHITE)); // Pawn on e4 should attack d5
    assert(board.isAttacked(F5, WHITE)); // Pawn on e4 should attack f5
    assert(!board.isAttacked(E5, WHITE)); // Pawn on e4 should NOT attack e5 (straight ahead)
    
    std::cout << "✓ Pawn attack tests passed" << std::endl;
    
    // Test knight attacks
    board.clear();
    board.setPiece(F3, WHITE_KNIGHT);
    
    std::cout << "Testing white knight on f3 attacks..." << std::endl;
    assert(board.isAttacked(E1, WHITE)); // Knight should attack e1
    assert(board.isAttacked(G1, WHITE)); // Knight should attack g1
    assert(board.isAttacked(D2, WHITE)); // Knight should attack d2
    assert(board.isAttacked(H2, WHITE)); // Knight should attack h2
    assert(board.isAttacked(D4, WHITE)); // Knight should attack d4
    assert(board.isAttacked(H4, WHITE)); // Knight should attack h4
    assert(board.isAttacked(E5, WHITE)); // Knight should attack e5
    assert(board.isAttacked(G5, WHITE)); // Knight should attack g5
    
    std::cout << "✓ Knight attack tests passed" << std::endl;
    
    // Test king attacks
    board.clear();
    board.setPiece(E4, WHITE_KING);
    
    std::cout << "Testing white king on e4 attacks..." << std::endl;
    assert(board.isAttacked(D3, WHITE)); // King should attack all adjacent squares
    assert(board.isAttacked(E3, WHITE));
    assert(board.isAttacked(F3, WHITE));
    assert(board.isAttacked(D4, WHITE));
    assert(board.isAttacked(F4, WHITE));
    assert(board.isAttacked(D5, WHITE));
    assert(board.isAttacked(E5, WHITE));
    assert(board.isAttacked(F5, WHITE));
    
    assert(!board.isAttacked(C3, WHITE)); // King should NOT attack distant squares
    assert(!board.isAttacked(G5, WHITE));
    
    std::cout << "✓ King attack tests passed" << std::endl;
    
    // Test sliding piece attacks
    board.clear();
    board.setPiece(D4, WHITE_BISHOP);
    
    std::cout << "Testing white bishop on d4 attacks..." << std::endl;
    assert(board.isAttacked(C3, WHITE)); // Diagonal attacks
    assert(board.isAttacked(B2, WHITE));
    assert(board.isAttacked(A1, WHITE));
    assert(board.isAttacked(E5, WHITE));
    assert(board.isAttacked(F6, WHITE));
    assert(board.isAttacked(G7, WHITE));
    assert(board.isAttacked(H8, WHITE));
    assert(board.isAttacked(C5, WHITE));
    assert(board.isAttacked(B6, WHITE));
    assert(board.isAttacked(A7, WHITE));
    assert(board.isAttacked(E3, WHITE));
    assert(board.isAttacked(F2, WHITE));
    assert(board.isAttacked(G1, WHITE));
    
    assert(!board.isAttacked(D3, WHITE)); // Should not attack along rank/file
    assert(!board.isAttacked(D5, WHITE));
    assert(!board.isAttacked(C4, WHITE));
    assert(!board.isAttacked(E4, WHITE));
    
    std::cout << "✓ Bishop attack tests passed" << std::endl;
    
    // Test rook attacks
    board.clear();
    board.setPiece(D4, WHITE_ROOK);
    
    std::cout << "Testing white rook on d4 attacks..." << std::endl;
    // File attacks
    assert(board.isAttacked(D1, WHITE));
    assert(board.isAttacked(D2, WHITE));
    assert(board.isAttacked(D3, WHITE));
    assert(board.isAttacked(D5, WHITE));
    assert(board.isAttacked(D6, WHITE));
    assert(board.isAttacked(D7, WHITE));
    assert(board.isAttacked(D8, WHITE));
    
    // Rank attacks
    assert(board.isAttacked(A4, WHITE));
    assert(board.isAttacked(B4, WHITE));
    assert(board.isAttacked(C4, WHITE));
    assert(board.isAttacked(E4, WHITE));
    assert(board.isAttacked(F4, WHITE));
    assert(board.isAttacked(G4, WHITE));
    assert(board.isAttacked(H4, WHITE));
    
    // Should not attack diagonals
    assert(!board.isAttacked(C3, WHITE));
    assert(!board.isAttacked(E5, WHITE));
    
    std::cout << "✓ Rook attack tests passed" << std::endl;
    
    // Test queen attacks (combination of bishop and rook)
    board.clear();
    board.setPiece(D4, WHITE_QUEEN);
    
    std::cout << "Testing white queen on d4 attacks..." << std::endl;
    // Should attack like both bishop and rook
    assert(board.isAttacked(D1, WHITE)); // File
    assert(board.isAttacked(A4, WHITE)); // Rank
    assert(board.isAttacked(A1, WHITE)); // Diagonal
    assert(board.isAttacked(G7, WHITE)); // Diagonal
    
    std::cout << "✓ Queen attack tests passed" << std::endl;
    
    std::cout << "\n🎉 All isAttacked() tests passed!\n";
    std::cout << "The isAttacked() function is working correctly for all piece types.\n";
    
    return 0;
}