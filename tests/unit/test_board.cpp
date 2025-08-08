#include "../../src/core/board.h"
#include "../../src/core/bitboard.h"
#include <iostream>
#include <cassert>

using namespace seajay;

void testClearBoard() {
    Board board;
    board.clear();
    
    for (Square s = 0; s < NUM_SQUARES; ++s) {
        assert(board.pieceAt(s) == NO_PIECE);
    }
    
    assert(board.occupied() == 0);
    assert(board.pieces(WHITE) == 0);
    assert(board.pieces(BLACK) == 0);
    assert(board.sideToMove() == WHITE);
    assert(board.castlingRights() == NO_CASTLING);
    assert(board.enPassantSquare() == NO_SQUARE);
    assert(board.halfmoveClock() == 0);
    assert(board.fullmoveNumber() == 1);
    
    std::cout << "✓ Clear board test passed\n";
}

void testStartingPosition() {
    Board board;
    board.setStartingPosition();
    
    assert(board.pieceAt(makeSquare(4, 0)) == WHITE_KING);
    assert(board.pieceAt(makeSquare(3, 0)) == WHITE_QUEEN);
    assert(board.pieceAt(makeSquare(0, 0)) == WHITE_ROOK);
    assert(board.pieceAt(makeSquare(7, 0)) == WHITE_ROOK);
    
    assert(board.pieceAt(makeSquare(4, 7)) == BLACK_KING);
    assert(board.pieceAt(makeSquare(3, 7)) == BLACK_QUEEN);
    assert(board.pieceAt(makeSquare(0, 7)) == BLACK_ROOK);
    assert(board.pieceAt(makeSquare(7, 7)) == BLACK_ROOK);
    
    for (File f = 0; f < 8; ++f) {
        assert(board.pieceAt(makeSquare(f, 1)) == WHITE_PAWN);
        assert(board.pieceAt(makeSquare(f, 6)) == BLACK_PAWN);
    }
    
    assert(board.sideToMove() == WHITE);
    assert(board.castlingRights() == ALL_CASTLING);
    assert(board.enPassantSquare() == NO_SQUARE);
    assert(board.halfmoveClock() == 0);
    assert(board.fullmoveNumber() == 1);
    
    std::cout << "✓ Starting position test passed\n";
}

void testSetAndRemovePiece() {
    Board board;
    board.clear();
    
    Square e4 = makeSquare(4, 3);
    board.setPiece(e4, WHITE_PAWN);
    
    assert(board.pieceAt(e4) == WHITE_PAWN);
    assert(testBit(board.pieces(WHITE), e4));
    assert(testBit(board.pieces(PAWN), e4));
    assert(testBit(board.occupied(), e4));
    
    board.removePiece(e4);
    assert(board.pieceAt(e4) == NO_PIECE);
    assert(!testBit(board.pieces(WHITE), e4));
    assert(!testBit(board.occupied(), e4));
    
    std::cout << "✓ Set and remove piece test passed\n";
}

void testMovePiece() {
    Board board;
    board.clear();
    
    Square e2 = makeSquare(4, 1);
    Square e4 = makeSquare(4, 3);
    
    board.setPiece(e2, WHITE_PAWN);
    board.movePiece(e2, e4);
    
    assert(board.pieceAt(e2) == NO_PIECE);
    assert(board.pieceAt(e4) == WHITE_PAWN);
    assert(testBit(board.pieces(WHITE), e4));
    assert(!testBit(board.pieces(WHITE), e2));
    
    std::cout << "✓ Move piece test passed\n";
}

void testFENParsing() {
    Board board;
    
    std::string startFEN = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
    assert(board.fromFEN(startFEN));
    assert(board.toFEN() == startFEN);
    
    std::string midgameFEN = "r1bqkb1r/pppp1ppp/2n2n2/1B2p3/4P3/5N2/PPPP1PPP/RNBQK2R w KQkq - 4 4";
    assert(board.fromFEN(midgameFEN));
    assert(board.toFEN() == midgameFEN);
    
    std::string enPassantFEN = "rnbqkbnr/ppp1p1pp/8/3pPp2/8/8/PPPP1PPP/RNBQKBNR w KQkq f6 0 3";
    assert(board.fromFEN(enPassantFEN));
    assert(board.enPassantSquare() == makeSquare(5, 5));
    assert(board.toFEN() == enPassantFEN);
    
    std::cout << "✓ FEN parsing test passed\n";
}

