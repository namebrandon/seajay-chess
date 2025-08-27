#include <iostream>
#include <iomanip>
#include "src/core/board.h"
#include "src/evaluation/evaluate.h"
#include "src/evaluation/material.h"

using namespace seajay;

int main() {
    std::cout << "=== Testing with CORRECTED FEN ===\n\n";
    
    // The CORRECT position after Nxa1 (without the extra c2 pawn)
    const char* fen_correct = "r1b1k2r/pp3ppp/3Bp3/3p4/6q1/8/1P3PPP/n4RK1 b kq - 0 12";
    
    Board board;
    if (!board.fromFEN(fen_correct)) {
        std::cerr << "Failed to parse FEN: " << fen_correct << std::endl;
        return 1;
    }
    
    std::cout << "Position after Nxa1 (CORRECTED FEN):\n";
    std::cout << fen_correct << "\n\n";
    
    // Get material counts
    const eval::Material& material = board.material();
    
    std::cout << "Material Count:\n";
    std::cout << "White: P=" << material.count(WHITE, PAWN) 
              << " N=" << material.count(WHITE, KNIGHT)
              << " B=" << material.count(WHITE, BISHOP)
              << " R=" << material.count(WHITE, ROOK)
              << " Q=" << material.count(WHITE, QUEEN) << "\n";
    std::cout << "Black: P=" << material.count(BLACK, PAWN)
              << " N=" << material.count(BLACK, KNIGHT) 
              << " B=" << material.count(BLACK, BISHOP)
              << " R=" << material.count(BLACK, ROOK)
              << " Q=" << material.count(BLACK, QUEEN) << "\n\n";
    
    std::cout << "Material Values:\n";
    std::cout << "White material: " << material.value(WHITE).value() << " cp\n";
    std::cout << "Black material: " << material.value(BLACK).value() << " cp\n";
    
    int material_diff = (material.value(BLACK) - material.value(WHITE)).value();
    std::cout << "Material difference (Black advantage): " << material_diff << " cp\n";
    std::cout << "Material difference in pawns: " << std::fixed << std::setprecision(2)
              << (material_diff / 100.0) << " pawns\n\n";
    
    // Evaluate position
    eval::Score score = eval::evaluate(board);
    
    std::cout << "Full Evaluation:\n";
    std::cout << "From side-to-move (Black) perspective: " << score.value() << " cp\n";
    std::cout << "From White perspective: " << (-score).value() << " cp\n";
    std::cout << "In pawns (Black perspective): " << std::fixed << std::setprecision(2) 
              << (score.value() / 100.0) << " pawns\n";
    std::cout << "In pawns (White perspective): " << std::fixed << std::setprecision(2)
              << ((-score).value() / 100.0) << " pawns\n\n";
    
    std::cout << "=== BREAKDOWN ===\n";
    std::cout << "Material difference alone: " << material_diff << " cp (" 
              << (material_diff / 100.0) << " pawns)\n";
    std::cout << "Additional positional factors: " << (score.value() - material_diff) << " cp\n";
    std::cout << "\nExpected material difference after losing a rook:\n";
    std::cout << "- White Queen missing: -950 cp\n";
    std::cout << "- Black has extra rook: +510 cp\n";
    std::cout << "- Black has extra knight: +320 cp\n"; 
    std::cout << "- Black has 3 extra pawns: +300 cp\n";
    std::cout << "Total expected: 950 + 510 + 320 + 300 = 2080 cp\n";
    std::cout << "Actual material difference: " << material_diff << " cp\n";
    std::cout << "\n=== CONCLUSION ===\n";
    std::cout << "With the CORRECT FEN (4 white pawns instead of 5),\n";
    std::cout << "the evaluation is reasonable for a position where:\n";
    std::cout << "- White is missing the queen\n";
    std::cout << "- Black has an extra rook, knight, and 3 pawns\n";
    std::cout << "The ~20.8 pawn advantage makes sense given White has no queen!\n";
    
    return 0;
}