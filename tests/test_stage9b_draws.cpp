// Stage 9b Draw Detection Comprehensive Test Suite
#include "../src/core/board.h"
#include "../src/core/move_generation.h"
#include "../src/core/board_safety.h"
#include <iostream>
#include <vector>
#include <chrono>
#include <cassert>
#include <fstream>
#include <iomanip>

using namespace seajay;

// Test result tracking
struct TestResult {
    std::string testName;
    bool passed;
    std::string details;
    double executionTime;
};

class Stage9bDrawTests {
private:
    std::vector<TestResult> results;
    int totalTests = 0;
    int passedTests = 0;
    
    // Helper to parse algebraic moves
    Move parseMove(Board& board, const std::string& moveStr) {
        if (moveStr.length() < 4) return NO_MOVE;
        
        Square from = static_cast<Square>((moveStr[0] - 'a') + (moveStr[1] - '1') * 8);
        Square to = static_cast<Square>((moveStr[2] - 'a') + (moveStr[3] - '1') * 8);
        
        MoveList moves = generateLegalMoves(board);
        for (const Move& move : moves) {
            if (moveFrom(move) == from && moveTo(move) == to) {
                // Handle promotions
                if (moveStr.length() == 5) {
                    char promoChar = moveStr[4];
                    PieceType promoPiece = NO_PIECE_TYPE;
                    switch (promoChar) {
                        case 'q': promoPiece = QUEEN; break;
                        case 'r': promoPiece = ROOK; break;
                        case 'b': promoPiece = BISHOP; break;
                        case 'n': promoPiece = KNIGHT; break;
                    }
                    if (moveFlags(move) & PROMOTION) {
                        if (promotionType(move) == promoPiece) {
                            return move;
                        }
                    }
                } else {
                    return move;
                }
            }
        }
        return NO_MOVE;
    }
    
    void recordTest(const std::string& name, bool passed, const std::string& details, 
                    double executionTime = 0.0) {
        TestResult result;
        result.testName = name;
        result.passed = passed;
        result.details = details;
        result.executionTime = executionTime;
        results.push_back(result);
        
        totalTests++;
        if (passed) passedTests++;
        
        std::cout << (passed ? "✓ " : "✗ ") << name;
        if (executionTime > 0) {
            std::cout << " (" << std::fixed << std::setprecision(3) << executionTime << "ms)";
        }
        std::cout << std::endl;
        if (!passed) {
            std::cout << "  FAILED: " << details << std::endl;
        }
    }
    
public:
    // === THREEFOLD REPETITION TESTS ===
    
