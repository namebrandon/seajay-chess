#include <iostream>
#include <iomanip>
#include <cmath>
#include "src/core/board.h"
#include "src/evaluation/evaluate.h"

using namespace seajay;

// Test color symmetry: position and its color-flipped version should have opposite evals
bool testColorSymmetry(const std::string& fen, const std::string& flippedFen, 
                       const std::string& description) {
    Board board1, board2;
    board1.fromFEN(fen);
    board2.fromFEN(flippedFen);
    
    eval::Score eval1 = eval::evaluate(board1);
    eval::Score eval2 = eval::evaluate(board2);
    
    int eval1_cp = eval1.value();
    int eval2_cp = eval2.value();
    
    // They should be opposite in sign and equal in magnitude
    bool passed = (eval1_cp == -eval2_cp);
    
    std::cout << (passed ? "✓" : "✗") << " " << description << "\n";
    std::cout << "  Original: " << eval1_cp << " cp\n";
    std::cout << "  Flipped:  " << eval2_cp << " cp\n";
    std::cout << "  Expected: " << -eval1_cp << " cp\n";
    
    if (!passed) {
        std::cout << "  ERROR: Color symmetry broken! Difference: " 
                  << (eval1_cp + eval2_cp) << " cp\n";
    }
    
    return passed;
}

int main() {
    std::cout << "Final Symmetry and Bias Testing\n";
    std::cout << "================================\n\n";
    
    bool all_passed = true;
    
    // Test 1: Starting position from both perspectives
    std::cout << "1. Starting Position Symmetry:\n";
    all_passed &= testColorSymmetry(
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR b KQkq - 0 1",
        "White to move vs Black to move"
    );
    std::cout << "\n";
    
    // Test 2: Color-flipped positions (swap all piece colors)
    std::cout << "2. Color-Flipped Positions:\n";
    all_passed &= testColorSymmetry(
        "RNBQKBNR/PPPPPPPP/8/8/8/8/pppppppp/rnbqkbnr w - - 0 1",
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w - - 0 1",
        "Colors swapped"
    );
    std::cout << "\n";
    
    // Test 3: Asymmetric material positions
    std::cout << "3. Material Imbalance:\n";
    all_passed &= testColorSymmetry(
        "rnbqkbn1/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQq - 0 1",  // Black missing rook
        "RNBQKBN1/PPPPPPPP/8/8/8/8/pppppppp/rnbqkbnr w - - 0 1",     // White missing rook (flipped)
        "Missing rook symmetry"
    );
    std::cout << "\n";
    
    // Test 4: Pawn structure
    std::cout << "4. Pawn Structure:\n";
    all_passed &= testColorSymmetry(
        "8/ppp5/8/8/8/8/PPP5/8 w - - 0 1",  // Three pawns each side
        "8/PPP5/8/8/8/8/ppp5/8 w - - 0 1",  // Flipped
        "Pawn chain symmetry"
    );
    std::cout << "\n";
    
    // Test 5: Piece placement
    std::cout << "5. Piece Placement:\n";
    all_passed &= testColorSymmetry(
        "8/8/8/3N4/3n4/8/8/8 w - - 0 1",  // Knights in center
        "8/8/8/3n4/3N4/8/8/8 w - - 0 1",  // Flipped
        "Central knights symmetry"
    );
    std::cout << "\n";
    
    // Test 6: Complex middlegame
    std::cout << "6. Complex Position:\n";
    all_passed &= testColorSymmetry(
        "r1bqkb1r/pppp1ppp/2n2n2/4p3/2B1P3/5N2/PPPP1PPP/RNBQK2R w KQkq - 0 1",
        "RNBQK2R/PPPP1PPP/5N2/2B1P3/4p3/2n2n2/pppp1ppp/r1bqkb1r w - - 0 1",
        "Italian Game symmetry"
    );
    std::cout << "\n";
    
    // Special test: Ensure PST values are correctly applied
    std::cout << "7. PST Verification:\n";
    Board board;
    board.setStartingPosition();
    const eval::MgEgScore& pstScore = board.pstScore();
    
    std::cout << "  Starting position PST score: " << pstScore.mg.value() << " cp\n";
    bool pst_correct = (std::abs(pstScore.mg.value()) < 1);  // Should be exactly 0
    std::cout << (pst_correct ? "  ✓" : "  ✗") << " PST correctly sums to ~0\n";
    all_passed &= pst_correct;
    
    std::cout << "\n================================\n";
    if (all_passed) {
        std::cout << "✓✓✓ ALL SYMMETRY TESTS PASSED! ✓✓✓\n";
        std::cout << "The evaluation function is now symmetric and unbiased.\n";
    } else {
        std::cout << "✗✗✗ SOME TESTS FAILED ✗✗✗\n";
        std::cout << "There may still be evaluation asymmetries.\n";
    }
    
    return all_passed ? 0 : 1;
}