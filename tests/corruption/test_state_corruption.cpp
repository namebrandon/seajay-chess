// SeaJay Chess Engine - State Corruption Detection Tests
// This test suite verifies that our make/unmake implementation
// properly maintains state integrity and prevents corruption

#include "../../src/core/board.h"
#include "../../src/core/board_safety.h"
#include "../../src/core/move_generation.h"
#include <iostream>
#include <vector>
#include <cassert>
#include <iomanip>

using namespace seajay;

class StateCorruptionTester {
private:
    static bool s_verbose;
    
    static void log(const std::string& msg) {
        if (s_verbose) {
            std::cout << "  " << msg << "\n";
        }
    }
    
    static void error(const std::string& msg) {
        std::cerr << "  ❌ ERROR: " << msg << "\n";
    }
    
public:
    static void setVerbose(bool v) { s_verbose = v; }
    
    // Test 1: Basic Make/Unmake Reversibility
    static bool testBasicReversibility() {
        std::cout << "\n[1] Testing basic make/unmake reversibility...\n";
        
        Board board;
        board.setStartingPosition();
        
        // Save initial state
        Hash initialZobrist = board.zobristKey();
        uint8_t initialCastling = board.castlingRights();
        Square initialEP = board.enPassantSquare();
        uint16_t initialHalfmove = board.halfmoveClock();
        
        // Test simple pawn move
        Move move = makeMove(E2, E4, DOUBLE_PAWN);
        Board::UndoInfo undo;
        
        board.makeMove(move, undo);
        
        // State should have changed
        if (board.zobristKey() == initialZobrist) {
            error("Zobrist key unchanged after move!");
            return false;
        }
        
        if (board.enPassantSquare() != E3) {
            error("En passant square not set after double pawn move!");
            return false;
        }
        
        // Unmake the move
        board.unmakeMove(move, undo);
        
        // Everything should be restored
        if (board.zobristKey() != initialZobrist) {
            error("Zobrist key not restored!");
            std::cerr << "    Expected: 0x" << std::hex << initialZobrist << "\n";
            std::cerr << "    Got:      0x" << board.zobristKey() << std::dec << "\n";
            return false;
        }
        
        if (board.castlingRights() != initialCastling ||
            board.enPassantSquare() != initialEP ||
            board.halfmoveClock() != initialHalfmove) {
            error("Game state not fully restored!");
            return false;
        }
        
        log("✓ Basic reversibility passed");
        return true;
    }
    
    // Test 2: Deep Make/Unmake Sequences
    static bool testDeepSequences() {
        std::cout << "\n[2] Testing deep make/unmake sequences...\n";
        
        Board board;
        board.setStartingPosition();
        
        // Save initial state for comparison
        BoardStateValidator::StateSnapshot initial(board);
        
        // Italian Game opening sequence
        std::vector<Move> moves = {
            makeMove(E2, E4, DOUBLE_PAWN),
            makeMove(E7, E5, DOUBLE_PAWN),
            makeMove(G1, F3),
            makeMove(B8, C6),
            makeMove(F1, C4),
            makeMove(F8, C5),
            makeMove(C2, C3),
            makeMove(G8, F6),
            makeMove(D2, D4, DOUBLE_PAWN),
            makeMove(E5, D4)  // Capture
        };
        
        std::vector<Board::UndoInfo> undos;
        
        // Make all moves
        for (size_t i = 0; i < moves.size(); ++i) {
            Board::UndoInfo undo;
            board.makeMove(moves[i], undo);
            undos.push_back(undo);
            
            // Validate after each move
            if (!board.validateZobrist()) {
                error("Zobrist inconsistent at move " + std::to_string(i + 1));
                return false;
            }
            
            if (!board.validateBitboardSync()) {
                error("Bitboard desync at move " + std::to_string(i + 1));
                return false;
            }
        }
        
        // Unmake all moves in reverse order
        for (int i = moves.size() - 1; i >= 0; --i) {
            board.unmakeMove(moves[i], undos[i]);
            
            if (!board.validateZobrist()) {
                error("Zobrist inconsistent during unmake at move " + std::to_string(i));
                return false;
            }
        }
        
        // Verify complete restoration
        BoardStateValidator::StateSnapshot final(board);
        if (!(initial == final)) {
            error("State not fully restored after deep sequence!");
            std::cerr << initial.compareWith(final) << "\n";
            return false;
        }
        
        log("✓ Deep sequences passed");
        return true;
    }
    
