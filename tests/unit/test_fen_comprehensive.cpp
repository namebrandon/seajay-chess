#include "../../src/core/board.h"
#include "../../src/core/bitboard.h"
#include <iostream>
#include <cassert>
#include <vector>
#include <string>

using namespace seajay;

// Test structures for organized testing
struct FENTestCase {
    std::string name;
    std::string fen;
    bool shouldPass;
    std::string expectedError = "";
};

struct RoundTripTestCase {
    std::string name;
    std::string fen;
};

class FENTester {
public:
    void runAllTests() {
        std::cout << "\n=== Running Comprehensive FEN Tests ===\n\n";
        
        testValidFENs();
        testInvalidFENs();
        testEdgeCases();
        testRoundTrip();
        testSpecialPositions();
        testBoundaryConditions();
        testPieceCountValidation();
        testKingValidation();
        testCastlingValidation();
        testEnPassantValidation();
        testClockValidation();
        
        std::cout << "\n✅ All FEN tests completed!\n";
        std::cout << "✅ Passed: " << passedTests << "\n";
        std::cout << "❌ Failed: " << failedTests << "\n\n";
        
        if (failedTests > 0) {
            std::cerr << "⚠️  Some tests failed - FEN implementation needs fixes!\n";
            exit(1);
        }
    }

private:
    int passedTests = 0;
    int failedTests = 0;
    
    void testCase(const FENTestCase& test) {
        Board board;
        bool result = board.fromFEN(test.fen);
        
        if (result == test.shouldPass) {
            std::cout << "✓ " << test.name << "\n";
            passedTests++;
        } else {
            std::cout << "❌ " << test.name << " - Expected " 
                      << (test.shouldPass ? "pass" : "fail") 
                      << " but got " << (result ? "pass" : "fail") << "\n";
            std::cout << "   FEN: " << test.fen << "\n";
            failedTests++;
        }
    }
    
    void testRoundTripCase(const RoundTripTestCase& test) {
        Board board;
        if (!board.fromFEN(test.fen)) {
            std::cout << "❌ " << test.name << " - Failed to parse FEN\n";
            failedTests++;
            return;
        }
        
        std::string regenerated = board.toFEN();
        if (regenerated == test.fen) {
            std::cout << "✓ " << test.name << " - Round trip\n";
            passedTests++;
        } else {
            std::cout << "❌ " << test.name << " - Round trip failed\n";
            std::cout << "   Original:    " << test.fen << "\n";
            std::cout << "   Regenerated: " << regenerated << "\n";
            failedTests++;
        }
    }
    
    void testValidFENs() {
        std::cout << "--- Testing Valid FEN Strings ---\n";
        
        std::vector<FENTestCase> validTests = {
            {"Starting position", "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", true},
            {"After 1.e4", "rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq e3 0 1", true},
            {"After 1.e4 c5", "rnbqkbnr/pp1ppppp/8/2p5/4P3/8/PPPP1PPP/RNBQKBNR w KQkq c6 0 2", true},
            {"Italian Game", "r1bqkbnr/pppp1ppp/2n5/4p3/2B1P3/5N2/PPPP1PPP/RNBQK2R b KQkq - 3 3", true},
            {"Ruy Lopez", "r1bqkbnr/pppp1ppp/2n5/1B2p3/4P3/5N2/PPPP1PPP/RNBQK2R b KQkq - 3 3", true},
            {"No castling rights", "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w - - 0 1", true},
            {"White kingside only", "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w K - 0 1", true},
            {"Black queenside only", "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w q - 0 1", true},
            {"High move numbers", "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 50 100", true},
            {"En passant on a-file", "rnbqkbnr/1ppppppp/8/p7/P7/8/1PPPPPPP/RNBQKBNR w KQkq a6 0 2", true},
            {"En passant on h-file", "rnbqkbnr/ppppppp1/8/7p/7P/8/PPPPPPP1/RNBQKBNR w KQkq h6 0 2", true}
        };
        
        for (const auto& test : validTests) {
            testCase(test);
        }
    }
    
    void testInvalidFENs() {
        std::cout << "\n--- Testing Invalid FEN Strings ---\n";
        
        std::vector<FENTestCase> invalidTests = {
            {"Empty string", "", false},
            {"Missing fields", "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR", false},
            {"Too few fields", "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq", false},
            {"Invalid piece character", "rnbqkbnr/ppppxppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", false},
            {"Too many ranks", "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR/8 w KQkq - 0 1", false},
            {"Too few ranks", "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP w KQkq - 0 1", false},
            {"Rank too long", "rnbqkbnrr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", false},
            {"Rank too short", "rnbqkbn/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", false},
            {"Invalid number in rank", "rnbqkbnr/pppp9pp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", false},
            {"Invalid side to move", "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR x KQkq - 0 1", false},
            {"Invalid castling rights", "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQX - 0 1", false},
            {"Invalid en passant square", "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq z9 0 1", false},
            {"En passant on wrong rank", "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq e4 0 1", false},
            {"Negative halfmove clock", "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - -1 1", false},
            {"Negative fullmove number", "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 -1", false},
            {"Zero fullmove number", "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 0", false},
            {"Non-numeric halfmove clock", "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - abc 1", false},
            {"Non-numeric fullmove number", "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 xyz", false}
        };
        
        for (const auto& test : invalidTests) {
            testCase(test);
        }
    }
    
