#include "src/core/board.h"
#include "src/core/move_generation.h"
#include <iostream>
#include <cassert>

using namespace seajay;

void testPawnAttacks() {
    std::cout << "Testing pawn attacks...\n";
    
    Board board;
    board.fromFEN("rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR w KQkq - 0 1");
    
    // White pawn on e4 should attack d5 and f5
    assert(board.isAttacked(D5, WHITE));
    assert(board.isAttacked(F5, WHITE));
    assert(!board.isAttacked(E5, WHITE)); // Not attacked by pawn
    assert(!board.isAttacked(D4, WHITE)); // Not attacked by pawn
    
    std::cout << "✓ Pawn attacks working correctly\n";
}

void testKnightAttacks() {
    std::cout << "Testing knight attacks...\n";
    
    Board board;
    board.fromFEN("rnbqkbnr/pppppppp/8/8/8/5N2/PPPPPPPP/RNBQKB1R w KQkq - 0 1");
    
    // Knight on f3 should attack various squares
    assert(board.isAttacked(D2, WHITE));
    assert(board.isAttacked(D4, WHITE));
    assert(board.isAttacked(E1, WHITE));
    assert(board.isAttacked(E5, WHITE));
    assert(board.isAttacked(G1, WHITE));
    assert(board.isAttacked(G5, WHITE));
    assert(board.isAttacked(H2, WHITE));
    assert(board.isAttacked(H4, WHITE));
    
    assert(!board.isAttacked(F2, WHITE)); // Not attacked by knight
    assert(!board.isAttacked(F4, WHITE)); // Not attacked by knight
    
    std::cout << "✓ Knight attacks working correctly\n";
}

void testBishopAttacks() {
    std::cout << "Testing bishop attacks...\n";
    
    Board board;
    board.fromFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKB1R w KQkq - 0 1");
    
    // Bishop on c1 should attack along diagonal
    assert(board.isAttacked(B2, WHITE));
    assert(board.isAttacked(D2, WHITE));
    assert(board.isAttacked(E3, WHITE));
    assert(board.isAttacked(F4, WHITE));
    assert(board.isAttacked(G5, WHITE));
    assert(board.isAttacked(H6, WHITE));
    
    assert(!board.isAttacked(A3, WHITE)); // Not on diagonal
    assert(!board.isAttacked(C2, WHITE)); // Blocked by pawn
    
    std::cout << "✓ Bishop attacks working correctly\n";
}

void testRookAttacks() {
    std::cout << "Testing rook attacks...\n";
    
    Board board;
    board.fromFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQK2R w KQkq - 0 1");
    
    // Rook on h1 should attack along rank and file
    assert(board.isAttacked(H2, WHITE));
    assert(board.isAttacked(H3, WHITE));
    assert(board.isAttacked(H4, WHITE));
    assert(board.isAttacked(H5, WHITE));
    assert(board.isAttacked(H6, WHITE));
    assert(board.isAttacked(H7, WHITE));
    assert(board.isAttacked(H8, WHITE));
    
    assert(board.isAttacked(G1, WHITE));
    assert(board.isAttacked(F1, WHITE));
    assert(board.isAttacked(E1, WHITE));
    
    std::cout << "✓ Rook attacks working correctly\n";
}

void testQueenAttacks() {
    std::cout << "Testing queen attacks...\n";
    
    Board board;
    board.fromFEN("rnbqkbnr/pppppppp/8/8/3Q4/8/PPPPPPPP/RNB1KBNR w KQkq - 0 1");
    
    // Queen on d4 should attack in all directions
    // Rank attacks
    assert(board.isAttacked(A4, WHITE));
    assert(board.isAttacked(B4, WHITE));
    assert(board.isAttacked(C4, WHITE));
    assert(board.isAttacked(E4, WHITE));
    assert(board.isAttacked(F4, WHITE));
    assert(board.isAttacked(G4, WHITE));
    assert(board.isAttacked(H4, WHITE));
    
    // File attacks
    assert(board.isAttacked(D3, WHITE));
    assert(board.isAttacked(D5, WHITE));
    assert(board.isAttacked(D6, WHITE));
    assert(board.isAttacked(D7, WHITE));
    
    // Diagonal attacks
    assert(board.isAttacked(C3, WHITE));
    assert(board.isAttacked(E5, WHITE));
    assert(board.isAttacked(F6, WHITE));
    assert(board.isAttacked(G7, WHITE));
    
    std::cout << "✓ Queen attacks working correctly\n";
}

void testKingAttacks() {
    std::cout << "Testing king attacks...\n";
    
    Board board;
    board.fromFEN("rnbqkbnr/pppppppp/8/8/8/3K4/PPPPPPPP/RNBQ1BNR w kq - 0 1");
    
    // King on d3 should attack adjacent squares
    assert(board.isAttacked(C2, WHITE));
    assert(board.isAttacked(C3, WHITE));
    assert(board.isAttacked(C4, WHITE));
    assert(board.isAttacked(D2, WHITE));
    assert(board.isAttacked(D4, WHITE));
    assert(board.isAttacked(E2, WHITE));
    assert(board.isAttacked(E3, WHITE));
    assert(board.isAttacked(E4, WHITE));
    
    assert(!board.isAttacked(B3, WHITE)); // Too far away
    assert(!board.isAttacked(F3, WHITE)); // Too far away
    
    std::cout << "✓ King attacks working correctly\n";
}

void testComplexPosition() {
    std::cout << "Testing complex position...\n";
    
    // Kiwipete position - a complex tactical position
    Board board;
    board.fromFEN("r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1");
    
    // Test some known attack patterns
    assert(board.isAttacked(C3, WHITE)); // Attacked by queen on a3
    assert(board.isAttacked(H7, WHITE)); // Attacked by knight on h6
    assert(board.isAttacked(B6, BLACK)); // Attacked by bishop on b6
    
    std::cout << "✓ Complex position attacks working correctly\n";
}

int main() {
    std::cout << "Testing isAttacked() implementation..." << std::endl;
    std::cout.flush();
    
    try {
        std::cout << "Starting tests..." << std::endl;
        testPawnAttacks();
        std::cout << "Pawn tests complete" << std::endl;
        
        testKnightAttacks();
        std::cout << "Knight tests complete" << std::endl;
        
        testBishopAttacks();
        std::cout << "Bishop tests complete" << std::endl;
        
        testRookAttacks();
        std::cout << "Rook tests complete" << std::endl;
        
        testQueenAttacks();
        std::cout << "Queen tests complete" << std::endl;
        
        testKingAttacks();
        std::cout << "King tests complete" << std::endl;
        
        testComplexPosition();
        std::cout << "Complex position tests complete" << std::endl;
        
        std::cout << "All attack detection tests passed!" << std::endl;
        std::cout << "The isAttacked() function is working correctly for all piece types." << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Test failed: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "Test failed with unknown exception" << std::endl;
        return 1;
    }
    
    return 0;
}