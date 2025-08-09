// Test suite for board safety infrastructure
#include "../src/core/board.h"
#include "../src/core/board_safety.h"
#include <iostream>
#include <vector>
#include <cassert>

using namespace seajay;

// Test that make/unmake properly restores state
void testMakeUnmakeStateRestoration() {
    std::cout << "Testing make/unmake state restoration...\n";
    
    Board board;
    board.setStartingPosition();
    
    // Save initial state
    BoardStateValidator::StateSnapshot initial(board);
    std::string initialFen = board.toFEN();
    
    // Test a series of moves
    std::vector<Move> moves = {
        makeMove(E2, E4, DOUBLE_PAWN),  // e4
        makeMove(E7, E5, DOUBLE_PAWN),  // e5
        makeMove(G1, F3),                // Nf3
        makeMove(B8, C6),                // Nc6
        makeCastlingMove(E1, G1),       // O-O
        makeMove(G8, F6),                // Nf6
    };
    
    std::vector<Board::UndoInfo> undoStack;
    
    // Make all moves
    for (Move move : moves) {
        Board::UndoInfo undo;
        board.makeMove(move, undo);
        undoStack.push_back(undo);
        
        // Validate state after each move
        assert(BoardStateValidator::validateFullIntegrity(board));
    }
    
    // Unmake all moves in reverse
    for (int i = moves.size() - 1; i >= 0; --i) {
        board.unmakeMove(moves[i], undoStack[i]);
        
        // Validate state after each unmake
        assert(BoardStateValidator::validateFullIntegrity(board));
    }
    
    // Verify complete restoration
    BoardStateValidator::StateSnapshot final(board);
    std::string finalFen = board.toFEN();
    
    assert(initial == final);
    assert(initialFen == finalFen);
    
    std::cout << "  ✓ State fully restored after make/unmake sequence\n";
}

// Test that zobrist keys are properly maintained
void testZobristConsistency() {
    std::cout << "Testing Zobrist key consistency...\n";
    
    Board board;
    board.setStartingPosition();
    
    // Make a move and verify zobrist
    Move move = makeMove(E2, E4, DOUBLE_PAWN);
    Board::UndoInfo undo;
    
    Hash keyBefore = board.zobristKey();
    board.makeMove(move, undo);
    Hash keyAfter = board.zobristKey();
    
    // Keys must be different after move
    assert(keyBefore != keyAfter);
    
    // Unmake and verify restoration
    board.unmakeMove(move, undo);
    Hash keyRestored = board.zobristKey();
    
    assert(keyBefore == keyRestored);
    assert(undo.zobristKey == keyRestored);
    
    std::cout << "  ✓ Zobrist keys properly maintained\n";
}

// Test special moves (castling, en passant, promotion)
void testSpecialMoves() {
    std::cout << "Testing special moves...\n";
    
    // Test castling
    {
        Board board;
        board.fromFEN("r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1");
        
        Move whiteKingside = makeCastlingMove(E1, G1);
        CompleteUndoInfo undo;
        
        board.makeMove(whiteKingside, undo);
        assert(board.pieceAt(G1) == WHITE_KING);
        assert(board.pieceAt(F1) == WHITE_ROOK);
        assert(board.pieceAt(E1) == NO_PIECE);
        assert(board.pieceAt(H1) == NO_PIECE);
        
        board.unmakeMove(whiteKingside, undo);
        assert(board.pieceAt(E1) == WHITE_KING);
        assert(board.pieceAt(H1) == WHITE_ROOK);
        assert(board.pieceAt(G1) == NO_PIECE);
        assert(board.pieceAt(F1) == NO_PIECE);
        
        std::cout << "  ✓ Castling works correctly\n";
    }
    
    // Test en passant
    {
        Board board;
        board.fromFEN("rnbqkbnr/1pp1pppp/8/p2pP3/8/8/PPPP1PPP/RNBQKBNR w KQkq d6 0 3");
        
        Move enPassant = makeEnPassantMove(E5, D6);
        CompleteUndoInfo undo;
        
        board.makeMove(enPassant, undo);
        assert(board.pieceAt(D6) == WHITE_PAWN);
        assert(board.pieceAt(D5) == NO_PIECE);  // Captured pawn removed
        assert(board.pieceAt(E5) == NO_PIECE);
        
        board.unmakeMove(enPassant, undo);
        assert(board.pieceAt(E5) == WHITE_PAWN);
        assert(board.pieceAt(D5) == BLACK_PAWN);  // Captured pawn restored
        assert(board.pieceAt(D6) == NO_PIECE);
        
        std::cout << "  ✓ En passant works correctly\n";
    }
    
    // Test promotion with capture
    {
        Board board;
        board.fromFEN("r1bqkbnr/pPpppppp/8/8/8/8/P1PPPPPP/RNBQKBNR w KQkq - 0 1");
        
        Move promoCap = makePromotionCaptureMove(B7, A8, QUEEN);
        CompleteUndoInfo undo;
        
        assert(board.pieceAt(B7) == WHITE_PAWN);
        assert(board.pieceAt(A8) == BLACK_ROOK);
        
        board.makeMove(promoCap, undo);
        assert(board.pieceAt(A8) == WHITE_QUEEN);
        assert(board.pieceAt(B7) == NO_PIECE);
        assert(undo.capturedPiece == BLACK_ROOK);
        
        board.unmakeMove(promoCap, undo);
        assert(board.pieceAt(B7) == WHITE_PAWN);
        assert(board.pieceAt(A8) == BLACK_ROOK);
        
        std::cout << "  ✓ Promotion with capture works correctly\n";
    }
}

