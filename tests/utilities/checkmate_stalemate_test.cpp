// Checkmate and Stalemate Position Test Utility
// Purpose: Test if the engine correctly identifies checkmate and stalemate positions
// Usage: g++ -std=c++20 -I/workspace -o checkmate_test checkmate_stalemate_test.cpp /workspace/build/libseajay_core.a
//        ./checkmate_test

#include <iostream>
#include <vector>
#include <string>
#include "src/core/board.h"
#include "src/core/move_generation.h"

using namespace seajay;

struct TestPosition {
    std::string name;
    std::string fen;
    bool expectedCheckmate;
    bool expectedStalemate;
};

void testPosition(const TestPosition& test) {
    std::cout << "\n=== Testing: " << test.name << " ===" << std::endl;
    std::cout << "FEN: " << test.fen << std::endl;
    
    Board board;
    if (!board.fromFEN(test.fen)) {
        std::cerr << "ERROR: Failed to parse FEN" << std::endl;
        return;
    }
    
    std::cout << board.toString() << std::endl;
    
    Color us = board.sideToMove();
    bool isInCheck = MoveGenerator::inCheck(board, us);
    MoveList moves;
    MoveGenerator::generateLegalMoves(board, moves);
    
    std::cout << "Side to move: " << (us == WHITE ? "White" : "Black") << std::endl;
    std::cout << "In check: " << (isInCheck ? "YES" : "NO") << std::endl;
    std::cout << "Legal moves: " << moves.size() << std::endl;
    
    bool isCheckmate = (moves.size() == 0 && isInCheck);
    bool isStalemate = (moves.size() == 0 && !isInCheck);
    
    std::cout << "Status: ";
    if (isCheckmate) {
        std::cout << "CHECKMATE";
    } else if (isStalemate) {
        std::cout << "STALEMATE";
    } else {
        std::cout << "PLAYABLE (" << moves.size() << " moves)";
    }
    std::cout << std::endl;
    
    // Verify against expected results
    bool passed = true;
    if (test.expectedCheckmate && !isCheckmate) {
        std::cout << "FAIL: Expected checkmate but position is not checkmate" << std::endl;
        passed = false;
    }
    if (test.expectedStalemate && !isStalemate) {
        std::cout << "FAIL: Expected stalemate but position is not stalemate" << std::endl;
        passed = false;
    }
    if (!test.expectedCheckmate && isCheckmate) {
        std::cout << "FAIL: Did not expect checkmate but position is checkmate" << std::endl;
        passed = false;
    }
    if (!test.expectedStalemate && isStalemate) {
        std::cout << "FAIL: Did not expect stalemate but position is stalemate" << std::endl;
        passed = false;
    }
    
    if (passed) {
        std::cout << "✓ Test PASSED" << std::endl;
    }
}

int main() {
    std::vector<TestPosition> testPositions = {
        // Checkmate positions
        {"Fool's Mate", "rnb1kbnr/pppp1ppp/8/4p3/6Pq/5P2/PPPPP2P/RNBQKBNR w KQkq - 0 1", true, false},
        {"Back Rank Mate", "6rk/5ppp/8/8/8/8/5PPP/6RK b - - 0 1", true, false},
        {"Smothered Mate", "6rk/5p1p/7N/8/8/8/5PPP/6K1 b - - 0 1", true, false},
        
        // Stalemate positions
        {"Classic Stalemate", "7k/5Q2/5K2/8/8/8/8/8 b - - 0 1", false, true},
        {"King vs King+Pawn", "8/8/8/8/8/1k6/p7/K7 w - - 0 1", false, true},
        
        // Playable positions (neither checkmate nor stalemate)
        {"Starting Position", "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", false, false},
        {"Middle Game", "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1", false, false}
    };
    
    std::cout << "=== Checkmate and Stalemate Test Suite ===" << std::endl;
    std::cout << "Testing " << testPositions.size() << " positions..." << std::endl;
    
    int passed = 0;
    int failed = 0;
    
    for (const auto& test : testPositions) {
        testPosition(test);
        
        // Count results
        Board board;
        board.fromFEN(test.fen);
        Color us = board.sideToMove();
        bool isInCheck = MoveGenerator::inCheck(board, us);
        MoveList moves;
        MoveGenerator::generateLegalMoves(board, moves);
        
        bool isCheckmate = (moves.size() == 0 && isInCheck);
        bool isStalemate = (moves.size() == 0 && !isInCheck);
        
        if ((test.expectedCheckmate == isCheckmate) && (test.expectedStalemate == isStalemate)) {
            passed++;
        } else {
            failed++;
        }
    }
    
    std::cout << "\n=== Final Results ===" << std::endl;
    std::cout << "Tests Passed: " << passed << std::endl;
    std::cout << "Tests Failed: " << failed << std::endl;
    std::cout << "Total Tests: " << (passed + failed) << std::endl;
    
    if (failed == 0) {
        std::cout << "✓ All tests PASSED!" << std::endl;
        return 0;
    } else {
        std::cout << "✗ Some tests FAILED" << std::endl;
        return 1;
    }
}