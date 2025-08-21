#include "../../src/search/countermoves.h"
#include "../../src/core/types.h"
#include <iostream>
#include <cassert>

using namespace seajay;

void testClear() {
    CounterMoves cm;
    
    // Add a countermove
    Move prevMove = makeMove(E2, E4);
    Move counterMove = makeMove(E7, E5);
    cm.update(prevMove, counterMove);
    
    // Verify it was stored
    assert(cm.getCounterMove(prevMove) == counterMove);
    assert(cm.hasCounterMove(prevMove));
    
    // Clear and verify it's gone
    cm.clear();
    assert(cm.getCounterMove(prevMove) == NO_MOVE);
    assert(!cm.hasCounterMove(prevMove));
    
    std::cout << "✓ testClear passed" << std::endl;
}

void testBasicUpdate() {
    CounterMoves cm;
    
    // Test basic update and retrieval
    Move prevMove = makeMove(D2, D4);
    Move counterMove = makeMove(D7, D5);
    
    cm.update(prevMove, counterMove);
    assert(cm.getCounterMove(prevMove) == counterMove);
    assert(cm.hasCounterMove(prevMove));
    
    // Test overwrite
    Move newCounterMove = makeMove(G8, F6);
    cm.update(prevMove, newCounterMove);
    assert(cm.getCounterMove(prevMove) == newCounterMove);
    
    std::cout << "✓ testBasicUpdate passed" << std::endl;
}

void testSpecialMoves() {
    CounterMoves cm;
    
    // Test NO_MOVE handling
    cm.update(NO_MOVE, makeMove(E2, E4));
    assert(cm.getCounterMove(NO_MOVE) == NO_MOVE);
    assert(!cm.hasCounterMove(NO_MOVE));
    
    // Test that NO_MOVE returns NO_MOVE for any square lookup
    Move anyMove = makeMove(E2, E4);
    assert(cm.getCounterMove(NO_MOVE) == NO_MOVE);
    assert(!cm.hasCounterMove(NO_MOVE));
    
    // Test capture moves (should not be stored as countermoves)
    Move prevMove = makeMove(E2, E4);
    Move captureMove = makeCaptureMove(D7, E6);
    cm.update(prevMove, captureMove);
    assert(cm.getCounterMove(prevMove) == NO_MOVE);
    
    // Test promotion moves (should not be stored as countermoves)
    Move promotionMove = makePromotionMove(E7, E8, QUEEN);
    cm.update(prevMove, promotionMove);
    assert(cm.getCounterMove(prevMove) == NO_MOVE);
    
    std::cout << "✓ testSpecialMoves passed" << std::endl;
}

void testMultipleCountermoves() {
    CounterMoves cm;
    
    // Store multiple countermoves for different previous moves
    Move prev1 = makeMove(E2, E4);
    Move counter1 = makeMove(E7, E5);
    
    Move prev2 = makeMove(D2, D4);
    Move counter2 = makeMove(D7, D5);
    
    Move prev3 = makeMove(G1, F3);
    Move counter3 = makeMove(G8, F6);
    
    cm.update(prev1, counter1);
    cm.update(prev2, counter2);
    cm.update(prev3, counter3);
    
    // Verify all are stored correctly
    assert(cm.getCounterMove(prev1) == counter1);
    assert(cm.getCounterMove(prev2) == counter2);
    assert(cm.getCounterMove(prev3) == counter3);
    
    // Verify non-existent move returns NO_MOVE
    Move nonExistent = makeMove(A2, A4);
    assert(cm.getCounterMove(nonExistent) == NO_MOVE);
    assert(!cm.hasCounterMove(nonExistent));
    
    std::cout << "✓ testMultipleCountermoves passed" << std::endl;
}

void testCastlingMoves() {
    CounterMoves cm;
    
    // Castling should be allowed as both previous and counter moves
    Move castleKingside = makeCastlingMove(E1, G1);
    Move response = makeCastlingMove(E8, G8);
    
    cm.update(castleKingside, response);
    assert(cm.getCounterMove(castleKingside) == response);
    
    std::cout << "✓ testCastlingMoves passed" << std::endl;
}

void testTableBounds() {
    CounterMoves cm;
    
    // Test all corner squares
    Move a1h8 = makeMove(A1, H8);
    Move h1a8 = makeMove(H1, A8);
    Move response1 = makeMove(B2, B3);
    Move response2 = makeMove(G7, G6);
    
    cm.update(a1h8, response1);
    cm.update(h1a8, response2);
    
    assert(cm.getCounterMove(a1h8) == response1);
    assert(cm.getCounterMove(h1a8) == response2);
    
    std::cout << "✓ testTableBounds passed" << std::endl;
}

int main() {
    std::cout << "Running CounterMoves unit tests..." << std::endl;
    
    testClear();
    testBasicUpdate();
    testSpecialMoves();
    testMultipleCountermoves();
    testCastlingMoves();
    testTableBounds();
    
    std::cout << "\nAll CounterMoves tests passed! ✓" << std::endl;
    return 0;
}