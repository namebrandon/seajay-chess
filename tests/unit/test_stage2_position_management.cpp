#include "../../src/core/board.h"
#include "../../src/core/bitboard.h"
#include <iostream>
#include <cassert>
#include <vector>
#include <string>

using namespace seajay;

// Test the new Result<T,E> error handling system
void testResultType() {
    std::cout << "Testing Result<T,E> type...\n";
    
    // Test successful result
    FenResult success = true;
    assert(success.hasValue());
    assert(!success.hasError());
    assert(success);
    assert(success.value() == true);
    
    // Test error result
    FenResult error = makeFenError(FenError::InvalidBoard, "Test error", 5);
    assert(!error.hasValue());
    assert(error.hasError());
    assert(!error);
    assert(error.error().error == FenError::InvalidBoard);
    assert(error.error().message == "Test error");
    assert(error.error().position == 5);
    
    std::cout << "✓ Result type tests passed\n";
}

// Test enhanced FEN parser with expert-recommended positions
void testFenParserSafety() {
    std::cout << "Testing FEN parser safety enhancements...\n";
    
    Board board;
    
    // Test valid starting position
    auto result = board.parseFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    assert(result.hasValue());
    assert(board.validatePosition());
    assert(board.validateBitboardSync());
    assert(board.validateZobrist());
    std::cout << "  ✓ Starting position parsed successfully\n";
    
    // Test Kiwipete position (complex tactical position)
    result = board.parseFEN("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1");
    assert(result.hasValue());
    assert(board.validatePosition());
    assert(board.validateBitboardSync());
    assert(board.validateZobrist());
    std::cout << "  ✓ Kiwipete position parsed successfully\n";
    
    // Test Position 4 (castling and promotion edge cases)
    result = board.parseFEN("r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1");
    assert(result.hasValue());
    assert(board.validatePosition());
    std::cout << "  ✓ Position 4 parsed successfully\n";
    
    std::cout << "✓ FEN parser safety tests passed\n";
}

