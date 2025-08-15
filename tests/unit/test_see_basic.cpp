/**
 * SeaJay Chess Engine - Stage 15: Static Exchange Evaluation
 * Basic SEE Tests (Day 1 & Day 2)
 */

#include "../test_framework.h"
#include "core/see.h"
#include "core/board.h"
#include "core/move_generation.h"
#include <iostream>

using namespace seajay;

// Helper to create a simple capture move
Move makeCapture(Square from, Square to) {
    return makeMove(from, to, CAPTURE);
}

// Day 1.4: Test simple 1-for-1 exchanges
TEST_CASE(PawnTakesPawn) {
    Board board;
    board.clear();
    
    // White pawn on e4, black pawn on d5
    board.setPiece(E4, WHITE_PAWN);
    board.setPiece(D5, BLACK_PAWN);
    board.setSideToMove(WHITE);
    
    Move capture = makeCapture(E4, D5);
    SEEValue value = see(board, capture);
    
    // Pawn takes pawn with no recapture = +100
    REQUIRE(value == 100);
}

TEST_CASE(PawnTakesPawnWithRecapture) {
    Board board;
    board.clear();
    
    // White pawn on e4, black pawn on d5, black pawn on c6 can recapture
    board.setPiece(E4, WHITE_PAWN);
    board.setPiece(D5, BLACK_PAWN);
    board.setPiece(C6, BLACK_PAWN);
    board.setSideToMove(WHITE);
    
    Move capture = makeCapture(E4, D5);
    SEEValue value = see(board, capture);
    
    // Pawn takes pawn, gets recaptured = 100 - 100 = 0
    REQUIRE(value == 0);
}

TEST_CASE(KnightTakesPawn) {
    Board board;
    board.clear();
    
    // White knight on f3, black pawn on e5
    board.setPiece(F3, WHITE_KNIGHT);
    board.setPiece(E5, BLACK_PAWN);
    board.setSideToMove(WHITE);
    
    Move capture = makeCapture(F3, E5);
    SEEValue value = see(board, capture);
    
    // Knight takes pawn with no recapture = +100
    REQUIRE(value == 100);
}

TEST_CASE(KnightTakesPawnWithRecapture) {
    Board board;
    board.clear();
    
    // White knight on f3, black pawn on e5, black bishop on c7 can recapture
    board.setPiece(F3, WHITE_KNIGHT);
    board.setPiece(E5, BLACK_PAWN);
    board.setPiece(C7, BLACK_BISHOP);
    board.setSideToMove(WHITE);
    
    Move capture = makeCapture(F3, E5);
    SEEValue value = see(board, capture);
    
    // Knight takes pawn, gets recaptured = 100 - 325 = -225
    REQUIRE(value == -225);
}

TEST_CASE(BishopTakesKnight) {
    Board board;
    board.clear();
    
    // White bishop on c1, black knight on f4
    board.setPiece(C1, WHITE_BISHOP);
    board.setPiece(F4, BLACK_KNIGHT);
    board.setSideToMove(WHITE);
    
    Move capture = makeCapture(C1, F4);
    SEEValue value = see(board, capture);
    
    // Bishop takes knight with no recapture = +325
    REQUIRE(value == 325);
}

TEST_CASE(BishopTakesKnightWithPawnRecapture) {
    Board board;
    board.clear();
    
    // White bishop on c1, black knight on f4, black pawn on g5 can recapture
    board.setPiece(C1, WHITE_BISHOP);
    board.setPiece(F4, BLACK_KNIGHT);
    board.setPiece(G5, BLACK_PAWN);
    board.setSideToMove(WHITE);
    
    Move capture = makeCapture(C1, F4);
    SEEValue value = see(board, capture);
    
    // Bishop takes knight, gets recaptured by pawn = 325 - 325 = 0
    REQUIRE(value == 0);
}