    // Test 3: Castling State Corruption
    static bool testCastlingCorruption() {
        std::cout << "\n[3] Testing castling state preservation...\n";
        
        Board board;
        board.fromFEN("r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1");
        
        Hash initialZobrist = board.zobristKey();
        uint8_t initialRights = board.castlingRights();
        
        // Test 1: King move loses castling rights
        {
            Move kingMove = makeMove(E1, F1);
            Board::UndoInfo undo;
            
            board.makeMove(kingMove, undo);
            
            // White should lose both castling rights
            if (board.castlingRights() & (WHITE_KINGSIDE | WHITE_QUEENSIDE)) {
                error("White castling rights not removed after king move!");
                return false;
            }
            
            board.unmakeMove(kingMove, undo);
            
            // Rights should be restored
            if (board.castlingRights() != initialRights) {
                error("Castling rights not restored!");
                return false;
            }
        }
        
        // Test 2: Actual castling move
        {
            Move castle = makeCastlingMove(E1, G1);
            Board::UndoInfo undo;
            
            board.makeMove(castle, undo);
            
            // Verify pieces moved correctly
            if (board.pieceAt(G1) != WHITE_KING || board.pieceAt(F1) != WHITE_ROOK) {
                error("Castling pieces not in correct positions!");
                return false;
            }
            
            // Verify zobrist changed
            if (board.zobristKey() == initialZobrist) {
                error("Zobrist unchanged after castling!");
                return false;
            }
            
            board.unmakeMove(castle, undo);
            
            // Everything should be back
            if (board.pieceAt(E1) != WHITE_KING || board.pieceAt(H1) != WHITE_ROOK) {
                error("Castling not properly unmade!");
                return false;
            }
            
            if (board.zobristKey() != initialZobrist) {
                error("Zobrist not restored after castling unmake!");
                return false;
            }
        }
        
        log("✓ Castling corruption prevention passed");
        return true;
    }
    
    // Test 4: En Passant State Corruption
    static bool testEnPassantCorruption() {
        std::cout << "\n[4] Testing en passant state preservation...\n";
        
        Board board;
        
        // Set up en passant position
        board.fromFEN("rnbqkbnr/1pp1pppp/8/p2pP3/8/8/PPPP1PPP/RNBQKBNR w KQkq d6 0 3");
        
        Hash initialZobrist = board.zobristKey();
        
        // Make en passant capture
        Move epCapture = makeEnPassantMove(E5, D6);
        Board::UndoInfo undo;
        
        board.makeMove(epCapture, undo);
        
        // Verify capture occurred
        if (board.pieceAt(D6) != WHITE_PAWN) {
            error("Pawn not at destination after en passant!");
            return false;
        }
        
        if (board.pieceAt(D5) != NO_PIECE) {
            error("Captured pawn not removed in en passant!");
            return false;
        }
        
        if (board.pieceAt(E5) != NO_PIECE) {
            error("Original pawn not removed from source!");
            return false;
        }
        
        // Unmake the capture
        board.unmakeMove(epCapture, undo);
        
        // Verify restoration
        if (board.pieceAt(E5) != WHITE_PAWN) {
            error("White pawn not restored after en passant unmake!");
            return false;
        }
        
        if (board.pieceAt(D5) != BLACK_PAWN) {
            error("Black pawn not restored after en passant unmake!");
            return false;
        }
        
        if (board.pieceAt(D6) != NO_PIECE) {
            error("Destination square not cleared after en passant unmake!");
            return false;
        }
        
        if (board.zobristKey() != initialZobrist) {
            error("Zobrist not restored after en passant unmake!");
            return false;
        }
        
        log("✓ En passant corruption prevention passed");
        return true;
    }
    
