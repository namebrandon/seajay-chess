#include "../../src/search/countermoves.h"
#include "../../src/core/board.h"
#include "../../src/core/types.h"
#include <iostream>
#include <cassert>

using namespace seajay;

void testClear() {
    CounterMoves cm;
    Board board;  // Create a board for testing
    board.setStartingPosition();
    
    // Add a countermove after e2-e4, play e7-e5
    Move prevMove = makeMove(E2, E4);
    board.makeMove(prevMove);  // Actually make the move on the board
    Move counterMove = makeMove(E7, E5);
    cm.update(board, prevMove, counterMove);
    
    // Verify it was stored
    assert(cm.getCounterMove(board, prevMove) == counterMove);
    assert(cm.hasCounterMove(board, prevMove));
    
    // Clear and verify it's gone
    cm.clear();
    assert(cm.getCounterMove(board, prevMove) == NO_MOVE);
    assert(!cm.hasCounterMove(board, prevMove));
    
    std::cout << "✓ testClear passed" << std::endl;
}

void testBasicUpdate() {
    CounterMoves cm;
    Board board;
    board.setStartingPosition();
    
    // Test basic update and retrieval
    Move prevMove = makeMove(D2, D4);
    board.makeMove(prevMove);  // Pawn to d4
    Move counterMove = makeMove(D7, D5);
    
    cm.update(board, prevMove, counterMove);
    assert(cm.getCounterMove(board, prevMove) == counterMove);
    assert(cm.hasCounterMove(board, prevMove));
    
    // Test overwrite
    Move newCounterMove = makeMove(G8, F6);
    cm.update(board, prevMove, newCounterMove);
    assert(cm.getCounterMove(board, prevMove) == newCounterMove);
    
    std::cout << "✓ testBasicUpdate passed" << std::endl;
}

void testSpecialMoves() {
    CounterMoves cm;
    Board board;
    board.setStartingPosition();
    
    // Test NO_MOVE handling
    cm.update(board, NO_MOVE, makeMove(E2, E4));
    assert(cm.getCounterMove(board, NO_MOVE) == NO_MOVE);
    assert(!cm.hasCounterMove(board, NO_MOVE));
    
    // Test that NO_MOVE returns NO_MOVE for any square lookup
    Move anyMove = makeMove(E2, E4);
    assert(cm.getCounterMove(board, NO_MOVE) == NO_MOVE);
    assert(!cm.hasCounterMove(board, NO_MOVE));
    
    // Test capture moves (should not be stored as countermoves)
    Move prevMove = makeMove(E2, E4);
    board.makeMove(prevMove);
    Move captureMove = makeCaptureMove(D7, E6);
    cm.update(board, prevMove, captureMove);
    assert(cm.getCounterMove(board, prevMove) == NO_MOVE);
    
    // Test promotion moves (should not be stored as countermoves)
    Move promotionMove = makePromotionMove(E7, E8, QUEEN);
    cm.update(board, prevMove, promotionMove);
    assert(cm.getCounterMove(board, prevMove) == NO_MOVE);
    
    std::cout << "✓ testSpecialMoves passed" << std::endl;
}

void testMultipleCountermoves() {
    CounterMoves cm;
    Board board;
    board.setStartingPosition();
    
    // Store multiple countermoves for different previous moves
    Move prev1 = makeMove(E2, E4);
    board.makeMove(prev1);
    Move counter1 = makeMove(E7, E5);
    cm.update(board, prev1, counter1);
    
    board.unmakeMove(prev1);  // Reset for next test
    
    Move prev2 = makeMove(D2, D4);
    board.makeMove(prev2);
    Move counter2 = makeMove(D7, D5);
    cm.update(board, prev2, counter2);
    
    board.unmakeMove(prev2);  // Reset for next test
    
    Move prev3 = makeMove(G1, F3);
    board.makeMove(prev3);
    Move counter3 = makeMove(G8, F6);
    cm.update(board, prev3, counter3);
    
    // Verify all are stored correctly
    // Note: We need to remake the moves to get the right piece positions
    board.setStartingPosition();
    board.makeMove(prev1);
    assert(cm.getCounterMove(board, prev1) == counter1);
    board.unmakeMove(prev1);
    
    board.makeMove(prev2);
    assert(cm.getCounterMove(board, prev2) == counter2);
    board.unmakeMove(prev2);
    
    board.makeMove(prev3);
    assert(cm.getCounterMove(board, prev3) == counter3);
    board.unmakeMove(prev3);
    
    // Verify non-existent move returns NO_MOVE
    Move nonExistent = makeMove(A2, A4);
    board.makeMove(nonExistent);
    assert(cm.getCounterMove(board, nonExistent) == NO_MOVE);
    assert(!cm.hasCounterMove(board, nonExistent));
    
    std::cout << "✓ testMultipleCountermoves passed" << std::endl;
}

