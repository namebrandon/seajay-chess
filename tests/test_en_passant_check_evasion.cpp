/**
 * Test for en passant move generation in check evasion scenarios
 * 
 * This test ensures that en passant captures are correctly generated
 * when they can evade check by:
 * 1. Blocking a sliding piece check
 * 2. Capturing the checking piece (if it's the pawn that just moved)
 * 
 * Bug history: White en passant captures were not being generated
 * in check evasion scenarios (fixed in commit after Stage 3).
 */

#include <iostream>
#include <cassert>
#include "core/board.h"
#include "core/move_generation.h"
#include "core/move_list.h"

using namespace seajay;

void testEnPassantCheckEvasion() {
    struct TestCase {
        const char* fen;
        const char* description;
        int expectedLegalMoves;
        bool shouldHaveEnPassant;
    };
    
    TestCase tests[] = {
        // Critical bug fix cases - white en passant blocking check
        {
            "8/8/8/1Ppp3r/1K3p1k/8/4P1P1/1R6 w - c6 0 1",
            "White king in check from rook, b5xc6 en passant blocks",
            7,
            true
        },
        {
            "8/8/8/1PpP3r/1K3p1k/8/6P1/1R6 w - c6 0 1",
            "White king in check, two en passant captures block",
            9,
            true
        },
        
        // Black en passant (was working correctly)
        {
            "8/8/3k4/8/1pPp4/8/1K6/8 b - c3 0 1",
            "Black has two en passant captures, not in check",
            11,
            true
        },
        
        // Additional test cases
        {
            "rnbqkbnr/ppp1p1pp/8/3pPp2/8/8/PPPP1PPP/RNBQKBNR w KQkq f6 0 3",
            "Normal white en passant, not in check",
            31,
            true
        },
        {
            "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
            "Starting position, no en passant",
            20,
            false
        }
    };
    
    std::cout << "Testing en passant in check evasion scenarios...\n\n";
    
    bool allPassed = true;
    
    for (const auto& test : tests) {
        Board board;
        if (!board.fromFEN(test.fen)) {
            std::cerr << "Failed to parse FEN: " << test.fen << "\n";
            allPassed = false;
            continue;
        }
        
        MoveList moves;
        MoveGenerator::generateLegalMoves(board, moves);
        
        // Count en passant moves
        int epCount = 0;
        for (Move move : moves) {
            if (isEnPassant(move)) {
                epCount++;
            }
        }
        
        bool hasEnPassant = (epCount > 0);
        bool moveCountCorrect = (moves.size() == test.expectedLegalMoves);
        bool epCorrect = (hasEnPassant == test.shouldHaveEnPassant);
        
        if (moveCountCorrect && epCorrect) {
            std::cout << "✅ PASS: " << test.description << "\n";
            std::cout << "         " << moves.size() << " legal moves";
            if (hasEnPassant) {
                std::cout << " (" << epCount << " en passant)";
            }
            std::cout << "\n";
        } else {
            std::cout << "❌ FAIL: " << test.description << "\n";
            std::cout << "         Expected: " << test.expectedLegalMoves << " moves, ";
            std::cout << (test.shouldHaveEnPassant ? "with" : "no") << " en passant\n";
            std::cout << "         Got: " << moves.size() << " moves, ";
            std::cout << epCount << " en passant\n";
            allPassed = false;
        }
        std::cout << "\n";
    }
    
    if (!allPassed) {
        std::cerr << "\n❌ En passant check evasion test FAILED!\n";
        assert(false);
    }
    
    std::cout << "✅ All en passant check evasion tests passed!\n";
}

int main() {
    try {
        testEnPassantCheckEvasion();
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Test failed with exception: " << e.what() << "\n";
        return 1;
    }
}