// Test that fullmove counter is properly maintained
void testFullmoveCounter() {
    std::cout << "Testing fullmove counter...\n";
    
    Board board;
    board.setStartingPosition();
    assert(board.fullmoveNumber() == 1);
    
    // White's move - fullmove shouldn't change
    Move e4 = makeMove(E2, E4, DOUBLE_PAWN);
    Board::UndoInfo undo1;
    board.makeMove(e4, undo1);
    assert(board.fullmoveNumber() == 1);
    
    // Black's move - fullmove should increment
    Move e5 = makeMove(E7, E5, DOUBLE_PAWN);
    Board::UndoInfo undo2;
    board.makeMove(e5, undo2);
    assert(board.fullmoveNumber() == 2);
    
    // Unmake black's move
    board.unmakeMove(e5, undo2);
    assert(board.fullmoveNumber() == 1);
    
    // Unmake white's move
    board.unmakeMove(e4, undo1);
    assert(board.fullmoveNumber() == 1);
    
    std::cout << "  ✓ Fullmove counter properly maintained\n";
}

// Test move sequence validation
void testMoveSequenceValidation() {
    std::cout << "Testing move sequence validation...\n";
    
    Board board;
    board.setStartingPosition();
    
    // Create a sequence of moves
    std::vector<Move> sequence = {
        makeMove(E2, E4, DOUBLE_PAWN),
        makeMove(E7, E5, DOUBLE_PAWN),
        makeMove(G1, F3),
        makeMove(B8, C6),
        makeMove(F1, C4),
        makeMove(F8, C5),
        makeCastlingMove(E1, G1),
        makeCastlingMove(E8, G8),
    };
    
    // Validate the sequence
    assert(MoveSequenceValidator::validateSequence(board, sequence));
    
    std::cout << "  ✓ Move sequence validation works\n";
}

// Test corruption detection
void testCorruptionDetection() {
    std::cout << "Testing corruption detection...\n";
    
    #ifdef DEBUG
    Board board;
    board.setStartingPosition();
    
    // Save checksum
    uint32_t checksum = FastValidator::quickChecksum(board);
    
    // Make a move
    Move move = makeMove(E2, E4, DOUBLE_PAWN);
    Board::UndoInfo undo;
    board.makeMove(move, undo);
    
    // Checksum should change
    uint32_t newChecksum = FastValidator::quickChecksum(board);
    assert(checksum != newChecksum);
    
    // Unmake move
    board.unmakeMove(move, undo);
    
    // Checksum should be restored
    uint32_t restoredChecksum = FastValidator::quickChecksum(board);
    assert(checksum == restoredChecksum);
    
    std::cout << "  ✓ Corruption detection works\n";
    #else
    std::cout << "  - Skipped (requires DEBUG build)\n";
    #endif
}

int main() {
    std::cout << "\n=== Board Safety Test Suite ===\n\n";
    
    try {
        testMakeUnmakeStateRestoration();
        testZobristConsistency();
        testSpecialMoves();
        testFullmoveCounter();
        testMoveSequenceValidation();
        testCorruptionDetection();
        
        std::cout << "\n✅ All safety tests passed!\n\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "\n❌ Test failed with exception: " << e.what() << "\n";
        return 1;
    }
}