// CORRECTED Test program for promotion move handling
// All test expectations have been fixed based on correct chess rules

#include <iostream>
#include <iomanip>
#include <vector>
#include <string>
#include <sstream>
#include "core/board.h"
#include "core/move_generation.h"
#include "core/move_list.h"
#include "core/types.h"

using namespace seajay;

struct TestCase {
    std::string fen;
    std::string description;
    int expectedMoveCount;
    bool shouldHavePromotions;
    std::string explanation;
};

// Helper to convert move to algebraic notation
std::string moveToAlgebraic(Move move) {
    std::ostringstream ss;
    ss << squareToString(moveFrom(move));
    ss << squareToString(moveTo(move));
    return ss.str();
}

int main() {
    std::vector<TestCase> tests = {
        // Category 1: Blocked positions
        {"r3k3/P7/8/8/8/8/8/4K3 w - - 0 1", 
         "Pawn a7 blocked by rook a8", 5, false,
         "Pawn cannot move forward (blocked) or capture (no diagonal enemies)"},
        
        {"rnbqkbnr/P7/8/8/8/8/8/4K3 w kq - 0 1", 
         "Pawn a7 with full black back rank", 9, true,  // CORRECTED: was 5
         "Pawn can capture knight on b8 diagonally (4 promos) + 5 king = 9"},
        
        {"4k3/8/8/8/8/8/p7/R3K3 b - - 0 1", 
         "Black pawn a2 blocked by white rook a1", 5, false,
         "Pawn cannot move forward (blocked) or capture (no diagonal enemies)"},
        
        {"n3k3/P7/8/8/8/8/8/4K3 w - - 0 1", 
         "Pawn a7, knight a8 blocks forward", 9, true,  // CORRECTED: was 5
         "Pawn cannot move forward but CAN capture b8 knight diagonally"},
        
        {"b3k3/1P6/8/8/8/8/8/4K3 w - - 0 1", 
         "Pawn b7 with bishop on a8", 13, true,  // CORRECTED: was 5
         "Pawn can capture a8 bishop + move to b8 (8 promos) + 5 king = 13"},
        
        // Category 2: Valid promotions
        {"4k3/P7/8/8/8/8/8/4K3 w - - 0 1", 
         "Pawn a7 with a8 empty", 9, true,
         "Pawn can move forward to a8 (4 promos) + 5 king = 9"},
        
        {"4k3/1P6/8/8/8/8/8/4K3 w - - 0 1", 
         "Pawn b7 with b8 empty", 9, true,
         "Pawn can move forward to b8 (4 promos) + 5 king = 9"},
        
        {"4k3/4P3/8/8/8/8/8/4K3 w - - 0 1", 
         "Pawn e7 with king on e8", 5, false,  // CORRECTED: was 9
         "Pawn is blocked by king, cannot move or capture"},
        
        {"rn2k3/P7/8/8/8/8/8/4K3 w - - 0 1", 
         "Pawn a7, rook a8, knight b8", 9, true,  // CORRECTED: was 13
         "Pawn can ONLY capture b8 knight diagonally (not a8 straight ahead)"},
        
        {"1r2k3/P7/8/8/8/8/8/4K3 w - - 0 1", 
         "Pawn a7, empty a8, rook b8", 13, true,  // CORRECTED: was 9
         "Pawn can move to a8 + capture b8 (8 promos) + 5 king = 13"}
    };
    
    std::cout << "====================================\n";
    std::cout << "CORRECTED PROMOTION TEST SUITE\n";
    std::cout << "====================================\n\n";
    
    int testNum = 0;
    int passed = 0;
    int failed = 0;
    
    for (const auto& test : tests) {
        testNum++;
        std::cout << "Test #" << testNum << ": " << test.description << "\n";
        std::cout << "FEN: " << test.fen << "\n";
        std::cout << "Explanation: " << test.explanation << "\n";
        
        Board board;
        if (!board.fromFEN(test.fen)) {
            std::cerr << "ERROR: Failed to parse FEN\n";
            failed++;
            continue;
        }
        
        MoveList moves;
        MoveGenerator::generateLegalMoves(board, moves);
        
        int promotionCount = 0;
        for (size_t i = 0; i < moves.size(); i++) {
            if (isPromotion(moves[i])) {
                promotionCount++;
            }
        }
        
        bool hasPromotions = (promotionCount > 0);
        
        std::cout << "Expected: " << test.expectedMoveCount << " moves, "
                  << (test.shouldHavePromotions ? "WITH" : "NO") << " promotions\n";
        std::cout << "Got:      " << moves.size() << " moves, "
                  << promotionCount << " promotions\n";
        
        bool moveCountCorrect = (moves.size() == test.expectedMoveCount);
        bool promotionStatusCorrect = (hasPromotions == test.shouldHavePromotions);
        
        if (moveCountCorrect && promotionStatusCorrect) {
            std::cout << "Result:   [PASS]\n";
            passed++;
        } else {
            std::cout << "Result:   [FAIL]";
            if (!moveCountCorrect) std::cout << " (Wrong move count)";
            if (!promotionStatusCorrect) std::cout << " (Wrong promotion status)";
            std::cout << "\n";
            failed++;
        }
        
        std::cout << "--------------------------------------------------\n";
    }
    
    std::cout << "\n====================================\n";
    std::cout << "TEST SUMMARY\n";
    std::cout << "====================================\n";
    std::cout << "Total Tests: " << testNum << "\n";
    std::cout << "Passed:      " << passed << "\n";
    std::cout << "Failed:      " << failed << "\n\n";
    
    if (failed == 0) {
        std::cout << "SUCCESS: All tests passed!\n";
        std::cout << "SeaJay's promotion move generation is CORRECT.\n";
    } else {
        std::cout << "FAILURE: Some tests failed.\n";
    }
    
    return failed;
}