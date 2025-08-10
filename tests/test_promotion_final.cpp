// FINAL CORRECTED Test Suite - Promotion Move Validation
// This version has all test expectations properly corrected

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

int main() {
    std::vector<TestCase> tests = {
        // Test 1: Pawn blocked by enemy piece straight ahead
        {"r3k3/P7/8/8/8/8/8/4K3 w - - 0 1", 
         "Pawn a7 blocked by rook a8", 5, false,
         "Pawn cannot move (blocked) or capture (no diagonal enemies). King: 5 moves"},
        
        // Test 2: Pawn can capture diagonally (with castling rights affecting king)
        {"rnbqkbnr/P7/8/8/8/8/8/4K3 w kq - 0 1", 
         "Pawn a7 with full black back rank", 7, true,
         "Pawn captures b8 knight (4 promos). King: 3 moves (kq rights block d1/d2)"},
        
        // Test 3: Black pawn blocked
        {"4k3/8/8/8/8/8/p7/R3K3 b - - 0 1", 
         "Black pawn a2 blocked by white rook a1", 5, false,
         "Black pawn blocked, cannot capture. Black king: 5 moves"},
        
        // Test 4: Pawn blocked by knight on a8
        {"n3k3/P7/8/8/8/8/8/4K3 w - - 0 1", 
         "Pawn a7 blocked by knight a8", 5, false,
         "Knight on a8 blocks forward, b8 empty (no capture). King: 5 moves"},
        
        // Test 5: Pawn can capture diagonal AND move forward
        {"b3k3/1P6/8/8/8/8/8/4K3 w - - 0 1", 
         "Pawn b7 with bishop on a8", 13, true,
         "Capture a8 bishop (4) + move to b8 (4) = 8 promos. King: 5 moves"},
        
        // Test 6: Simple forward promotion
        {"4k3/P7/8/8/8/8/8/4K3 w - - 0 1", 
         "Pawn a7 with a8 empty", 9, true,
         "Move to a8 (4 promos). King: 5 moves"},
        
        // Test 7: Another forward promotion
        {"4k3/1P6/8/8/8/8/8/4K3 w - - 0 1", 
         "Pawn b7 with b8 empty", 9, true,
         "Move to b8 (4 promos). King: 5 moves"},
        
        // Test 8: Pawn blocked by king
        {"4k3/4P3/8/8/8/8/8/4K3 w - - 0 1", 
         "Pawn e7 blocked by king e8", 5, false,
         "King on e8 blocks pawn. White king: 5 moves"},
        
        // Test 9: Pawn can only capture diagonally
        {"rn2k3/P7/8/8/8/8/8/4K3 w - - 0 1", 
         "Pawn a7, rook a8, knight b8", 9, true,
         "Capture b8 knight only (4 promos). Cannot capture a8 (not diagonal). King: 5"},
        
        // Test 10: Pawn can move forward AND capture
        {"1r2k3/P7/8/8/8/8/8/4K3 w - - 0 1", 
         "Pawn a7, empty a8, rook b8", 13, true,
         "Move to a8 (4) + capture b8 (4) = 8 promos. King: 5 moves"}
    };
    
    std::cout << "====================================\n";
    std::cout << "FINAL PROMOTION TEST VALIDATION\n";
    std::cout << "====================================\n\n";
    
    int testNum = 0;
    int passed = 0;
    int failed = 0;
    
    for (const auto& test : tests) {
        testNum++;
        std::cout << "Test #" << testNum << ": " << test.description << "\n";
        std::cout << "FEN: " << test.fen << "\n";
        
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
        
        std::cout << "Expected: " << test.expectedMoveCount << " moves\n";
        std::cout << "Got:      " << moves.size() << " moves\n";
        
        bool correct = (moves.size() == test.expectedMoveCount) && 
                      (hasPromotions == test.shouldHavePromotions);
        
        if (correct) {
            std::cout << "Result:   ✓ PASS\n";
            passed++;
        } else {
            std::cout << "Result:   ✗ FAIL\n";
            std::cout << "Explanation: " << test.explanation << "\n";
            failed++;
        }
        
        std::cout << "--------------------------------------------------\n";
    }
    
    std::cout << "\n====================================\n";
    std::cout << "FINAL RESULTS\n";
    std::cout << "====================================\n";
    std::cout << "Total Tests: " << testNum << "\n";
    std::cout << "Passed:      " << passed << "\n";
    std::cout << "Failed:      " << failed << "\n\n";
    
    if (failed == 0) {
        std::cout << "✓ SUCCESS: SeaJay's promotion generation is CORRECT!\n";
        std::cout << "\nCONCLUSION:\n";
        std::cout << "There was NO bug in the engine. The original test expectations\n";
        std::cout << "were incorrect due to misunderstanding pawn capture rules:\n";
        std::cout << "- Pawns move FORWARD\n";
        std::cout << "- Pawns capture DIAGONALLY\n";
        std::cout << "- Pawns CANNOT capture straight ahead\n";
    } else {
        std::cout << "✗ Some tests still failing - investigation needed\n";
    }
    
    return failed;
}