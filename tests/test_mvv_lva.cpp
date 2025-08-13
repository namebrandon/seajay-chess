#include "../src/search/move_ordering.h"
#include "../src/core/board.h"
#include "../src/core/move_generation.h"
#include <iostream>
#include <cassert>

using namespace seajay;
using namespace seajay::search;

// Test that infrastructure compiles and basic assertions pass
void testPhase1_Infrastructure() {
    std::cout << "Phase 1: Testing infrastructure and type safety..." << std::endl;
    
    // Test that tables are correctly indexed
    assert(VICTIM_VALUES[PAWN] == 100);
    assert(VICTIM_VALUES[KNIGHT] == 325);
    assert(VICTIM_VALUES[BISHOP] == 325);
    assert(VICTIM_VALUES[ROOK] == 500);
    assert(VICTIM_VALUES[QUEEN] == 900);
    assert(VICTIM_VALUES[KING] == 10000);
    assert(VICTIM_VALUES[NO_PIECE_TYPE] == 0);
    
    assert(ATTACKER_VALUES[PAWN] == 1);
    assert(ATTACKER_VALUES[KNIGHT] == 3);
    assert(ATTACKER_VALUES[BISHOP] == 3);
    assert(ATTACKER_VALUES[ROOK] == 5);
    assert(ATTACKER_VALUES[QUEEN] == 9);
    assert(ATTACKER_VALUES[KING] == 100);
    assert(ATTACKER_VALUES[NO_PIECE_TYPE] == 0);
    
    // Test MoveScore structure
    // Note: operator< is inverted for sorting (higher scores come first)
    // operator< returns (score > other.score) to sort in descending order
    MoveScore ms1{makeMove(E2, E4), 100};
    MoveScore ms2{makeMove(D7, D5), 50};
    assert((ms1 < ms2) == true);  // ms1.score(100) > ms2.score(50), so returns true
    assert((ms2 < ms1) == false); // ms2.score(50) > ms1.score(100), so returns false
    
    // Test mvvLvaScore formula
    int score = MvvLvaOrdering::mvvLvaScore(QUEEN, PAWN);  // PxQ
    assert(score == 899);  // 900 - 1
    
    score = MvvLvaOrdering::mvvLvaScore(PAWN, QUEEN);  // QxP
    assert(score == 91);   // 100 - 9
    
    score = MvvLvaOrdering::mvvLvaScore(ROOK, KNIGHT);  // NxR
    assert(score == 497);  // 500 - 3
    
    score = MvvLvaOrdering::mvvLvaScore(PAWN, PAWN);  // PxP (en passant case)
    assert(score == 99);   // 100 - 1
    
    std::cout << "✓ Phase 1 complete: Infrastructure and type safety verified" << std::endl;
}

int main() {
    std::cout << "=== Stage 11: MVV-LVA Move Ordering Test ===" << std::endl;
    
    testPhase1_Infrastructure();
    
    std::cout << "\nAll Phase 1 tests passed!" << std::endl;
    return 0;
}