    void testEdgeCases() {
        std::cout << "\n--- Testing Edge Cases ---\n";
        
        std::vector<FENTestCase> edgeTests = {
            {"Empty board", "8/8/8/8/8/8/8/8 w - - 0 1", false}, // No kings
            {"Only white king", "8/8/8/8/8/8/8/K7 w - - 0 1", false}, // Missing black king
            {"Only black king", "8/8/8/8/8/8/8/k7 w - - 0 1", false}, // Missing white king
            {"Both kings", "k7/8/8/8/8/8/8/K7 w - - 0 1", true}, // Minimal valid position
            {"Kings only, black to move", "k7/8/8/8/8/8/8/K7 b - - 0 1", true},
            {"All pieces", "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", true},
            {"Maximum pieces legal", "rnbqkb1r/pppppppp/5n2/8/8/5N2/PPPPPPPP/RNBQKB1R w KQkq - 0 1", true}
        };
        
        for (const auto& test : edgeTests) {
            testCase(test);
        }
    }
    
    void testRoundTrip() {
        std::cout << "\n--- Testing Round Trip (FEN -> Board -> FEN) ---\n";
        
        std::vector<RoundTripTestCase> roundTripTests = {
            {"Starting position", "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"},
            {"Italian Game", "r1bqkbnr/pppp1ppp/2n5/4p3/2B1P3/5N2/PPPP1PPP/RNBQK2R b KQkq - 3 3"},
            {"En passant position", "rnbqkbnr/ppp1p1pp/8/3pPp2/8/8/PPPP1PPP/RNBQKBNR w KQkq f6 0 3"},
            {"No castling rights", "r3k2r/8/8/8/8/8/8/R3K2R w - - 0 1"},
            {"Partial castling rights", "r3k2r/8/8/8/8/8/8/R3K2R w Kq - 0 1"},
            {"High move numbers", "k7/8/8/8/8/8/8/K7 w - - 99 200"},
            {"Complex position", "r2q1rk1/pP1p2pp/Q4n2/bbp1p3/Np6/1B3NBn/pPPP1PPP/R3K2R b KQ - 3 9"}
        };
        
        for (const auto& test : roundTripTests) {
            testRoundTripCase(test);
        }
    }
    
    void testSpecialPositions() {
        std::cout << "\n--- Testing Special Chess Positions ---\n";
        
        std::vector<RoundTripTestCase> specialTests = {
            {"Valid promotion position", "8/8/8/8/8/8/8/k6K w - - 0 1"}, // Just kings for valid test
            {"Castling test position", "r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1"},
            {"En passant test position", "rnbqkbnr/ppp1p1pp/8/3pPp2/8/8/PPPP1PPP/RNBQKBNR w KQkq f6 0 3"},
            {"Stalemate position", "8/8/8/8/8/8/8/k6K b - - 0 1"},
            {"Zugzwang position", "8/8/1p6/8/1P6/8/8/k6K w - - 0 1"},
            {"Endgame position", "8/2k5/8/8/8/8/2K5/8 w - - 50 100"}
        };
        
        for (const auto& test : specialTests) {
            testRoundTripCase(test);
        }
    }
    
    void testBoundaryConditions() {
        std::cout << "\n--- Testing Boundary Conditions ---\n";
        
        std::vector<FENTestCase> boundaryTests = {
            {"Corner squares a1", "8/8/8/8/8/8/8/K6k w - - 0 1", true},
            {"Corner squares h1", "8/8/8/8/8/8/8/k6K w - - 0 1", true},
            {"Corner squares a8", "K6k/8/8/8/8/8/8/8 w - - 0 1", true},
            {"Corner squares h8", "k6K/8/8/8/8/8/8/8 w - - 0 1", true},
            {"Maximum halfmove clock", "k7/8/8/8/8/8/8/K7 w - - 100 1", true},
            {"Large fullmove number", "k7/8/8/8/8/8/8/K7 w - - 0 9999", true}
        };
        
        for (const auto& test : boundaryTests) {
            testCase(test);
        }
    }
    
