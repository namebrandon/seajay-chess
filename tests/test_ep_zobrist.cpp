/**
 * Test En Passant Zobrist Handling
 * Verifies that en passant square only affects hash when capture is possible
 */

#include "../src/core/board.h"
#include "../src/core/types.h"
#include <iostream>
#include <cassert>

using namespace seajay;

void testEnPassantOnlyWhenCapturable() {
    std::cout << "Testing En Passant Zobrist Handling\n";
    std::cout << "=====================================\n\n";
    
    Board board1, board2;
    
    // Test 1: En passant square set but no enemy pawn can capture
    std::cout << "Test 1: False en passant (no enemy pawn nearby)\n";
    board1.parseFEN("8/8/8/2k5/3P4/8/8/3K4 b - e3 0 1");
    board2.parseFEN("8/8/8/2k5/3P4/8/8/3K4 b - - 0 1");
    
    std::cout << "  Position with e3 set:    0x" << std::hex << board1.zobristKey() << "\n";
    std::cout << "  Position without e3 set: 0x" << board2.zobristKey() << std::dec << "\n";
    
    if (board1.zobristKey() == board2.zobristKey()) {
        std::cout << "  ✓ PASS: Hashes are equal (en passant ignored when not capturable)\n";
    } else {
        std::cout << "  ✗ FAIL: Hashes differ (en passant incorrectly included)\n";
    }
    
    // Test 2: En passant square set with enemy pawn that can capture
    std::cout << "\nTest 2: True en passant (enemy pawn can capture)\n";
    board1.parseFEN("8/8/8/2k5/2pP4/8/8/3K4 b - d3 0 1");
    board2.parseFEN("8/8/8/2k5/2pP4/8/8/3K4 b - - 0 1");
    
    std::cout << "  Position with d3 set:    0x" << std::hex << board1.zobristKey() << "\n";
    std::cout << "  Position without d3 set: 0x" << board2.zobristKey() << std::dec << "\n";
    
    if (board1.zobristKey() != board2.zobristKey()) {
        std::cout << "  ✓ PASS: Hashes differ (en passant correctly included)\n";
    } else {
        std::cout << "  ✗ FAIL: Hashes are equal (en passant incorrectly ignored)\n";
    }
    
    // Test 3: En passant with pawn on right side
    std::cout << "\nTest 3: En passant with pawn on right\n";
    board1.parseFEN("8/8/8/8/3Pp3/8/8/k6K b - d3 0 1");
    board2.parseFEN("8/8/8/8/3Pp3/8/8/k6K b - - 0 1");
    
    std::cout << "  Position with d3 set:    0x" << std::hex << board1.zobristKey() << "\n";
    std::cout << "  Position without d3 set: 0x" << board2.zobristKey() << std::dec << "\n";
    
    if (board1.zobristKey() != board2.zobristKey()) {
        std::cout << "  ✓ PASS: Hashes differ (en passant correctly included)\n";
    } else {
        std::cout << "  ✗ FAIL: Hashes are equal (en passant incorrectly ignored)\n";
    }
    
    // Test 4: En passant on white's turn
    std::cout << "\nTest 4: En passant for white\n";
    board1.parseFEN("8/8/8/4Pp2/8/8/8/k6K w - f6 0 1");
    board2.parseFEN("8/8/8/4Pp2/8/8/8/k6K w - - 0 1");
    
    std::cout << "  Position with f6 set:    0x" << std::hex << board1.zobristKey() << "\n";
    std::cout << "  Position without f6 set: 0x" << board2.zobristKey() << std::dec << "\n";
    
    if (board1.zobristKey() != board2.zobristKey()) {
        std::cout << "  ✓ PASS: Hashes differ (en passant correctly included)\n";
    } else {
        std::cout << "  ✗ FAIL: Hashes are equal (en passant incorrectly ignored)\n";
    }
    
    // Test 5: Correct en passant scenario
    std::cout << "\nTest 5: Proper en passant scenario\n";
    
    // Position where white has advanced to e5, then black plays d7-d5
    // creating en passant opportunity
    board1.parseFEN("rnbqkbnr/ppp1pppp/8/3pP3/8/8/PPPP1PPP/RNBQKBNR w KQkq d6 0 3");
    uint64_t hash1 = board1.zobristKey();
    
    // Same position but no en passant square
    board2.parseFEN("rnbqkbnr/ppp1pppp/8/3pP3/8/8/PPPP1PPP/RNBQKBNR w KQkq - 0 3");
    uint64_t hash2 = board2.zobristKey();
    
    std::cout << "  Position with d6 en passant (e5 can capture):  0x" << std::hex << hash1 << "\n";
    std::cout << "  Same position without en passant:               0x" << hash2 << "\n";
    
    if (hash1 != hash2) {
        std::cout << "  ✓ PASS: En passant correctly affects hash when capturable\n";
    } else {
        std::cout << "  ✗ FAIL: En passant not included when it should be\n";
    }
    
    std::cout << "\n=== All En Passant Tests Complete ===\n";
}

int main() {
    testEnPassantOnlyWhenCapturable();
    return 0;
}