TEST_CASE(RookTakesQueen) {
    Board board;
    board.clear();
    
    // White rook on e1, black queen on e8
    board.setPiece(E1, WHITE_ROOK);
    board.setPiece(E8, BLACK_QUEEN);
    board.setSideToMove(WHITE);
    
    Move capture = makeCapture(E1, E8);
    SEEValue value = see(board, capture);
    
    // Rook takes queen with no recapture = +975
    REQUIRE(value == 975);
}

TEST_CASE(RookTakesQueenWithKingRecapture) {
    Board board;
    board.clear();
    
    // White rook on e1, black queen on e7, black king on e8 can recapture
    board.setPiece(E1, WHITE_ROOK);
    board.setPiece(E7, BLACK_QUEEN);
    board.setPiece(E8, BLACK_KING);
    board.setSideToMove(WHITE);
    
    Move capture = makeCapture(E1, E7);
    SEEValue value = see(board, capture);
    
    // Rook takes queen, gets recaptured by king = 975 - 500 = 475
    REQUIRE(value == 475);
}

TEST_CASE(QueenTakesPawn) {
    Board board;
    board.clear();
    
    // White queen on d1, black pawn on d7
    board.setPiece(D1, WHITE_QUEEN);
    board.setPiece(D7, BLACK_PAWN);
    board.setSideToMove(WHITE);
    
    Move capture = makeCapture(D1, D7);
    SEEValue value = see(board, capture);
    
    // Queen takes pawn with no recapture = +100
    REQUIRE(value == 100);
}

TEST_CASE(QueenTakesPawnWithRookRecapture) {
    Board board;
    board.clear();
    
    // White queen on d1, black pawn on d5, black rook on d8 can recapture
    board.setPiece(D1, WHITE_QUEEN);
    board.setPiece(D5, BLACK_PAWN);
    board.setPiece(D8, BLACK_ROOK);
    board.setSideToMove(WHITE);
    
    Move capture = makeCapture(D1, D5);
    SEEValue value = see(board, capture);
    
    // Queen takes pawn, gets recaptured by rook = 100 - 975 = -875
    REQUIRE(value == -875);
}

TEST_CASE(EnPassantCapture) {
    Board board;
    board.clear();
    
    // White pawn on e5, black pawn on d5 (just moved two squares)
    board.setPiece(E5, WHITE_PAWN);
    board.setPiece(D5, BLACK_PAWN);
    board.setEnPassantSquare(D6);
    board.setSideToMove(WHITE);
    
    Move epCapture = makeEnPassantMove(E5, D6);
    SEEValue value = see(board, epCapture);
    
    // En passant capture with no recapture = +100
    REQUIRE(value == 100);
}

TEST_CASE(EnPassantCaptureWithRecapture) {
    Board board;
    board.clear();
    
    // White pawn on e5, black pawn on d5, black bishop on a3 can recapture on d6
    board.setPiece(E5, WHITE_PAWN);
    board.setPiece(D5, BLACK_PAWN);
    board.setPiece(A3, BLACK_BISHOP);
    board.setEnPassantSquare(D6);
    board.setSideToMove(WHITE);
    
    Move epCapture = makeEnPassantMove(E5, D6);
    SEEValue value = see(board, epCapture);
    
    // En passant, gets recaptured = 100 - 100 = 0
    REQUIRE(value == 0);
}

TEST_CASE(NonCaptureMove) {
    Board board;
    board.clear();
    
    // White knight on b1 moves to c3 (no capture)
    board.setPiece(B1, WHITE_KNIGHT);
    board.setSideToMove(WHITE);
    
    Move move = makeMove(B1, C3);
    SEEValue value = see(board, move);
    
    // Non-capture move = 0
    REQUIRE(value == 0);
}

TEST_CASE(InvalidMove) {
    Board board;
    board.clear();
    
    // Try to move from empty square
    board.setSideToMove(WHITE);
    
    Move move = makeMove(E4, E5);
    SEEValue value = see(board, move);
    
    // Invalid move
    REQUIRE(value == SEE_INVALID);
}