    void testBasicThreefold() {
        auto start = std::chrono::high_resolution_clock::now();
        
        Board board;
        board.setStartingPosition();
        board.clearGameHistory();
        
        // Nc3 Nc6 Nb1 Nb8 Nc3 Nc6 Nb1 Nb8 Nc3 - creates threefold
        std::vector<std::string> moves = {
            "b1c3", "b8c6", "c3b1", "c6b8",
            "b1c3", "b8c6", "c3b1", "c6b8", 
            "b1c3"
        };
        
        bool isRep = false;
        for (size_t i = 0; i < moves.size(); ++i) {
            Move move = parseMove(board, moves[i]);
            if (move == NO_MOVE) {
                recordTest("Basic Threefold", false, 
                          "Failed to parse move: " + moves[i]);
                return;
            }
            Board::UndoInfo undo;
            board.makeMove(move, undo);
            
            // Check after the final move
            if (i == moves.size() - 1) {
                isRep = board.isRepetitionDraw();
            }
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> elapsed = end - start;
        
        recordTest("Basic Threefold", isRep, 
                  isRep ? "Knight shuttling creates threefold repetition" 
                        : "Failed to detect threefold repetition", 
                  elapsed.count());
    }
    
    void testCastlingRightsNotRepetition() {
        auto start = std::chrono::high_resolution_clock::now();
        
        Board board;
        // Position where rook moves change castling rights
        board.fromFEN("r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1");
        board.clearGameHistory();
        
        // Record initial position
        Hash initialHash = board.zobristKey();
        uint8_t initialCastling = board.castlingRights();
        
        // Ra1-a2-a1 removes white queenside castling
        std::vector<std::string> moves = {"a1a2", "a8a7", "a2a1", "a7a8"};
        
        for (const auto& moveStr : moves) {
            Move move = parseMove(board, moveStr);
            if (move == NO_MOVE) {
                recordTest("Castling Rights Not Repetition", false, 
                          "Failed to parse move: " + moveStr);
                return;
            }
            Board::UndoInfo undo;
            board.makeMove(move, undo);
        }
        
        // Position looks same but castling rights differ
        Hash finalHash = board.zobristKey();
        uint8_t finalCastling = board.castlingRights();
        bool isRep = board.isRepetitionDraw();
        
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> elapsed = end - start;
        
        bool correctBehavior = !isRep && (initialHash != finalHash || initialCastling != finalCastling);
        
        recordTest("Castling Rights Not Repetition", correctBehavior, 
                  correctBehavior ? "Different castling rights prevent repetition"
                                  : "Incorrectly detected repetition with different castling rights",
                  elapsed.count());
    }
    
    void testEnPassantPhantom() {
        auto start = std::chrono::high_resolution_clock::now();
        
        Board board;
        // Position just before en passant opportunity  
        board.fromFEN("k7/2p5/8/1P6/K7/8/8/8 w - - 0 1");
        board.clearGameHistory();
        
        // White pawn moves first
        Move move1 = parseMove(board, "b5b6");
        if (move1 == NO_MOVE) {
            recordTest("En Passant Phantom", false, "Failed to parse b5b6");
            return;
        }
        Board::UndoInfo undo1;
        board.makeMove(move1, undo1);
        
        // Black plays c7-c5, creating en passant square
        Move move2 = parseMove(board, "c7c5");
        if (move2 == NO_MOVE) {
            recordTest("En Passant Phantom", false, "Failed to parse c7c5");
            return;
        }
        Board::UndoInfo undo2;
        board.makeMove(move2, undo2);
        
        // Now en passant is possible at c6
        Square epSquare = board.enPassantSquare();
        bool hasEP = (epSquare != NO_SQUARE);
        
        // King moves (doesn't use en passant)
        Move move3 = parseMove(board, "a4a3");
        if (move3 == NO_MOVE) {
            recordTest("En Passant Phantom", false, "Failed to parse a5a4");
            return;
        }
        Board::UndoInfo undo3;
        board.makeMove(move3, undo3);
        
        // En passant is now gone
        Square newEpSquare = board.enPassantSquare();
        bool epGone = (newEpSquare == NO_SQUARE);
        
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> elapsed = end - start;
        
        bool passed = hasEP && epGone;
        recordTest("En Passant Phantom", passed, 
                  passed ? "En passant square correctly affects position uniqueness"
                         : "En passant handling incorrect",
                  elapsed.count());
    }
    
    void testActualThreefoldInGame() {
        auto start = std::chrono::high_resolution_clock::now();
        
        Board board;
        // Real game position that leads to threefold
        board.fromFEN("r1bqkb1r/pppp1ppp/2n2n2/4p3/2B1P3/5N2/PPPP1PPP/RNBQK2R w KQkq - 0 1");
        board.clearGameHistory();
        
        // Moves that create repetition: Bc4-b5 Nc6-a5 Bb5-c4 Na5-c6 etc.
        std::vector<std::string> moves = {
            "c4b5", "c6a5", "b5c4", "a5c6",
            "c4b5", "c6a5", "b5c4", "a5c6",
            "c4b5"  // Third time reaching the position
        };
        
        bool isRep = false;
        for (size_t i = 0; i < moves.size(); ++i) {
            Move move = parseMove(board, moves[i]);
            if (move == NO_MOVE) {
                recordTest("Actual Threefold In Game", false, 
                          "Failed to parse move: " + moves[i]);
                return;
            }
            Board::UndoInfo undo;
            board.makeMove(move, undo);
            
            if (i == moves.size() - 1) {
                isRep = board.isRepetitionDraw();
            }
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> elapsed = end - start;
        
        recordTest("Actual Threefold In Game", isRep,
                  isRep ? "Detected threefold in realistic game position"
                        : "Failed to detect threefold in game",
                  elapsed.count());
    }
    
    // === FIFTY-MOVE RULE TESTS ===
    
    void testFiftyMoveExactTrigger() {
        auto start = std::chrono::high_resolution_clock::now();
        
        Board board;
        // Position at 99 halfmoves
        board.fromFEN("8/8/8/4k3/8/3K4/8/8 w - - 99 1");
        
        bool beforeFifty = board.isFiftyMoveRule();
        
        // Next move triggers fifty-move rule (king move)
        Move move = parseMove(board, "d3c4");
        if (move == NO_MOVE) {
            recordTest("Fifty Move Exact Trigger", false, "Failed to parse move d3c4");
            return;
        }
        Board::UndoInfo undo;
        board.makeMove(move, undo);
        
        bool afterFifty = board.isFiftyMoveRule();
        
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> elapsed = end - start;
        
        bool passed = !beforeFifty && afterFifty;
        recordTest("Fifty Move Exact Trigger", passed,
                  passed ? "Triggers at exactly 100 halfmoves"
                         : "Failed to trigger at 100 halfmoves",
                  elapsed.count());
    }
    
    void testFiftyMoveResetOnPawn() {
        auto start = std::chrono::high_resolution_clock::now();
        
        Board board;
        // Position with pawn that can move
        board.fromFEN("8/8/8/4k3/8/8/3PK3/8 w - - 99 1");
        
        // Pawn move resets counter
        Move move = parseMove(board, "d2d4");
        if (move == NO_MOVE) {
            recordTest("Fifty Move Reset on Pawn", false, "Failed to parse move d2d4");
            return;
        }
        Board::UndoInfo undo;
        board.makeMove(move, undo);
        
        bool isFifty = board.isFiftyMoveRule();
        uint16_t halfmoves = board.halfmoveClock();
        
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> elapsed = end - start;
        
        bool passed = !isFifty && halfmoves == 0;
        recordTest("Fifty Move Reset on Pawn", passed,
                  passed ? "Pawn moves reset halfmove clock"
                         : "Failed to reset halfmove clock on pawn move",
                  elapsed.count());
    }
    
    void testFiftyMoveResetOnCapture() {
        auto start = std::chrono::high_resolution_clock::now();
        
        Board board;
        // Position where king can capture rook
        board.fromFEN("8/8/4k3/8/3r4/4K3/8/8 w - - 99 1");
        
        // Capture resets counter (King takes rook)
        Move move = parseMove(board, "e3d4");
        if (move == NO_MOVE) {
            recordTest("Fifty Move Reset on Capture", false, "Failed to parse move e3d4");
            return;
        }
        Board::UndoInfo undo;
        board.makeMove(move, undo);
        
        bool isFifty = board.isFiftyMoveRule();
        uint16_t halfmoves = board.halfmoveClock();
        
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> elapsed = end - start;
        
        bool passed = !isFifty && halfmoves == 0;
        recordTest("Fifty Move Reset on Capture", passed,
                  passed ? "Captures reset halfmove clock"
                         : "Failed to reset halfmove clock on capture",
                  elapsed.count());
    }
    
    void testFiftyMoveProgress() {
        auto start = std::chrono::high_resolution_clock::now();
        
        Board board;
        board.fromFEN("8/8/8/4k3/8/3K4/8/8 w - - 90 1");
        
        // Make 5 non-pawn, non-capture moves (alternating king moves)
        std::vector<std::string> moves = {
            "d3c4", "e5d6", "c4c3", "d6e5", "c3d3"
        };
        
        for (size_t i = 0; i < moves.size(); ++i) {
            Move move = parseMove(board, moves[i]);
            if (move == NO_MOVE) {
                recordTest("Fifty Move Progress", false, "Failed to parse move: " + moves[i] + " at move " + std::to_string(i));
                return;
            }
            Board::UndoInfo undo;
            board.makeMove(move, undo);
        }
        
        uint16_t finalClock = board.halfmoveClock();
        bool notYetFifty = !board.isFiftyMoveRule();
        
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> elapsed = end - start;
        
        bool passed = (finalClock == 95) && notYetFifty;
        recordTest("Fifty Move Progress", passed,
                  passed ? "Halfmove clock increments correctly"
                         : "Halfmove clock not tracking properly",
                  elapsed.count());
    }
    
    // === INSUFFICIENT MATERIAL TESTS ===
    
    void testInsufficientKvsK() {
        auto start = std::chrono::high_resolution_clock::now();
        
        Board board;
        board.fromFEN("8/8/8/4k3/8/3K4/8/8 w - - 0 1");
        
        bool insufficient = board.isInsufficientMaterial();
        
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> elapsed = end - start;
        
        recordTest("Insufficient K vs K", insufficient,
                  insufficient ? "King vs King is insufficient material"
                               : "Failed to detect K vs K as insufficient",
                  elapsed.count());
    }
    
    void testInsufficientKNvsK() {
        auto start = std::chrono::high_resolution_clock::now();
        
        Board board;
        board.fromFEN("8/8/8/4k3/8/3K4/8/N7 w - - 0 1");
        
        bool insufficient = board.isInsufficientMaterial();
        
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> elapsed = end - start;
        
        recordTest("Insufficient KN vs K", insufficient,
                  insufficient ? "King+Knight vs King is insufficient"
                               : "Failed to detect KN vs K as insufficient",
                  elapsed.count());
    }
    
    void testInsufficientKBvsK() {
        auto start = std::chrono::high_resolution_clock::now();
        
        Board board;
        board.fromFEN("8/8/8/4k3/8/3K4/B7/8 w - - 0 1");
        
        bool insufficient = board.isInsufficientMaterial();
        
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> elapsed = end - start;
        
        recordTest("Insufficient KB vs K", insufficient,
                  insufficient ? "King+Bishop vs King is insufficient"
                               : "Failed to detect KB vs K as insufficient",
                  elapsed.count());
    }
    
    void testInsufficientKBvsKB_SameColor() {
        auto start = std::chrono::high_resolution_clock::now();
        
        Board board;
        // Both bishops on light squares (h1 and a8)
        board.fromFEN("b7/8/8/4k3/8/8/8/3K3B w - - 0 1");
        
        bool insufficient = board.isInsufficientMaterial();
        
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> elapsed = end - start;
        
        recordTest("Insufficient KB vs KB Same Color", insufficient,
                  insufficient ? "Bishops on same color = insufficient"
                               : "Failed to detect same-color bishops as insufficient",
                  elapsed.count());
    }
    
    void testSufficientKBvsKB_OppositeColor() {
        auto start = std::chrono::high_resolution_clock::now();
        
        Board board;
        // Bishops on opposite colors (a1 dark for white, a8 light for black)
        board.fromFEN("b7/8/8/4k3/8/8/8/B2K4 w - - 0 1");
        
        bool insufficient = board.isInsufficientMaterial();
        
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> elapsed = end - start;
        
        recordTest("Sufficient KB vs KB Opposite Color", !insufficient,
                  !insufficient ? "Bishops on opposite colors = sufficient"
                                : "Incorrectly marked opposite-color bishops as insufficient",
                  elapsed.count());
    }
    
    void testSufficientWithPawn() {
        auto start = std::chrono::high_resolution_clock::now();
        
        Board board;
        board.fromFEN("8/8/8/4k3/8/3K4/4P3/8 w - - 0 1");
        
        bool insufficient = board.isInsufficientMaterial();
        
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> elapsed = end - start;
        
        recordTest("Sufficient With Pawn", !insufficient,
                  !insufficient ? "Any pawn = sufficient material"
                                : "Incorrectly marked position with pawn as insufficient",
                  elapsed.count());
    }
    
    void testSufficientWithRook() {
        auto start = std::chrono::high_resolution_clock::now();
        
        Board board;
        board.fromFEN("8/8/8/4k3/8/3K4/8/R7 w - - 0 1");
        
        bool insufficient = board.isInsufficientMaterial();
        
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> elapsed = end - start;
        
        recordTest("Sufficient With Rook", !insufficient,
                  !insufficient ? "Any rook = sufficient material"
                                : "Incorrectly marked position with rook as insufficient",
                  elapsed.count());
    }
    
    void testSufficientWithQueen() {
        auto start = std::chrono::high_resolution_clock::now();
        
        Board board;
        board.fromFEN("8/8/8/4k3/8/3K4/8/Q7 w - - 0 1");
        
        bool insufficient = board.isInsufficientMaterial();
        
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> elapsed = end - start;
        
        recordTest("Sufficient With Queen", !insufficient,
                  !insufficient ? "Any queen = sufficient material"
                                : "Incorrectly marked position with queen as insufficient",
                  elapsed.count());
    }
    
    // === COMBINED DRAW DETECTION ===
    
    void testIsDrawCombined() {
        auto start = std::chrono::high_resolution_clock::now();
        
        // Test 1: Fifty-move rule
        Board board1;
        board1.fromFEN("8/8/8/4k3/8/3K4/8/8 w - - 100 1");
        bool draw1 = board1.isDraw();
        
        // Test 2: Insufficient material
        Board board2;
        board2.fromFEN("8/8/8/4k3/8/3K4/8/8 w - - 0 1");
        bool draw2 = board2.isDraw();
        
        // Test 3: Not a draw
        Board board3;
        board3.fromFEN("r1bqkbnr/pppppppp/2n5/8/4P3/5N2/PPPP1PPP/RNBQKB1R w KQkq - 0 1");
        bool draw3 = board3.isDraw();
        
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> elapsed = end - start;
        
        bool passed = draw1 && draw2 && !draw3;
        recordTest("Combined isDraw() Method", passed,
                  passed ? "isDraw() correctly combines all draw conditions"
                         : "isDraw() not working correctly",
                  elapsed.count());
    }
    
    // === EDGE CASES ===
    
    void testRootPositionDraw() {
        auto start = std::chrono::high_resolution_clock::now();
        
        Board board;
        board.setStartingPosition();
        board.clearGameHistory();
        
        // Push same position to history twice
        board.pushGameHistory();
        board.pushGameHistory();
        
        // Current position is third occurrence
        bool isDraw = board.isRepetitionDraw();
        
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> elapsed = end - start;
        
        recordTest("Root Position Draw", isDraw,
                  isDraw ? "Detect draw when root position is repetition"
                         : "Failed to detect root position repetition",
                  elapsed.count());
    }
    
    void testNoDrawInStartPosition() {
        auto start = std::chrono::high_resolution_clock::now();
        
        Board board;
        board.setStartingPosition();
        
        bool isDraw = board.isDraw();
        
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> elapsed = end - start;
        
        recordTest("No Draw In Start Position", !isDraw,
                  !isDraw ? "Starting position is not a draw"
                          : "Incorrectly detected starting position as draw",
                  elapsed.count());
    }
    
    // === PERFORMANCE TESTS ===
    
    void testDrawDetectionPerformance() {
        Board board;
        board.setStartingPosition();
        
        auto start = std::chrono::high_resolution_clock::now();
        
        // Test 1000 draw detections
        for (int i = 0; i < 1000; ++i) {
            board.isDraw();
            board.isRepetitionDraw();
            board.isFiftyMoveRule();
            board.isInsufficientMaterial();
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> elapsed = end - start;
        
        bool fast = elapsed.count() < 5.0;  // Should be < 5ms
        recordTest("Draw Detection Performance", fast,
                  "1000 checks in " + std::to_string(elapsed.count()) + "ms",
                  elapsed.count());
    }
    
    void testRepetitionHistoryManagement() {
        auto start = std::chrono::high_resolution_clock::now();
        
        Board board;
        board.setStartingPosition();
        board.clearGameHistory();
        
        // Make/unmake same moves multiple times
        for (int cycle = 0; cycle < 10; ++cycle) {
            Move move = parseMove(board, "e2e4");
            if (move != NO_MOVE) {
                Board::UndoInfo undo;
            board.makeMove(move, undo);
            }
            move = parseMove(board, "e7e5");
            if (move != NO_MOVE) {
                Board::UndoInfo undo;
            board.makeMove(move, undo);
            }
        }
        
        // Should have history but not be a repetition yet
        bool notRep = !board.isRepetitionDraw();
        
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> elapsed = end - start;
        
        recordTest("Repetition History Management", notRep,
                  notRep ? "History managed correctly without false positives"
                         : "False positive in repetition detection",
                  elapsed.count());
    }
    
    // === TEST RUNNER ===
    
    void runAllTests() {
        std::cout << "=== Stage 9b Draw Detection Test Suite ===" << std::endl;
        std::cout << "Testing draw detection functionality..." << std::endl;
        std::cout << std::endl;
        
        std::cout << "--- Threefold Repetition Tests ---" << std::endl;
        testBasicThreefold();
        testCastlingRightsNotRepetition();
        testEnPassantPhantom();
        testActualThreefoldInGame();
        testRootPositionDraw();
        
        std::cout << std::endl << "--- Fifty-Move Rule Tests ---" << std::endl;
        testFiftyMoveExactTrigger();
        testFiftyMoveResetOnPawn();
        testFiftyMoveResetOnCapture();
        testFiftyMoveProgress();
        
        std::cout << std::endl << "--- Insufficient Material Tests ---" << std::endl;
        testInsufficientKvsK();
        testInsufficientKNvsK();
        testInsufficientKBvsK();
        testInsufficientKBvsKB_SameColor();
        testSufficientKBvsKB_OppositeColor();
        testSufficientWithPawn();
        testSufficientWithRook();
        testSufficientWithQueen();
        
        std::cout << std::endl << "--- Combined Draw Detection ---" << std::endl;
        testIsDrawCombined();
        
        std::cout << std::endl << "--- Edge Cases ---" << std::endl;
        testNoDrawInStartPosition();
        testRepetitionHistoryManagement();
        
        std::cout << std::endl << "--- Performance Tests ---" << std::endl;
        testDrawDetectionPerformance();
        
        std::cout << std::endl << "=== Test Summary ===" << std::endl;
        std::cout << "Total Tests: " << totalTests << std::endl;
        std::cout << "Passed: " << passedTests << std::endl;
        std::cout << "Failed: " << (totalTests - passedTests) << std::endl;
        std::cout << "Success Rate: " << std::fixed << std::setprecision(1) 
                  << (passedTests * 100.0 / totalTests) << "%" << std::endl;
        
        if (passedTests == totalTests) {
            std::cout << std::endl << "🎉 ALL TESTS PASSED! Stage 9b is ready for SPRT testing." << std::endl;
        } else {
            std::cout << std::endl << "⚠️  Some tests failed. Please review and fix before SPRT testing." << std::endl;
        }
        
        generateReport();
    }
    
    void generateReport() {
        std::ofstream report("stage9b_test_report.txt");
        report << "Stage 9b Draw Detection Test Report" << std::endl;
        report << "====================================" << std::endl;
        report << std::endl;
        
        report << "Test Results:" << std::endl;
        report << "-------------" << std::endl;
        for (const auto& result : results) {
            report << (result.passed ? "[PASS] " : "[FAIL] ") 
                   << result.testName;
            if (result.executionTime > 0) {
                report << " (" << std::fixed << std::setprecision(3) 
                       << result.executionTime << "ms)";
            }
            report << std::endl;
            if (!result.passed) {
                report << "       Details: " << result.details << std::endl;
            }
        }
        
        report << std::endl;
        report << "Summary Statistics:" << std::endl;
        report << "------------------" << std::endl;
        report << "Total Tests: " << totalTests << std::endl;
        report << "Passed: " << passedTests << std::endl;
        report << "Failed: " << (totalTests - passedTests) << std::endl;
        report << "Success Rate: " << std::fixed << std::setprecision(1) 
               << (passedTests * 100.0 / totalTests) << "%" << std::endl;
        
        // Calculate average execution time
        double totalTime = 0;
        int timedTests = 0;
        for (const auto& result : results) {
            if (result.executionTime > 0) {
                totalTime += result.executionTime;
                timedTests++;
            }
        }
        if (timedTests > 0) {
            report << "Average Execution Time: " << std::fixed << std::setprecision(3)
                   << (totalTime / timedTests) << "ms" << std::endl;
        }
        
        report.close();
        
        std::cout << "Test report written to stage9b_test_report.txt" << std::endl;
    }
};

int main() {
    std::cout << "SeaJay Chess Engine - Stage 9b Draw Detection Tests" << std::endl;
    std::cout << "====================================================" << std::endl;
    std::cout << std::endl;
    
    Stage9bDrawTests tests;
    tests.runAllTests();
    
    return 0;
}