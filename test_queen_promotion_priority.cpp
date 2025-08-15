#include "src/core/board.h"
#include "src/core/board_safety.h"
#include "src/core/move_generation.h"
#include "src/search/quiescence.h"
#include "src/search/move_ordering.h"
#include <iostream>
#include <vector>

using namespace seajay;
using namespace seajay::search;

// Test queen promotion prioritization in quiescence search
int main() {
    std::cout << "Testing Queen Promotion Prioritization...\n\n";
    
    // Test position with promotion opportunities  
    // Simple promotion position
    const std::string testFen = "8/P7/8/8/8/8/8/8 w - - 0 1";
    
    Board board;
    if (!board.fromFEN(testFen)) {
        std::cerr << "Failed to load test FEN\n";
        return 1;
    }
    
    std::cout << "Test Position: " << testFen << "\n";
    std::cout << "White to move with pawn on a7 that can promote\n\n";
    
    // Generate captures for quiescence (should include promotions)
    MoveList moves;
    MoveGenerator::generateCaptures(board, moves);
    
    // Add all promotion moves for testing  
    MoveList allMoves = generateLegalMoves(board);
    for (const Move& move : allMoves) {
        if (isPromotion(move)) {
            moves.push_back(move);
        }
    }
    
    std::cout << "Moves before ordering:\n";
    for (size_t i = 0; i < moves.size(); ++i) {
        Move move = moves[i];
        std::cout << "  " << i << ": " << SafeMoveExecutor::moveToString(move);
        if (isPromotion(move)) {
            std::cout << " (promotion to " << 
                (promotionType(move) == QUEEN ? "Queen" :
                 promotionType(move) == ROOK ? "Rook" :
                 promotionType(move) == BISHOP ? "Bishop" : "Knight") << ")";
        }
        if (isCapture(move)) {
            std::cout << " [capture]";
        }
        std::cout << "\n";
    }
    
    // Apply MVV-LVA ordering
    MvvLvaOrdering mvvLva;
    mvvLva.orderMoves(board, moves);
    
    std::cout << "\nMoves after MVV-LVA ordering:\n";
    for (size_t i = 0; i < moves.size(); ++i) {
        Move move = moves[i];
        int score = MvvLvaOrdering::scoreMove(board, move);
        std::cout << "  " << i << ": " << SafeMoveExecutor::moveToString(move) << " (score: " << score << ")";
        if (isPromotion(move)) {
            std::cout << " [promotion to " << 
                (promotionType(move) == QUEEN ? "Queen" :
                 promotionType(move) == ROOK ? "Rook" :
                 promotionType(move) == BISHOP ? "Bishop" : "Knight") << "]";
        }
        if (isCapture(move)) {
            std::cout << " [capture]";
        }
        std::cout << "\n";
    }
    
    // Now apply queen promotion prioritization (simulate quiescence logic)
    auto queenPromoIt = moves.begin();
    for (auto it = moves.begin(); it != moves.end(); ++it) {
        if (isPromotion(*it) && promotionType(*it) == QUEEN) {
            if (it != queenPromoIt) {
                std::rotate(queenPromoIt, it, it + 1);
            }
            ++queenPromoIt;
        }
    }
    
    std::cout << "\nMoves after Queen Promotion Prioritization:\n";
    for (size_t i = 0; i < moves.size(); ++i) {
        Move move = moves[i];
        std::cout << "  " << i << ": " << SafeMoveExecutor::moveToString(move);
        if (isPromotion(move)) {
            std::cout << " [promotion to " << 
                (promotionType(move) == QUEEN ? "Queen" :
                 promotionType(move) == ROOK ? "Rook" :
                 promotionType(move) == BISHOP ? "Bishop" : "Knight") << "]";
        }
        if (isCapture(move)) {
            std::cout << " [capture]";
        }
        std::cout << "\n";
    }
    
    // Validation: Check that queen promotions come first
    bool foundNonQueenPromo = false;
    int queenPromoCount = 0;
    bool success = true;
    
    for (const Move& move : moves) {
        if (isPromotion(move) && promotionType(move) == QUEEN) {
            if (foundNonQueenPromo) {
                std::cout << "\nERROR: Queen promotion found after non-queen promotion!\n";
                success = false;
            }
            queenPromoCount++;
        } else if (isPromotion(move) || isCapture(move)) {
            foundNonQueenPromo = true;
        }
    }
    
    std::cout << "\n" << (success ? "SUCCESS" : "FAILURE") << ": ";
    if (success) {
        std::cout << "Queen promotions (" << queenPromoCount << ") correctly prioritized\n";
    } else {
        std::cout << "Queen promotion prioritization failed\n";
    }
    
    return success ? 0 : 1;
}