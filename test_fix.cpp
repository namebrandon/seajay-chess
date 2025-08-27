#include <iostream>
#include <iomanip>
#include "src/core/board.h"
#include "src/evaluation/evaluate.h"

using namespace seajay;

int main() {
    std::cout << "Testing PST Fix for Evaluation Symmetry\n";
    std::cout << "========================================\n\n";
    
    // Test 1: Starting position with White to move
    Board board1;
    board1.setStartingPosition();
    eval::Score eval1 = eval::evaluate(board1);
    
    std::cout << "Starting position (White to move):\n";
    std::cout << "  Raw evaluation: " << eval1.value() << " cp\n";
    std::cout << "  From side to move perspective: " << board1.evaluate().value() << " cp\n\n";
    
    // Test 2: Starting position with Black to move
    Board board2;
    board2.fromFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR b KQkq - 0 1");
    eval::Score eval2Raw = eval::evaluate(board2);  // Raw from White's perspective
    eval::Score eval2 = board2.evaluate();          // From Black's perspective
    
    std::cout << "Starting position (Black to move):\n";
    std::cout << "  Raw evaluation (White perspective): " << eval2Raw.value() << " cp\n";
    std::cout << "  From side to move perspective: " << eval2.value() << " cp\n\n";
    
    // Test 3: After symmetric moves 1.e4 e5
    Board board3;
    board3.fromFEN("rnbqkbnr/pppp1ppp/8/4p3/4P3/8/PPPP1PPP/RNBQKBNR w KQkq - 0 2");
    eval::Score eval3 = board3.evaluate();
    
    std::cout << "After 1.e4 e5 (White to move):\n";
    std::cout << "  Evaluation: " << eval3.value() << " cp\n\n";
    
    // Test 4: Check PST contribution directly
    std::cout << "PST Score Analysis:\n";
    board1.setStartingPosition();
    const eval::MgEgScore& pstScore = board1.pstScore();
    std::cout << "  Starting position PST score: " << pstScore.mg.value() << " cp\n";
    
    // Test 5: Simple position to verify fix
    Board board4;
    board4.fromFEN("8/8/8/8/8/8/P7/8 w - - 0 1");  // Just white pawn on a2
    eval::Score eval4 = board4.evaluate();
    
    Board board5;  
    board5.fromFEN("8/p7/8/8/8/8/8/8 w - - 0 1");  // Just black pawn on a7
    eval::Score eval5 = board5.evaluate();
    
    std::cout << "\nSingle pawn test:\n";
    std::cout << "  White pawn on a2: " << eval4.value() << " cp\n";
    std::cout << "  Black pawn on a7: " << eval5.value() << " cp\n";
    std::cout << "  These should be opposite in sign and similar magnitude\n";
    
    std::cout << "\n✓ If starting position evaluates close to 0, the PST bug is fixed!\n";
    std::cout << "✗ If it shows -232 cp, the double negation bug still exists.\n";
    
    return 0;
}