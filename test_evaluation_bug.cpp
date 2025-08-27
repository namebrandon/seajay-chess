#include <iostream>
#include <iomanip>
#include "src/core/board.h"
#include "src/evaluation/evaluate.h"

using namespace seajay;

int main() {
    // Test position after Black's knight captured White's rook on a1
    const char* fen = "r1b1k2r/pp3ppp/3Bp3/3p4/6q1/8/1PP2PPP/n4RK1 b kq - 0 12";
    
    Board board;
    if (!board.fromFEN(fen)) {
        std::cerr << "Failed to parse FEN: " << fen << std::endl;
        return 1;
    }
    
    std::cout << "Testing position: " << fen << "\n\n";
    
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
    std::cout << "Material difference (White perspective): " 
              << (material.value(WHITE) - material.value(BLACK)).value() << " cp\n\n";
    
    // Evaluate position
    eval::Score score = eval::evaluate(board);
    
    std::cout << "Evaluation:\n";
    std::cout << "From side-to-move (Black) perspective: " << score.value() << " cp\n";
    std::cout << "From side-to-move (Black) perspective (to_cp): " << score.to_cp() << " cp\n";
    
    // Also show from White's perspective
    eval::Score whiteScore = -score;
    std::cout << "From White perspective: " << whiteScore.value() << " cp\n";
    std::cout << "From White perspective (to_cp): " << whiteScore.to_cp() << " cp\n\n";
    
    // Convert to pawns for comparison
    std::cout << "In pawns:\n";
    std::cout << "From Black perspective: " << std::fixed << std::setprecision(2) 
              << (score.value() / 100.0) << " pawns\n";
    std::cout << "From White perspective: " << std::fixed << std::setprecision(2)
              << (whiteScore.value() / 100.0) << " pawns\n";
    
    return 0;
}