TEST_CASE(SEESign) {
    Board board;
    
    // Positive SEE
    board.clear();
    board.setPiece(E4, WHITE_PAWN);
    board.setPiece(D5, BLACK_PAWN);
    board.setSideToMove(WHITE);
    
    Move capture = makeCapture(E4, D5);
    REQUIRE(seeSign(board, capture) == 1);
    
    // Negative SEE
    board.clear();
    board.setPiece(D1, WHITE_QUEEN);
    board.setPiece(D5, BLACK_PAWN);
    board.setPiece(D8, BLACK_ROOK);
    board.setSideToMove(WHITE);
    
    capture = makeCapture(D1, D5);
    REQUIRE(seeSign(board, capture) == -1);
    
    // Zero SEE
    board.clear();
    board.setPiece(E4, WHITE_PAWN);
    board.setPiece(D5, BLACK_PAWN);
    board.setPiece(C6, BLACK_PAWN);
    board.setSideToMove(WHITE);
    
    capture = makeCapture(E4, D5);
    REQUIRE(seeSign(board, capture) == 0);
}

TEST_CASE(SEEThreshold) {
    Board board;
    board.clear();
    
    board.setPiece(E4, WHITE_PAWN);
    board.setPiece(D5, BLACK_PAWN);
    board.setSideToMove(WHITE);
    
    Move capture = makeCapture(E4, D5);
    
    // SEE value is 100
    REQUIRE(seeGE(board, capture, 0));
    REQUIRE(seeGE(board, capture, 50));
    REQUIRE(seeGE(board, capture, 100));
    REQUIRE(!seeGE(board, capture, 101));
    REQUIRE(!seeGE(board, capture, 200));
}

TEST_CASE(Fingerprint) {
    REQUIRE(g_seeCalculator.fingerprint() == 0x5EE15000u);
    REQUIRE(g_seeCalculator.version() == 1u);
}

// Main test runner
int main() {
    std::cout << "Running SEE Basic Tests...\n\n";
    
    // Day 1 tests
    std::cout << "Day 1 Tests:\n";
    test_PawnTakesPawn();
    std::cout << "  ✓ PawnTakesPawn\n";
    test_PawnTakesPawnWithRecapture();
    std::cout << "  ✓ PawnTakesPawnWithRecapture\n";
    test_KnightTakesPawn();
    std::cout << "  ✓ KnightTakesPawn\n";
    test_KnightTakesPawnWithRecapture();
    std::cout << "  ✓ KnightTakesPawnWithRecapture\n";
    test_BishopTakesKnight();
    std::cout << "  ✓ BishopTakesKnight\n";
    test_BishopTakesKnightWithPawnRecapture();
    std::cout << "  ✓ BishopTakesKnightWithPawnRecapture\n";
    test_RookTakesQueen();
    std::cout << "  ✓ RookTakesQueen\n";
    test_RookTakesQueenWithKingRecapture();
    std::cout << "  ✓ RookTakesQueenWithKingRecapture\n";
    test_QueenTakesPawn();
    std::cout << "  ✓ QueenTakesPawn\n";
    test_QueenTakesPawnWithRookRecapture();
    std::cout << "  ✓ QueenTakesPawnWithRookRecapture\n";
    test_EnPassantCapture();
    std::cout << "  ✓ EnPassantCapture\n";
    test_EnPassantCaptureWithRecapture();
    std::cout << "  ✓ EnPassantCaptureWithRecapture\n";
    test_NonCaptureMove();
    std::cout << "  ✓ NonCaptureMove\n";
    test_InvalidMove();
    std::cout << "  ✓ InvalidMove\n";
    test_SEESign();
    std::cout << "  ✓ SEESign\n";
    test_SEEThreshold();
    std::cout << "  ✓ SEEThreshold\n";
    test_Fingerprint();
    std::cout << "  ✓ Fingerprint\n";
    
    std::cout << "\n✓ All Day 1 tests passed!\n";
    return 0;
}