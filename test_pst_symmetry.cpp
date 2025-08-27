#include <iostream>
#include "src/core/board.h"
#include "src/core/types.h"
#include "src/evaluation/evaluate.h"
#include "src/evaluation/pst.h"

using namespace seajay;

int main() {
    // Test 1: Starting position
    Board board;
    board.setStartingPosition();
    
    std::cout << "=== PST Symmetry Test ===" << std::endl;
    std::cout << "\n1. Starting position (White to move):" << std::endl;
    
    eval::Score whiteEval = board.evaluate();
    std::cout << "   Evaluation: " << whiteEval.value() << " cp" << std::endl;
    
    // Test 2: Starting position with Black to move
    Board board2;
    board2.fromFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR b KQkq - 0 1");
    std::cout << "\n2. Starting position (Black to move):" << std::endl;
    eval::Score blackEval = board2.evaluate();
    std::cout << "   Evaluation from Black's perspective: " << blackEval.value() << " cp" << std::endl;
    std::cout << "   Evaluation from White's perspective: " << (-blackEval).value() << " cp" << std::endl;
    
    // Test 3: After symmetric moves
    Board board3;
    board3.fromFEN("rnbqkbnr/pppp1ppp/8/4p3/4P3/8/PPPP1PPP/RNBQKBNR w KQkq - 0 2");
    std::cout << "\n3. After 1.e4 e5 (White to move):" << std::endl;
    eval::Score afterMovesEval = board3.evaluate();
    std::cout << "   Evaluation: " << afterMovesEval.value() << " cp" << std::endl;
    
    // Test 4: Check PST values directly
    std::cout << "\n4. Direct PST analysis:" << std::endl;
    board.setStartingPosition();
    
    // Get the PST score from board
    const eval::MgEgScore& pstScore = board.pstScore();
    std::cout << "   Board's accumulated PST score: " << pstScore.mg.value() << " cp" << std::endl;
    
    // Manually calculate what PST score should be
    eval::MgEgScore manualPst;
    
    // White pieces
    manualPst += eval::PST::value(ROOK, A1, WHITE);
    manualPst += eval::PST::value(KNIGHT, B1, WHITE);
    manualPst += eval::PST::value(BISHOP, C1, WHITE);
    manualPst += eval::PST::value(QUEEN, D1, WHITE);
    manualPst += eval::PST::value(KING, E1, WHITE);
    manualPst += eval::PST::value(BISHOP, F1, WHITE);
    manualPst += eval::PST::value(KNIGHT, G1, WHITE);
    manualPst += eval::PST::value(ROOK, H1, WHITE);
    for (int i = A2; i <= H2; i++) {
        manualPst += eval::PST::value(PAWN, static_cast<Square>(i), WHITE);
    }
    
    // Black pieces - SHOULD BE SUBTRACTED
    manualPst -= eval::PST::value(ROOK, A8, BLACK);
    manualPst -= eval::PST::value(KNIGHT, B8, BLACK);
    manualPst -= eval::PST::value(BISHOP, C8, BLACK);
    manualPst -= eval::PST::value(QUEEN, D8, BLACK);
    manualPst -= eval::PST::value(KING, E8, BLACK);
    manualPst -= eval::PST::value(BISHOP, F8, BLACK);
    manualPst -= eval::PST::value(KNIGHT, G8, BLACK);
    manualPst -= eval::PST::value(ROOK, H8, BLACK);
    for (int i = A7; i <= H7; i++) {
        manualPst -= eval::PST::value(PAWN, static_cast<Square>(i), BLACK);
    }
    
    std::cout << "   Manual calculation (with double negation bug): " << manualPst.mg.value() << " cp" << std::endl;
    
    // Now calculate what it SHOULD be without the bug
    eval::MgEgScore correctPst;
    
    // White pieces
    correctPst += eval::PST::rawValue(ROOK, A1);
    correctPst += eval::PST::rawValue(KNIGHT, B1);
    correctPst += eval::PST::rawValue(BISHOP, C1);
    correctPst += eval::PST::rawValue(QUEEN, D1);
    correctPst += eval::PST::rawValue(KING, E1);
    correctPst += eval::PST::rawValue(BISHOP, F1);
    correctPst += eval::PST::rawValue(KNIGHT, G1);
    correctPst += eval::PST::rawValue(ROOK, H1);
    for (int i = A2; i <= H2; i++) {
        correctPst += eval::PST::rawValue(PAWN, static_cast<Square>(i));
    }
    
    // Black pieces - properly mirrored and negated
    correctPst -= eval::PST::rawValue(ROOK, Square(A8 ^ 56));
    correctPst -= eval::PST::rawValue(KNIGHT, Square(B8 ^ 56));
    correctPst -= eval::PST::rawValue(BISHOP, Square(C8 ^ 56));
    correctPst -= eval::PST::rawValue(QUEEN, Square(D8 ^ 56));
    correctPst -= eval::PST::rawValue(KING, Square(E8 ^ 56));
    correctPst -= eval::PST::rawValue(BISHOP, Square(F8 ^ 56));
    correctPst -= eval::PST::rawValue(KNIGHT, Square(G8 ^ 56));
    correctPst -= eval::PST::rawValue(ROOK, Square(H8 ^ 56));
    for (int i = A7; i <= H7; i++) {
        correctPst -= eval::PST::rawValue(PAWN, Square(i ^ 56));
    }
    
    std::cout << "   Correct PST (without double negation): " << correctPst.mg.value() << " cp" << std::endl;
    
    std::cout << "\n=== Analysis ===" << std::endl;
    std::cout << "The bug is a double negation for Black pieces:" << std::endl;
    std::cout << "1. PST::value() negates the value for Black pieces" << std::endl;
    std::cout << "2. board.cpp then subtracts this already-negated value" << std::endl;
    std::cout << "Result: Black pieces ADD to White's score instead of subtracting!" << std::endl;
    
    return 0;
}