// Test error handling with malformed FEN strings
void testFenErrorHandling() {
    std::cout << "Testing FEN error handling...\n";
    
    Board board;
    
    // Test empty FEN
    auto result = board.parseFEN("");
    assert(!result);
    assert(result.error().error == FenError::InvalidFormat);
    std::cout << "  ✓ Empty FEN rejected: " << result.error().message << "\n";
    
    // Test wrong number of fields
    result = board.parseFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq");
    assert(!result);
    assert(result.error().error == FenError::InvalidFormat);
    std::cout << "  ✓ Incomplete FEN rejected: " << result.error().message << "\n";
    
    // Test invalid board (too many pieces in rank - buffer overflow protection)
    result = board.parseFEN("rnbqkbnrr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    assert(!result);
    assert(result.error().error == FenError::BoardOverflow);
    std::cout << "  ✓ Board overflow rejected: " << result.error().message << "\n";
    
    // Test invalid piece character
    result = board.parseFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBXR w KQkq - 0 1");
    assert(!result);
    assert(result.error().error == FenError::InvalidPieceChar);
    std::cout << "  ✓ Invalid piece character rejected: " << result.error().message << "\n";
    
    // Test pawn on back rank
    result = board.parseFEN("Pnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    assert(!result);
    assert(result.error().error == FenError::PawnOnBackRank);
    std::cout << "  ✓ Pawn on back rank rejected: " << result.error().message << "\n";
    
    // Test invalid side to move
    result = board.parseFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR x KQkq - 0 1");
    assert(!result);
    assert(result.error().error == FenError::InvalidSideToMove);
    std::cout << "  ✓ Invalid side to move rejected: " << result.error().message << "\n";
    
    // Test invalid castling rights
    result = board.parseFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkqX - 0 1");
    assert(!result);
    assert(result.error().error == FenError::InvalidCastling);
    std::cout << "  ✓ Invalid castling rights rejected: " << result.error().message << "\n";
    
    // Test invalid en passant square
    result = board.parseFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq x9 0 1");
    assert(!result);
    assert(result.error().error == FenError::InvalidEnPassant);
    std::cout << "  ✓ Invalid en passant rejected: " << result.error().message << "\n";
    
    // Test invalid halfmove clock
    result = board.parseFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - abc 1");
    assert(!result);
    assert(result.error().error == FenError::InvalidClocks);
    std::cout << "  ✓ Invalid halfmove clock rejected: " << result.error().message << "\n";
    
    // Test invalid fullmove number
    result = board.parseFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 0");
    assert(!result);
    assert(result.error().error == FenError::InvalidClocks);
    std::cout << "  ✓ Invalid fullmove number rejected: " << result.error().message << "\n";
    
    std::cout << "✓ FEN error handling tests passed\n";
}

// Test critical validation functions
void testValidationFunctions() {
    std::cout << "Testing validation functions...\n";
    
    Board board;
    
    // Test valid position
    board.parseFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    assert(board.validatePosition());
    assert(board.validateBitboardSync());
    assert(board.validateZobrist());
    assert(board.validateKings());
    assert(board.validatePieceCounts());
    assert(board.validateCastlingRights());
    assert(board.validateEnPassant());
    // Note: validateNotInCheck() is placeholder for Stage 4
    std::cout << "  ✓ All validations pass for starting position\n";
    
    // Test missing king (should fail)
    board.clear();
    board.setPiece(SQ_E1, WHITE_QUEEN);  // Queen instead of king
    board.setPiece(SQ_E8, BLACK_KING);
    assert(!board.validateKings());
    std::cout << "  ✓ Missing white king detected\n";
    
    // Test kings adjacent (should fail)
    board.clear();
    board.setPiece(SQ_E4, WHITE_KING);
    board.setPiece(SQ_E5, BLACK_KING);  // Adjacent to white king
    assert(!board.validateKings());
    std::cout << "  ✓ Adjacent kings detected\n";
    
    std::cout << "✓ Validation function tests passed\n";
}

// Test round-trip consistency (board → FEN → board)
void testRoundTripConsistency() {
    std::cout << "Testing round-trip consistency...\n";
    
    std::vector<std::string> testPositions = {
        // Starting position
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
        // Kiwipete
        "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1",
        // Position 4
        "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1",
        // Steven Edwards position
        "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8",
        // Endgame position
        "8/8/8/8/8/8/8/K6k w - - 50 100"
    };
    
    for (const auto& originalFen : testPositions) {
        Board board1;
        auto result1 = board1.parseFEN(originalFen);
        assert(result1.hasValue());
        
        std::string generatedFen = board1.toFEN();
        
        Board board2;
        auto result2 = board2.parseFEN(generatedFen);
        assert(result2.hasValue());
        
        // Verify boards are identical using position hash
        assert(board1.positionHash() == board2.positionHash());
        
        // Verify all validations pass
        assert(board1.validatePosition());
        assert(board1.validateBitboardSync());
        assert(board1.validateZobrist());
        
        assert(board2.validatePosition());
        assert(board2.validateBitboardSync());
        assert(board2.validateZobrist());
        
        std::cout << "  ✓ Round-trip test passed for: " << originalFen.substr(0, 50) << "...\n";
    }
    
    std::cout << "✓ Round-trip consistency tests passed\n";
}

// Test buffer overflow protection (critical security test)
void testBufferOverflowProtection() {
    std::cout << "Testing buffer overflow protection...\n";
    
    Board board;
    
    // Test rank overflow with empty squares
    auto result = board.parseFEN("9/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    assert(!result);
    assert(result.error().error == FenError::BoardOverflow);
    std::cout << "  ✓ Empty square overflow detected\n";
    
    // Test combination overflow
    result = board.parseFEN("ppp5pp/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    assert(!result);
    assert(result.error().error == FenError::BoardOverflow);
    std::cout << "  ✓ Combination overflow detected\n";
    
    // Test too many pieces in rank
    result = board.parseFEN("rrrrrrrrr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    assert(!result);
    assert(result.error().error == FenError::BoardOverflow);
    std::cout << "  ✓ Piece overflow detected\n";
    
    std::cout << "✓ Buffer overflow protection tests passed\n";
}

// Test Zobrist key rebuilding (critical for correctness)
void testZobristRebuild() {
    std::cout << "Testing Zobrist key rebuilding...\n";
    
    Board board;
    
    // Parse starting position
    auto result = board.parseFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    assert(result.hasValue());
    
    Hash originalKey = board.zobristKey();
    
    // Rebuild Zobrist key manually
    board.rebuildZobristKey();
    
    // Should match exactly
    assert(board.zobristKey() == originalKey);
    assert(board.validateZobrist());
    
    std::cout << "  ✓ Zobrist key rebuilding works correctly\n";
    
    // Test with complex position
    result = board.parseFEN("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1");
    assert(result.hasValue());
    assert(board.validateZobrist());
    
    std::cout << "  ✓ Complex position Zobrist validation passed\n";
    
    std::cout << "✓ Zobrist rebuild tests passed\n";
}

// Test position hash function (separate from Zobrist)
void testPositionHash() {
    std::cout << "Testing position hash function...\n";
    
    Board board1, board2;
    
    // Same position should have same hash
    board1.parseFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    board2.parseFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    
    assert(board1.positionHash() == board2.positionHash());
    std::cout << "  ✓ Identical positions have same hash\n";
    
    // Different positions should have different hashes
    board2.parseFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR b KQkq - 0 1");
    assert(board1.positionHash() != board2.positionHash());
    std::cout << "  ✓ Different positions have different hashes\n";
    
    std::cout << "✓ Position hash tests passed\n";
}

// Test debug display function
void testDebugDisplay() {
    std::cout << "Testing debug display function...\n";
    
    Board board;
    board.parseFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    
    std::string debug = board.debugDisplay();
    
    // Should contain key information
    assert(debug.find("=== Board State Debug ===") != std::string::npos);
    assert(debug.find("Validation Status:") != std::string::npos);
    assert(debug.find("PASS") != std::string::npos);
    
    std::cout << "  ✓ Debug display format correct\n";
    std::cout << "✓ Debug display tests passed\n";
}

int main() {
    std::cout << "Running Stage 2 Position Management Tests\n";
    std::cout << "==========================================\n\n";
    
    try {
        testResultType();
        testFenParserSafety();
        testFenErrorHandling();
        testValidationFunctions();
        testRoundTripConsistency();
        testBufferOverflowProtection();
        testZobristRebuild();
        testPositionHash();
        testDebugDisplay();
        
        std::cout << "\n🎉 ALL STAGE 2 TESTS PASSED! 🎉\n";
        std::cout << "SeaJay Stage 2 (Position Management) is ready for Stage 3.\n";
        
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "\n❌ TEST FAILED: " << e.what() << "\n";
        return 1;
    } catch (...) {
        std::cerr << "\n❌ UNKNOWN TEST FAILURE\n";
        return 1;
    }
}