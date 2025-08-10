// Test program for Bug #003: Promotion Move Handling
// Compile: g++ -std=c++20 -I../src test_promotion_bug.cpp ../build/libseajay_core.a -o test_promotion_bug

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
    std::string expectedMoves;  // For specific move validation
};

// Helper to convert move to algebraic notation
std::string moveToAlgebraic(Move move) {
    std::ostringstream ss;
    ss << squareToString(moveFrom(move));
    ss << squareToString(moveTo(move));
    
    // Add promotion piece if applicable
    if (moveFlags(move) & PROMOTION) {
        PieceType promo = promotionType(move);
        switch (promo) {
            case QUEEN:  ss << "q"; break;
            case ROOK:   ss << "r"; break;
            case BISHOP: ss << "b"; break;
            case KNIGHT: ss << "n"; break;
            default: break;
        }
    }
    return ss.str();
}

void printBitboard(Bitboard bb, const std::string& label) {
    std::cout << label << ":\n";
    for (int rank = 7; rank >= 0; rank--) {
        std::cout << (rank + 1) << " ";
        for (int file = 0; file < 8; file++) {
            Square sq = static_cast<Square>(rank * 8 + file);
            std::cout << ((bb & squareBB(sq)) ? "1 " : ". ");
        }
        std::cout << "\n";
    }
    std::cout << "  a b c d e f g h\n";
    std::cout << "  Hex: 0x" << std::hex << bb << std::dec << "\n\n";
}

void debugPosition(const Board& board, const std::string& fen) {
    std::cout << "\n=== POSITION DEBUG ===\n";
    std::cout << "FEN: " << fen << "\n\n";
    
    // Print the board
    std::cout << board.toString() << "\n";
    
    // Print bitboards
    printBitboard(board.occupied(), "Occupied squares");
    printBitboard(board.pieces(WHITE, PAWN), "White pawns");
    printBitboard(board.pieces(BLACK), "Black pieces");
    
    // Check specific squares
    Square a7 = static_cast<Square>(48);  // a7
    Square a8 = static_cast<Square>(56);  // a8
    
    std::cout << "Square a7 (index " << a7 << "): " 
              << (board.pieceAt(a7) != NO_PIECE ? "Occupied" : "Empty") << "\n";
    std::cout << "Square a8 (index " << a8 << "): " 
              << (board.pieceAt(a8) != NO_PIECE ? "Occupied" : "Empty") << "\n";
    
    Bitboard a8_bit = squareBB(a8);
    bool a8_occupied = (board.occupied() & a8_bit) != 0;
    std::cout << "a8 in occupied bitboard: " << (a8_occupied ? "YES" : "NO") << "\n";
    std::cout << "=====================\n\n";
}

