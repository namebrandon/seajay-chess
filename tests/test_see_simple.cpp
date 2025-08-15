#include "../src/core/see.h"
#include "../src/core/board.h"
#include "../src/core/move_generation.h"
#include <iostream>
#include <cassert>

using namespace seajay;

void testPawnTakesPawn() {
    Board board;
    board.clear();
    
    // White pawn on e4, black pawn on d5
    board.setPiece(E4, WHITE_PAWN);
    board.setPiece(D5, BLACK_PAWN);
    board.setSideToMove(WHITE);
    
    Move capture = makeMove(E4, D5, CAPTURE);
    SEEValue value = see(board, capture);
    
    // Pawn takes pawn with no recapture = +100
    assert(value == 100);
    std::cout << "✓ PawnTakesPawn: " << value << " (expected 100)" << std::endl;
}

void testPawnTakesPawnWithRecapture() {
    Board board;
    board.clear();
    
    // White pawn on e4, black pawn on d5, black pawn on c6 can recapture
    board.setPiece(E4, WHITE_PAWN);
    board.setPiece(D5, BLACK_PAWN);
    board.setPiece(C6, BLACK_PAWN);
    board.setSideToMove(WHITE);
    
    Move capture = makeMove(E4, D5, CAPTURE);
    SEEValue value = see(board, capture);
    
    // Pawn takes pawn, gets recaptured = 100 - 100 = 0
    assert(value == 0);
    std::cout << "✓ PawnTakesPawnWithRecapture: " << value << " (expected 0)" << std::endl;
}

void testKnightTakesPawn() {
    Board board;
    board.clear();
    
    // White knight on f3, black pawn on e5
    board.setPiece(F3, WHITE_KNIGHT);
    board.setPiece(E5, BLACK_PAWN);
    board.setSideToMove(WHITE);
    
    Move capture = makeMove(F3, E5, CAPTURE);
    SEEValue value = see(board, capture);
    
    // Knight takes pawn with no recapture = +100
    assert(value == 100);
    std::cout << "✓ KnightTakesPawn: " << value << " (expected 100)" << std::endl;
}

void testKnightTakesPawnWithRecapture() {
    Board board;
    board.clear();
    
    // White knight on f3, black pawn on e5, black bishop on c7 can recapture
    board.setPiece(F3, WHITE_KNIGHT);
    board.setPiece(E5, BLACK_PAWN);
    board.setPiece(C7, BLACK_BISHOP);
    board.setSideToMove(WHITE);
    
    Move capture = makeMove(F3, E5, CAPTURE);
    SEEValue value = see(board, capture);
    
    // Knight takes pawn, gets recaptured = 100 - 325 = -225
    assert(value == -225);
    std::cout << "✓ KnightTakesPawnWithRecapture: " << value << " (expected -225)" << std::endl;
}

void testBishopTakesKnight() {
    Board board;
    board.clear();
    
    // White bishop on c1, black knight on f4
    board.setPiece(C1, WHITE_BISHOP);
    board.setPiece(F4, BLACK_KNIGHT);
    board.setSideToMove(WHITE);
    
    Move capture = makeMove(C1, F4, CAPTURE);
    SEEValue value = see(board, capture);
    
    // Bishop takes knight with no recapture = +325
    assert(value == 325);
    std::cout << "✓ BishopTakesKnight: " << value << " (expected 325)" << std::endl;
}

void testRookTakesQueen() {
    Board board;
    board.clear();
    
    // White rook on e1, black queen on e8
    board.setPiece(E1, WHITE_ROOK);
    board.setPiece(E8, BLACK_QUEEN);
    board.setSideToMove(WHITE);
    
    Move capture = makeMove(E1, E8, CAPTURE);
    SEEValue value = see(board, capture);
    
    // Rook takes queen with no recapture = +975
    assert(value == 975);
    std::cout << "✓ RookTakesQueen: " << value << " (expected 975)" << std::endl;
}

void testQueenTakesPawnWithRookRecapture() {
    Board board;
    board.clear();
    
    // White queen on d1, black pawn on d5, black rook on d8 can recapture
    board.setPiece(D1, WHITE_QUEEN);
    board.setPiece(D5, BLACK_PAWN);
    board.setPiece(D8, BLACK_ROOK);
    board.setSideToMove(WHITE);
    
    Move capture = makeMove(D1, D5, CAPTURE);
    SEEValue value = see(board, capture);
    
    // Queen takes pawn, gets recaptured by rook = 100 - 975 = -875
    assert(value == -875);
    std::cout << "✓ QueenTakesPawnWithRookRecapture: " << value << " (expected -875)" << std::endl;
}

void testEnPassantCapture() {
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
    assert(value == 100);
    std::cout << "✓ EnPassantCapture: " << value << " (expected 100)" << std::endl;
}

void testNonCaptureMove() {
    Board board;
    board.clear();
    
    // White knight on b1 moves to c3 (no capture)
    board.setPiece(B1, WHITE_KNIGHT);
    board.setSideToMove(WHITE);
    
    Move move = makeMove(B1, C3);
    SEEValue value = see(board, move);
    
    // Non-capture move = 0
    assert(value == 0);
    std::cout << "✓ NonCaptureMove: " << value << " (expected 0)" << std::endl;
}

void testFingerprint() {
    assert(g_seeCalculator.fingerprint() == 0x5EE15000u);
    assert(g_seeCalculator.version() == 1u);
    std::cout << "✓ Fingerprint: 0x" << std::hex << g_seeCalculator.fingerprint() 
              << " Version: " << std::dec << g_seeCalculator.version() << std::endl;
}

int main() {
    std::cout << "\n=== Stage 15 Day 1 SEE Basic Tests ===" << std::endl;
    std::cout << "Testing simple 1-for-1 exchanges...\n" << std::endl;
    
    try {
        testPawnTakesPawn();
        testPawnTakesPawnWithRecapture();
        testKnightTakesPawn();
        testKnightTakesPawnWithRecapture();
        testBishopTakesKnight();
        testRookTakesQueen();
        testQueenTakesPawnWithRookRecapture();
        testEnPassantCapture();
        testNonCaptureMove();
        testFingerprint();
        
        std::cout << "\n✅ All Day 1 tests passed!" << std::endl;
        std::cout << "\nDay 1 Deliverables Complete:" << std::endl;
        std::cout << "  1.1 ✓ SEE types and constants" << std::endl;
        std::cout << "  1.2 ✓ Attack detection wrapper" << std::endl;
        std::cout << "  1.3 ✓ Basic swap array logic" << std::endl;
        std::cout << "  1.4 ✓ Simple 1-for-1 exchanges" << std::endl;
        
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "❌ Test failed: " << e.what() << std::endl;
        return 1;
    }
}