    // Test 5: Promotion State Corruption
    static bool testPromotionCorruption() {
        std::cout << "\n[5] Testing promotion state preservation...\n";
        
        Board board;
        
        // Test simple promotion
        {
            board.fromFEN("8/P7/8/8/8/8/8/8 w - - 0 1");
            Hash initialZobrist = board.zobristKey();
            
            Move promo = makePromotionMove(A7, A8, QUEEN);
            Board::UndoInfo undo;
            
            board.makeMove(promo, undo);
            
            if (board.pieceAt(A8) != WHITE_QUEEN) {
                error("Queen not placed after promotion!");
                return false;
            }
            
            board.unmakeMove(promo, undo);
            
            if (board.pieceAt(A7) != WHITE_PAWN || board.pieceAt(A8) != NO_PIECE) {
                error("Promotion not properly unmade!");
                return false;
            }
            
            if (board.zobristKey() != initialZobrist) {
                error("Zobrist not restored after promotion unmake!");
                return false;
            }
        }
        
        // Test promotion with capture
        {
            board.fromFEN("r7/P7/8/8/8/8/8/8 w - - 0 1");
            Hash initialZobrist = board.zobristKey();
            
            Move promoCap = makePromotionCaptureMove(A7, A8, QUEEN);
            Board::UndoInfo undo;
            
            board.makeMove(promoCap, undo);
            
            if (board.pieceAt(A8) != WHITE_QUEEN) {
                error("Queen not placed after promotion capture!");
                return false;
            }
            
            board.unmakeMove(promoCap, undo);
            
            if (board.pieceAt(A7) != WHITE_PAWN || board.pieceAt(A8) != BLACK_ROOK) {
                error("Promotion capture not properly unmade!");
                return false;
            }
            
            if (board.zobristKey() != initialZobrist) {
                error("Zobrist not restored after promotion capture unmake!");
                return false;
            }
        }
        
        log("✓ Promotion corruption prevention passed");
        return true;
    }
    
    // Test 6: Complex Game Sequence
    static bool testComplexGameSequence() {
        std::cout << "\n[6] Testing complex game sequence...\n";
        
        Board board;
        board.setStartingPosition();
        
        // Scholar's mate attempt and defense
        std::vector<Move> moves = {
            makeMove(E2, E4, DOUBLE_PAWN),
            makeMove(E7, E5, DOUBLE_PAWN),
            makeMove(F1, C4),
            makeMove(B8, C6),
            makeMove(D1, H5),
            makeMove(G7, G6),
            makeMove(H5, F3),
            makeMove(G8, F6),
            makeMove(F3, F7)  // Check!
        };
        
        std::vector<Board::UndoInfo> undos;
        Hash zobristHistory[10];
        
        // Make moves and track zobrist keys
        for (size_t i = 0; i < moves.size(); ++i) {
            zobristHistory[i] = board.zobristKey();
            
            Board::UndoInfo undo;
            board.makeMove(moves[i], undo);
            undos.push_back(undo);
            
            // Ensure zobrist changes each move
            if (board.zobristKey() == zobristHistory[i]) {
                error("Zobrist unchanged at move " + std::to_string(i + 1));
                return false;
            }
        }
        
        // Unmake and verify zobrist restoration
        for (int i = moves.size() - 1; i >= 0; --i) {
            board.unmakeMove(moves[i], undos[i]);
            
            if (board.zobristKey() != zobristHistory[i]) {
                error("Zobrist not restored at position " + std::to_string(i));
                return false;
            }
        }
        
        log("✓ Complex game sequence passed");
        return true;
    }
    