void testPieceTypeIndexing() {
    CounterMoves cm;
    Board board;
    board.setStartingPosition();
    
    // Test that different pieces moving to the same square
    // don't overwrite each other's countermoves
    
    // Knight to e4
    board.makeMove(makeMove(G1, F3));  // Nf3
    board.makeMove(makeMove(E7, E6));  // e6
    board.makeMove(makeMove(F3, E5));  // Ne5
    board.makeMove(makeMove(D7, D6));  // d6
    Move knightMove = makeMove(E5, C4);  // Nc4
    board.makeMove(knightMove);
    
    // Store countermove for knight to c4
    Move knightCounter = makeMove(B7, B5);
    cm.update(board, knightMove, knightCounter);
    assert(cm.getCounterMove(board, knightMove) == knightCounter);
    
    // Now test bishop to c4 (different piece, same destination)
    board.setStartingPosition();
    board.makeMove(makeMove(E2, E4));  // e4
    board.makeMove(makeMove(E7, E5));  // e5
    board.makeMove(makeMove(F1, C4));  // Bc4
    Move bishopMove = makeMove(F1, C4);
    
    // This is a bit tricky - we need to position the board correctly
    board.setStartingPosition();
    board.makeMove(makeMove(E2, E4));
    board.makeMove(makeMove(E7, E5));
    Move bishopToC4 = makeMove(F1, C4);
    board.makeMove(bishopToC4);
    
    // Store different countermove for bishop to c4
    Move bishopCounter = makeMove(G8, F6);
    cm.update(board, bishopToC4, bishopCounter);
    
    // Verify they're stored separately
    assert(cm.getCounterMove(board, bishopToC4) == bishopCounter);
    
    // Go back and verify knight countermove is still there
    board.setStartingPosition();
    board.makeMove(makeMove(G1, F3));
    board.makeMove(makeMove(E7, E6));
    board.makeMove(makeMove(F3, E5));
    board.makeMove(makeMove(D7, D6));
    board.makeMove(knightMove);
    assert(cm.getCounterMove(board, knightMove) == knightCounter);
    
    std::cout << "✓ testPieceTypeIndexing passed (critical fix verified!)" << std::endl;
}

void testCastlingMoves() {
    CounterMoves cm;
    Board board;
    
    // Set up position where castling is possible
    board.setFromFEN("r3k2r/pppppppp/8/8/8/8/PPPPPPPP/R3K2R w KQkq - 0 1");
    
    // Castling should be allowed as both previous and counter moves
    Move castleKingside = makeCastlingMove(E1, G1);
    board.makeMove(castleKingside);
    Move response = makeMove(A7, A6);  // Simple pawn move as response
    
    cm.update(board, castleKingside, response);
    assert(cm.getCounterMove(board, castleKingside) == response);
    
    std::cout << "✓ testCastlingMoves passed" << std::endl;
}

void testTableBounds() {
    CounterMoves cm;
    Board board;
    
    // Test edge cases with piece-type indexing
    // Set up a position with pieces on edge squares
    board.setFromFEN("8/8/8/8/8/8/8/R6R w - - 0 1");
    
    Move a1h1 = makeMove(A1, B1);
    board.makeMove(a1h1);
    Move response1 = makeMove(H1, H2);
    cm.update(board, a1h1, response1);
    assert(cm.getCounterMove(board, a1h1) == response1);
    
    std::cout << "✓ testTableBounds passed" << std::endl;
}

int main() {
    std::cout << "Running CounterMoves unit tests..." << std::endl;
    std::cout << "Now with piece-type indexing to prevent collisions!" << std::endl << std::endl;
    
    testClear();
    testBasicUpdate();
    testSpecialMoves();
    testMultipleCountermoves();
    testPieceTypeIndexing();  // New critical test!
    testCastlingMoves();
    testTableBounds();
    
    std::cout << "\nAll CounterMoves tests passed! ✓" << std::endl;
    std::cout << "Critical fix: Now using [piece_type][to_square] indexing" << std::endl;
    return 0;
}