#include <iostream>
#include <string>
#include <vector>
#include "../src/core/board.h"
#include "../src/core/move_generation.h"
#include "../src/core/move_list.h"
#include "../src/core/types.h"

using namespace seajay;

struct TestPosition {
    std::string fen;
    std::string description;
    std::vector<std::pair<Square, Square>> illegalKingMoves;  // King moves that should NOT be generated
    std::vector<std::pair<Square, Square>> legalKingMoves;    // King moves that SHOULD be generated
};

bool testPosition(const TestPosition& test) {
    Board board;
    if (!board.fromFEN(test.fen)) {
        std::cout << "ERROR: Failed to parse FEN: " << test.fen << std::endl;
        return false;
    }
    
    std::cout << "\nTesting: " << test.description << std::endl;
    std::cout << "FEN: " << test.fen << std::endl;
    std::cout << board.toString() << std::endl;
    
    MoveList moves;
    MoveGenerator::generateLegalMoves(board, moves);
    
    // Check that illegal king moves are NOT generated
    for (const auto& [from, to] : test.illegalKingMoves) {
        Move illegalMove = makeMove(from, to, NORMAL);
        bool found = false;
        for (size_t i = 0; i < moves.size(); ++i) {
            if (moveFrom(moves[i]) == from && moveTo(moves[i]) == to) {
                found = true;
                break;
            }
        }
        if (found) {
            std::cout << "ERROR: Generated illegal king move " 
                     << squareToString(from) << squareToString(to) << std::endl;
            return false;
        }
    }
    
    // Check that legal king moves ARE generated
    for (const auto& [from, to] : test.legalKingMoves) {
        bool found = false;
        for (size_t i = 0; i < moves.size(); ++i) {
            if (moveFrom(moves[i]) == from && moveTo(moves[i]) == to) {
                found = true;
                break;
            }
        }
        if (!found) {
            std::cout << "ERROR: Did not generate legal king move " 
                     << squareToString(from) << squareToString(to) << std::endl;
            return false;
        }
    }
    
    std::cout << "PASSED - Generated " << moves.size() << " legal moves" << std::endl;
    return true;
}

int main() {
    std::vector<TestPosition> tests = {
        {
            "k7/8/8/8/8/8/8/R6K b - - 0 1",
            "Black king in check from rook on a-file",
            {{A8, A7}, {A8, A6}},  // These moves along the file are illegal (rook still attacks)
            {{A8, B8}, {A8, B7}}   // These moves away from the file are legal
        },
        {
            "8/8/8/8/8/2k5/8/B6K b - - 0 1",
            "Black king in check from bishop on diagonal",
            {{C3, B2}, {C3, D4}},  // These moves along the diagonal are illegal
            {{C3, B3}, {C3, C2}, {C3, D3}, {C3, C4}, {C3, B4}, {C3, D2}}  // These are legal
        },
        {
            "3qk3/8/8/8/8/8/8/4K3 w - - 0 1",
            "White king in check from queen",
            {{E1, D1}, {E1, E2}},  // These moves are blocked by king itself
            {{E1, F2}, {E1, F1}}   // These are legal escapes
        },
        {
            "8/8/8/8/3k4/8/1K6/3Q4 b - - 0 1",
            "Black king in check from queen on d-file",
            {{D4, D3}, {D4, D5}},  // Along file - illegal
            {{D4, C3}, {D4, E3}, {D4, C4}, {D4, E4}, {D4, C5}, {D4, E5}}  // Other moves - legal
        },
        {
            "r7/8/8/4k3/8/8/8/4K3 b - - 0 1",
            "Black king with rook check from a8 (no blocking issue)",
            {},  // No illegal moves in this case
            {{E5, D4}, {E5, D5}, {E5, D6}, {E5, E4}, {E5, E6}, {E5, F4}, {E5, F5}, {E5, F6}}  // All king moves should be legal
        }
    };
    
    std::cout << "Testing King Evasion Bug Fix" << std::endl;
    std::cout << "=============================" << std::endl;
    
    int passed = 0;
    int failed = 0;
    
    for (const auto& test : tests) {
        if (testPosition(test)) {
            passed++;
        } else {
            failed++;
        }
    }
    
    std::cout << "\n=============================" << std::endl;
    std::cout << "Results: " << passed << " passed, " << failed << " failed" << std::endl;
    
    if (failed > 0) {
        std::cout << "\nBug fix verification FAILED!" << std::endl;
    } else {
        std::cout << "\nBug fix verification PASSED!" << std::endl;
    }
    
    return failed > 0 ? 1 : 0;
}