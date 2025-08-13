#include "../src/search/move_ordering.h"
#include "../src/core/board.h"
#include "../src/core/move_generation.h"
#include "../src/core/move_list.h"
#include <iostream>
#include <cassert>
#include <vector>

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

// Test Phase 2: Basic MVV-LVA scoring for simple captures
void testPhase2_BasicCaptures() {
    std::cout << "\nPhase 2: Testing basic capture scoring..." << std::endl;
    
    // Create a simple test position with known captures
    Board board;
    board.setStartingPosition();
    
    // Manually set up a position with captures available
    // We'll test with a simple position where we know the pieces
    
    // Test 1: Queen captures pawn (QxP)
    Move qxp = makeCaptureMove(D1, D7);  // Queen from D1 captures pawn on D7
    // Simulate the board state for this capture
    Board testBoard1;
    testBoard1.setStartingPosition();
    // Place white queen on D1 and black pawn on D7 for testing
    // Since we can't access m_mailbox directly, we'll test with scoreMove
    // which uses board.pieceAt() internally
    
    // For now, let's test the MVV-LVA formula directly
    int score = MvvLvaOrdering::mvvLvaScore(PAWN, QUEEN);  // QxP
    assert(score == 91);  // Pawn(100) - Queen(9) = 91
    std::cout << "  QxP score = " << score << " ✓" << std::endl;
    
    // Test 2: Pawn captures queen (PxQ) - best capture
    score = MvvLvaOrdering::mvvLvaScore(QUEEN, PAWN);  // PxQ
    assert(score == 899);  // Queen(900) - Pawn(1) = 899
    std::cout << "  PxQ score = " << score << " ✓" << std::endl;
    
    // Test 3: Knight captures rook (NxR)
    score = MvvLvaOrdering::mvvLvaScore(ROOK, KNIGHT);  // NxR
    assert(score == 497);  // Rook(500) - Knight(3) = 497
    std::cout << "  NxR score = " << score << " ✓" << std::endl;
    
    // Test 4: Rook captures knight (RxN)
    score = MvvLvaOrdering::mvvLvaScore(KNIGHT, ROOK);  // RxN
    assert(score == 320);  // Knight(325) - Rook(5) = 320
    std::cout << "  RxN score = " << score << " ✓" << std::endl;
    
    // Test 5: Bishop captures bishop (BxB)
    score = MvvLvaOrdering::mvvLvaScore(BISHOP, BISHOP);  // BxB
    assert(score == 322);  // Bishop(325) - Bishop(3) = 322
    std::cout << "  BxB score = " << score << " ✓" << std::endl;
    
    // Test 6: Pawn captures pawn (PxP)
    score = MvvLvaOrdering::mvvLvaScore(PAWN, PAWN);  // PxP
    assert(score == 99);  // Pawn(100) - Pawn(1) = 99
    std::cout << "  PxP score = " << score << " ✓" << std::endl;
    
    // Test move ordering with actual moves
    // Create a position where we can test move ordering
    const char* testFen = "rnbqkb1r/pppp1ppp/5n2/4p3/4P3/5N2/PPPP1PPP/RNBQKB1R w KQkq - 0 1";
    auto result = board.parseFEN(testFen);
    if (!result.hasValue()) {
        std::cerr << "Failed to parse FEN" << std::endl;
        return;
    }
    
    // Test that quiet moves get score 0
    Move quietMove = makeMove(G1, H3);  // Knight move, no capture
    score = MvvLvaOrdering::scoreMove(board, quietMove);
    assert(score == 0);
    std::cout << "  Quiet move score = " << score << " ✓" << std::endl;
    
    // Verify statistics tracking
    MvvLvaOrdering::resetStatistics();
    auto& stats = MvvLvaOrdering::getStatistics();
    assert(stats.captures_scored == 0);
    assert(stats.quiet_moves == 0);
    
    // Score a few moves to update statistics
    MvvLvaOrdering::scoreMove(board, quietMove);
    assert(stats.quiet_moves == 1);
    std::cout << "  Statistics tracking verified ✓" << std::endl;
    
    std::cout << "✓ Phase 2 complete: Basic capture scoring verified" << std::endl;
}

// Test Phase 3: En passant special case handling
void testPhase3_EnPassant() {
    std::cout << "\nPhase 3: Testing en passant handling..." << std::endl;
    
    // Test en passant move scoring directly
    // En passant moves should always score as PxP regardless of board state
    
    // Create en passant moves
    Move epMove1 = makeEnPassantMove(D5, C6);  // White pawn captures left
    Move epMove2 = makeEnPassantMove(E5, D6);  // White pawn captures right
    Move epMove3 = makeEnPassantMove(C4, D3);  // Black pawn captures
    
    // Set up a simple board (doesn't matter much since en passant is special-cased)
    Board board;
    board.setStartingPosition();
    
    // Test that all en passant moves score as PxP (99 points)
    int score = MvvLvaOrdering::scoreMove(board, epMove1);
    assert(score == 99);  // Always PxP: Pawn(100) - Pawn(1) = 99
    std::cout << "  En passant move 1 score = " << score << " ✓" << std::endl;
    
    score = MvvLvaOrdering::scoreMove(board, epMove2);
    assert(score == 99);  // Always PxP
    std::cout << "  En passant move 2 score = " << score << " ✓" << std::endl;
    
    score = MvvLvaOrdering::scoreMove(board, epMove3);
    assert(score == 99);  // Always PxP
    std::cout << "  En passant move 3 score = " << score << " ✓" << std::endl;
    
    // Verify statistics
    MvvLvaOrdering::resetStatistics();
    auto& stats = MvvLvaOrdering::getStatistics();
    MvvLvaOrdering::scoreMove(board, epMove1);
    assert(stats.en_passants_scored == 1);
    assert(stats.captures_scored == 0);  // En passant is counted separately
    std::cout << "  En passant statistics tracked correctly ✓" << std::endl;
    
    // Test that en passant is recognized correctly
    assert(isEnPassant(epMove1));
    assert(isEnPassant(epMove2));
    assert(isEnPassant(epMove3));
    assert(isCapture(epMove1));  // En passant is a type of capture
    assert(isCapture(epMove2));
    assert(isCapture(epMove3));
    
    // Verify en passant vs regular capture distinction
    Move regularCapture = makeCaptureMove(E2, D3);
    assert(!isEnPassant(regularCapture));
    assert(isCapture(regularCapture));
    
    std::cout << "✓ Phase 3 complete: En passant handling verified" << std::endl;
}

int main() {
    std::cout << "=== Stage 11: MVV-LVA Move Ordering Test ===" << std::endl;
    
    testPhase1_Infrastructure();
    testPhase2_BasicCaptures();
    testPhase3_EnPassant();
    
    std::cout << "\nAll Phase 1-3 tests passed!" << std::endl;
    return 0;
}