int main() {
    std::vector<TestCase> tests = {
        // Category 1: Blocked Promotion Positions (NO promotions should be generated)
        {"r3k3/P7/8/8/8/8/8/4K3 w - - 0 1", 
         "BUG #003: White pawn a7 blocked by black rook a8", 5, false, ""},
        
        {"rnbqkbnr/P7/8/8/8/8/8/4K3 w kq - 0 1", 
         "White pawn a7 with full black back rank", 7, true, "Pawn captures b8 knight, king limited by castling"},
        
        {"4k3/8/8/8/8/8/p7/R3K3 b - - 0 1", 
         "Black pawn a2 blocked by white rook a1", 5, false, ""},
        
        {"n3k3/P7/8/8/8/8/8/4K3 w - - 0 1", 
         "White pawn a7 blocked by black knight a8", 5, false, "Knight on a8 blocks forward, b8 empty"},
        
        {"b3k3/1P6/8/8/8/8/8/4K3 w - - 0 1", 
         "White pawn b7 with black bishop on a8", 13, true, "Can capture a8 + move to b8"},
        
        // Category 2: Valid Promotion Positions (promotions SHOULD be generated)
        {"4k3/P7/8/8/8/8/8/4K3 w - - 0 1", 
         "White pawn a7 with a8 empty (valid promotion)", 9, true, ""},
        
        {"4k3/1P6/8/8/8/8/8/4K3 w - - 0 1", 
         "White pawn b7 with b8 empty (valid promotion)", 9, true, ""},
        
        {"4k3/4P3/8/8/8/8/8/4K3 w - - 0 1", 
         "White pawn e7 blocked by black king e8", 5, false, "King blocks pawn"},
        
        // Category 3: Promotion with Capture
        {"rn2k3/P7/8/8/8/8/8/4K3 w - - 0 1", 
         "White pawn a7 can only capture knight b8 diagonally", 9, true, "Cannot capture a8 (not diagonal)"},
        
        {"1r2k3/P7/8/8/8/8/8/4K3 w - - 0 1", 
         "White pawn a7 can move to a8 AND capture rook b8", 13, true, "Move forward + diagonal capture"},
    };
    
    int totalTests = tests.size();
    int passedTests = 0;
    
    std::cout << "====================================\n";
    std::cout << "PROMOTION BUG TEST SUITE\n";
    std::cout << "====================================\n\n";
    
    for (size_t testNum = 0; testNum < tests.size(); testNum++) {
        const auto& test = tests[testNum];
        
        std::cout << "Test #" << (testNum + 1) << ": " << test.description << "\n";
        std::cout << "FEN: " << test.fen << "\n";
        
        Board board;
        if (!board.fromFEN(test.fen)) {
            std::cout << "ERROR: Failed to parse FEN!\n\n";
            continue;
        }
        
        // For the bug position, show detailed debug info
        if (testNum == 0) {
            debugPosition(board, test.fen);
        }
        
        MoveList moves;
        MoveGenerator::generateLegalMoves(board, moves);
        
        // Analyze the moves
        int promotionCount = 0;
        int kingMoves = 0;
        std::vector<std::string> promotionMoves;
        std::vector<std::string> allMoves;
        
        for (size_t i = 0; i < moves.size(); i++) {
            Move move = moves[i];
            std::string moveStr = moveToAlgebraic(move);
            allMoves.push_back(moveStr);
            
            // Check if it's a promotion
            if (moveFlags(move) & PROMOTION) {
                promotionCount++;
                promotionMoves.push_back(moveStr);
            }
            
            // Check if it's a king move
            Square from = moveFrom(move);
            if (board.pieceAt(from) == makePiece(board.sideToMove(), KING)) {
                kingMoves++;
            }
        }
        
        bool hasPromotions = (promotionCount > 0);
        bool correctPromotions = (hasPromotions == test.shouldHavePromotions);
        bool correctMoveCount = (static_cast<int>(moves.size()) == test.expectedMoveCount);
        bool testPassed = correctMoveCount && correctPromotions;
        
        // Output results
        std::cout << "Expected: " << test.expectedMoveCount << " moves, "
                  << (test.shouldHavePromotions ? "WITH" : "NO") << " promotions\n";
        std::cout << "Got:      " << moves.size() << " moves, "
                  << promotionCount << " promotions\n";
        
        if (testPassed) {
            std::cout << "Result:   [PASS]\n";
            passedTests++;
        } else {
            std::cout << "Result:   [FAIL] ";
            if (!correctMoveCount) {
                std::cout << "(Wrong move count) ";
            }
            if (!correctPromotions) {
                std::cout << "(Wrong promotion status)";
            }
            std::cout << "\n";
            
            // Show all moves for failed tests
            std::cout << "\nGenerated moves (" << moves.size() << "):\n";
            for (const auto& mv : allMoves) {
                std::cout << "  " << mv;
                if (std::find(promotionMoves.begin(), promotionMoves.end(), mv) != promotionMoves.end()) {
                    std::cout << " [PROMOTION]";
                }
                std::cout << "\n";
            }
            
            if (promotionCount > 0 && !test.shouldHavePromotions) {
                std::cout << "\nERROR: Generated " << promotionCount 
                          << " promotion moves when pawn is BLOCKED!\n";
                std::cout << "Promotion moves: ";
                for (const auto& pm : promotionMoves) {
                    std::cout << pm << " ";
                }
                std::cout << "\n";
            }
        }
        
        std::cout << std::string(50, '-') << "\n\n";
    }
    
    // Summary
    std::cout << "====================================\n";
    std::cout << "TEST SUMMARY\n";
    std::cout << "====================================\n";
    std::cout << "Total Tests: " << totalTests << "\n";
    std::cout << "Passed:      " << passedTests << "\n";
    std::cout << "Failed:      " << (totalTests - passedTests) << "\n";
    
    if (passedTests == totalTests) {
        std::cout << "\nSUCCESS: All tests passed!\n";
        return 0;
    } else {
        std::cout << "\nFAILURE: Bug #003 is likely present.\n";
        std::cout << "The engine is generating promotion moves for blocked pawns.\n";
        return 1;
    }
}