    void testPieceCountValidation() {
        std::cout << "\n--- Testing Piece Count Validation ---\n";
        
        std::vector<FENTestCase> pieceCountTests = {
            {"Too many white pawns", "8/PPPPPPPPP/8/8/8/8/8/k6K w - - 0 1", false},
            {"Too many black pawns", "k6K/8/8/8/8/8/ppppppppp/8 w - - 0 1", false},
            {"Multiple white kings", "K6K/8/8/8/8/8/8/k7 w - - 0 1", false},
            {"Multiple black kings", "k6k/8/8/8/8/8/8/K7 w - - 0 1", false},
            {"Too many white queens", "QQQQQQQQQ/8/8/8/8/8/8/k6K w - - 0 1", false},
            {"Too many pieces total", "rnbqkbnr/pppppppp/PPPPPPPP/RNBQKBNR/rnbqkbnr/pppppppp/PPPPPPPP/RNBQKBNR w - - 0 1", false},
            {"Pawns on back ranks", "Pkkkkkk1/8/8/8/8/8/8/1KKKKKK1 w - - 0 1", false},
            {"Valid maximum pieces", "rnbqkb1r/pppppppp/5n2/8/8/5N2/PPPPPPPP/RNBQKB1R w KQkq - 0 1", true}
        };
        
        for (const auto& test : pieceCountTests) {
            testCase(test);
        }
    }
    
    void testKingValidation() {
        std::cout << "\n--- Testing King Validation ---\n";
        
        std::vector<FENTestCase> kingTests = {
            {"No white king", "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQ1BNR w KQkq - 0 1", false},
            {"No black king", "rnbq1bnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", false},
            {"Both kings present", "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", true},
            {"Kings adjacent (illegal)", "8/8/8/8/8/8/8/kK6 w - - 0 1", false},
            {"Kings one square apart", "8/8/8/8/8/8/8/k1K5 w - - 0 1", true}
        };
        
        for (const auto& test : kingTests) {
            testCase(test);
        }
    }
    
    void testCastlingValidation() {
        std::cout << "\n--- Testing Castling Rights Validation ---\n";
        
        std::vector<FENTestCase> castlingTests = {
            {"Valid all castling rights", "r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1", true},
            {"No castling rights valid", "r3k2r/8/8/8/8/8/8/R3K2R w - - 0 1", true},
            {"White king moved, no white castling", "r3k2r/8/8/8/8/8/8/R2K3R w kq - 0 1", true},
            {"Black king moved, no black castling", "r2k3r/8/8/8/8/8/8/R3K2R w KQ - 0 1", true},
            {"Rook moved, partial castling", "r3k3/8/8/8/8/8/8/R3K2R w KQq - 0 1", true},
            {"Invalid castling with missing pieces", "8/8/8/8/8/8/8/4K3 w KQ - 0 1", false}
        };
        
        for (const auto& test : castlingTests) {
            testCase(test);
        }
    }
    
    void testEnPassantValidation() {
        std::cout << "\n--- Testing En Passant Validation ---\n";
        
        std::vector<FENTestCase> enPassantTests = {
            {"Valid en passant white", "rnbqkbnr/ppp1p1pp/8/3pPp2/8/8/PPPP1PPP/RNBQKBNR w KQkq f6 0 3", true},
            {"Valid en passant black", "rnbqkbnr/pppp1ppp/8/8/3Pp3/8/PPP1PPPP/RNBQKBNR b KQkq d3 0 2", true},
            {"En passant on wrong rank (white)", "rnbqkbnr/ppp1pppp/8/3p4/4P3/8/PPPP1PPP/RNBQKBNR b KQkq e4 0 2", false},
            {"En passant on wrong rank (black)", "rnbqkbnr/pppp1ppp/8/8/4P3/8/PPP1PPPP/RNBQKBNR w KQkq e5 0 1", false},
            {"En passant without double move setup", "rnbqkbnr/pppp1ppp/8/8/8/4P3/PPPP1PPP/RNBQKBNR b KQkq e3 0 1", false},
            {"No en passant", "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", true}
        };
        
        for (const auto& test : enPassantTests) {
            testCase(test);
        }
    }
    
    void testClockValidation() {
        std::cout << "\n--- Testing Clock Validation ---\n";
        
        std::vector<FENTestCase> clockTests = {
            {"Normal clocks", "k7/8/8/8/8/8/8/K7 w - - 0 1", true},
            {"High halfmove clock", "k7/8/8/8/8/8/8/K7 w - - 50 1", true},
            {"Max halfmove clock", "k7/8/8/8/8/8/8/K7 w - - 100 1", true},
            {"Over max halfmove clock", "k7/8/8/8/8/8/8/K7 w - - 101 1", false},
            {"High fullmove number", "k7/8/8/8/8/8/8/K7 w - - 0 999", true},
            {"Zero fullmove number", "k7/8/8/8/8/8/8/K7 w - - 0 0", false},
            {"Negative halfmove (should be caught in parsing)", "k7/8/8/8/8/8/8/K7 w - - -1 1", false}
        };
        
        for (const auto& test : clockTests) {
            testCase(test);
        }
    }
};

int main() {
    FENTester tester;
    tester.runAllTests();
    return 0;
}