    // Test 7: Halfmove Clock Tracking
    static bool testHalfmoveClock() {
        std::cout << "\n[7] Testing halfmove clock preservation...\n";
        
        Board board;
        board.fromFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 5 10");
        
        uint16_t initialHalfmove = board.halfmoveClock();
        uint16_t initialFullmove = board.fullmoveNumber();
        
        // Non-pawn, non-capture move (increments halfmove)
        Move knightMove = makeMove(G1, F3);
        Board::UndoInfo undo1;
        
        board.makeMove(knightMove, undo1);
        
        if (board.halfmoveClock() != initialHalfmove + 1) {
            error("Halfmove clock not incremented!");
            return false;
        }
        
        board.unmakeMove(knightMove, undo1);
        
        if (board.halfmoveClock() != initialHalfmove) {
            error("Halfmove clock not restored!");
            return false;
        }
        
        // Pawn move (resets halfmove)
        Move pawnMove = makeMove(E2, E4, DOUBLE_PAWN);
        Board::UndoInfo undo2;
        
        board.makeMove(pawnMove, undo2);
        
        if (board.halfmoveClock() != 0) {
            error("Halfmove clock not reset on pawn move!");
            return false;
        }
        
        board.unmakeMove(pawnMove, undo2);
        
        if (board.halfmoveClock() != initialHalfmove) {
            error("Halfmove clock not restored after pawn move!");
            return false;
        }
        
        if (board.fullmoveNumber() != initialFullmove) {
            error("Fullmove number changed incorrectly!");
            return false;
        }
        
        log("✓ Halfmove clock tracking passed");
        return true;
    }
    
    // Main test runner
    static bool runAllTests(bool verbose = false) {
        setVerbose(verbose);
        
        std::cout << "\n" << std::string(60, '=') << "\n";
        std::cout << "STATE CORRUPTION DETECTION TEST SUITE\n";
        std::cout << std::string(60, '=') << "\n";
        
        int passed = 0;
        int total = 0;
        
        // Run all tests
        std::vector<std::pair<std::string, bool(*)()>> tests = {
            {"Basic Reversibility", testBasicReversibility},
            {"Deep Sequences", testDeepSequences},
            {"Castling Corruption", testCastlingCorruption},
            {"En Passant Corruption", testEnPassantCorruption},
            {"Promotion Corruption", testPromotionCorruption},
            {"Complex Game", testComplexGameSequence},
            {"Halfmove Clock", testHalfmoveClock}
        };
        
        for (const auto& [name, testFunc] : tests) {
            total++;
            if (testFunc()) {
                passed++;
                std::cout << "  ✅ " << name << " PASSED\n";
            } else {
                std::cout << "  ❌ " << name << " FAILED\n";
            }
        }
        
        std::cout << "\n" << std::string(60, '-') << "\n";
        std::cout << "Results: " << passed << "/" << total << " tests passed\n";
        
        if (passed == total) {
            std::cout << "✅ ALL STATE CORRUPTION TESTS PASSED!\n";
            std::cout << "   Make/unmake implementation is robust against corruption.\n";
        } else {
            std::cout << "❌ SOME TESTS FAILED!\n";
            std::cout << "   State corruption vulnerabilities detected.\n";
        }
        std::cout << std::string(60, '-') << "\n\n";
        
        return passed == total;
    }
};

bool StateCorruptionTester::s_verbose = false;

// Main entry point
int main(int argc, char* argv[]) {
    bool verbose = (argc > 1 && std::string(argv[1]) == "-v");
    
    try {
        return StateCorruptionTester::runAllTests(verbose) ? 0 : 1;
    } catch (const std::exception& e) {
        std::cerr << "\n💥 Test crashed: " << e.what() << "\n";
        return 2;
    }
}