void testBitboardOperations() {
    Bitboard bb = 0;
    
    Square e4 = makeSquare(4, 3);
    setBit(bb, e4);
    assert(testBit(bb, e4));
    assert(popCount(bb) == 1);
    assert(lsb(bb) == e4);
    assert(msb(bb) == e4);
    
    Square d5 = makeSquare(3, 4);
    setBit(bb, d5);
    assert(popCount(bb) == 2);
    assert(lsb(bb) == e4);
    assert(msb(bb) == d5);
    
    clearBit(bb, e4);
    assert(!testBit(bb, e4));
    assert(testBit(bb, d5));
    assert(popCount(bb) == 1);
    
    bb = RANK_1_BB;
    assert(popCount(bb) == 8);
    Square s = popLsb(bb);
    assert(s == makeSquare(0, 0));
    assert(popCount(bb) == 7);
    
    std::cout << "✓ Bitboard operations test passed\n";
}

void testBitboardShifts() {
    Bitboard bb = squareBB(makeSquare(4, 3));
    
    Bitboard north = shift<NORTH>(bb);
    assert(testBit(north, makeSquare(4, 4)));
    
    Bitboard south = shift<SOUTH>(bb);
    assert(testBit(south, makeSquare(4, 2)));
    
    Bitboard east = shift<EAST>(bb);
    assert(testBit(east, makeSquare(5, 3)));
    
    Bitboard west = shift<WEST>(bb);
    assert(testBit(west, makeSquare(3, 3)));
    
    bb = FILE_H_BB;
    Bitboard eastFromH = shift<EAST>(bb);
    assert(eastFromH == 0);
    
    bb = FILE_A_BB;
    Bitboard westFromA = shift<WEST>(bb);
    assert(westFromA == 0);
    
    std::cout << "✓ Bitboard shift test passed\n";
}

void testSlidingPieceAttacks() {
    // Test rook attacks from e4 with some blockers
    Square e4 = makeSquare(4, 3);
    Bitboard occupied = 0;
    setBit(occupied, makeSquare(4, 6));  // Blocker on e7
    setBit(occupied, makeSquare(2, 3));  // Blocker on c4
    setBit(occupied, makeSquare(4, 1));  // Blocker on e2
    setBit(occupied, makeSquare(7, 3));  // Blocker on h4
    
    Bitboard rookAtks = rookAttacks(e4, occupied);
    
    // Should attack e5, e6 (blocked by e7)
    assert(testBit(rookAtks, makeSquare(4, 4)));  // e5
    assert(testBit(rookAtks, makeSquare(4, 5)));  // e6
    assert(!testBit(rookAtks, makeSquare(4, 6))); // e7 blocked
    
    // Should attack d4, c4 (includes blocker)
    assert(testBit(rookAtks, makeSquare(3, 3)));  // d4
    assert(testBit(rookAtks, makeSquare(2, 3)));  // c4 (blocker included)
    assert(!testBit(rookAtks, makeSquare(1, 3))); // b4 blocked
    
    // Test bishop attacks from d4
    Square d4 = makeSquare(3, 3);
    occupied = 0;
    setBit(occupied, makeSquare(1, 1));  // Blocker on b2
    setBit(occupied, makeSquare(6, 6));  // Blocker on g7
    
    Bitboard bishopAtks = bishopAttacks(d4, occupied);
    
    // Should attack c3, b2 (includes blocker)
    assert(testBit(bishopAtks, makeSquare(2, 2)));  // c3
    assert(testBit(bishopAtks, makeSquare(1, 1)));  // b2 (blocker included)
    assert(!testBit(bishopAtks, makeSquare(0, 0))); // a1 blocked
    
    // Should attack e5, f6, g7 (includes blocker)
    assert(testBit(bishopAtks, makeSquare(4, 4)));  // e5
    assert(testBit(bishopAtks, makeSquare(5, 5)));  // f6
    assert(testBit(bishopAtks, makeSquare(6, 6)));  // g7 (blocker included)
    assert(!testBit(bishopAtks, makeSquare(7, 7))); // h8 blocked
    
    // Test queen attacks (should be rook + bishop)
    Square e5 = makeSquare(4, 4);
    occupied = 0;
    setBit(occupied, makeSquare(4, 7));  // Blocker on e8
    setBit(occupied, makeSquare(7, 7));  // Blocker on h8
    
    Bitboard queenAtks = queenAttacks(e5, occupied);
    Bitboard expectedQueen = rookAttacks(e5, occupied) | bishopAttacks(e5, occupied);
    
    assert(queenAtks == expectedQueen);
    
    std::cout << "✓ Sliding piece attacks test passed\n";
}

void testBoardDisplay() {
    Board board;
    board.setStartingPosition();
    
    std::string display = board.toString();
    assert(!display.empty());
    
    std::cout << "✓ Board display test passed\n";
    std::cout << "\nStarting position:\n" << display << "\n";
}

int main() {
    std::cout << "\n=== Running Board and Bitboard Unit Tests ===\n\n";
    
    testClearBoard();
    testStartingPosition();
    testSetAndRemovePiece();
    testMovePiece();
    testFENParsing();
    testBitboardOperations();
    testBitboardShifts();
    testSlidingPieceAttacks();
    testBoardDisplay();
    
    std::cout << "\n✅ All tests passed!\n\n";
    
    return 0;
}