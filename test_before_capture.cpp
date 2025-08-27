#include <iostream>
#include <iomanip>
#include "src/core/board.h"
#include "src/evaluation/evaluate.h"

using namespace seajay;

int main() {
    // Let's reconstruct what the position was BEFORE Nxa1
    // The knight was probably on b3 or c2 before capturing the rook on a1
    // Let's test with knight on c2 and rook still on a1
    const char* fen_before = "r1b1k2r/pp3ppp/3Bp3/3p4/6q1/8/1Pn2PPP/R4RK1 w kq - 0 12";
    
    Board board_before;
    if (!board_before.fromFEN(fen_before)) {
        std::cerr << "Failed to parse FEN: " << fen_before << std::endl;
        return 1;
    }
    
    std::cout << "Position BEFORE Nxa1:\n";
    std::cout << "FEN: " << fen_before << "\n\n";
    
    // Get material counts
    const eval::Material& material_before = board_before.material();
    
    std::cout << "Material Count:\n";
    std::cout << "White: P=" << material_before.count(WHITE, PAWN) 
              << " N=" << material_before.count(WHITE, KNIGHT)
              << " B=" << material_before.count(WHITE, BISHOP)
              << " R=" << material_before.count(WHITE, ROOK)
              << " Q=" << material_before.count(WHITE, QUEEN) << "\n";
    std::cout << "Black: P=" << material_before.count(BLACK, PAWN)
              << " N=" << material_before.count(BLACK, KNIGHT) 
              << " B=" << material_before.count(BLACK, BISHOP)
              << " R=" << material_before.count(BLACK, ROOK)
              << " Q=" << material_before.count(BLACK, QUEEN) << "\n\n";
    
    std::cout << "Material Values:\n";
    std::cout << "White material: " << material_before.value(WHITE).value() << " cp\n";
    std::cout << "Black material: " << material_before.value(BLACK).value() << " cp\n";
    std::cout << "Material difference (White perspective): " 
              << (material_before.value(WHITE) - material_before.value(BLACK)).value() << " cp\n\n";
    
    // Evaluate position
    eval::Score score_before = eval::evaluate(board_before);
    
    std::cout << "Evaluation:\n";
    std::cout << "From side-to-move (White) perspective: " << score_before.value() << " cp\n";
    std::cout << "In pawns: " << std::fixed << std::setprecision(2) 
              << (score_before.value() / 100.0) << " pawns\n\n";
    
    // Now the position AFTER Nxa1
    const char* fen_after = "r1b1k2r/pp3ppp/3Bp3/3p4/6q1/8/1PP2PPP/n4RK1 b kq - 0 12";
    
    Board board_after;
    if (!board_after.fromFEN(fen_after)) {
        std::cerr << "Failed to parse FEN: " << fen_after << std::endl;
        return 1;
    }
    
    std::cout << "Position AFTER Nxa1:\n";
    std::cout << "FEN: " << fen_after << "\n\n";
    
    const eval::Material& material_after = board_after.material();
    
    std::cout << "Material Count:\n";
    std::cout << "White: P=" << material_after.count(WHITE, PAWN) 
              << " N=" << material_after.count(WHITE, KNIGHT)
              << " B=" << material_after.count(WHITE, BISHOP)
              << " R=" << material_after.count(WHITE, ROOK)
              << " Q=" << material_after.count(WHITE, QUEEN) << "\n";
    std::cout << "Black: P=" << material_after.count(BLACK, PAWN)
              << " N=" << material_after.count(BLACK, KNIGHT) 
              << " B=" << material_after.count(BLACK, BISHOP)
              << " R=" << material_after.count(BLACK, ROOK)
              << " Q=" << material_after.count(BLACK, QUEEN) << "\n\n";
    
    std::cout << "Material Values:\n";
    std::cout << "White material: " << material_after.value(WHITE).value() << " cp\n";
    std::cout << "Black material: " << material_after.value(BLACK).value() << " cp\n";
    std::cout << "Material difference (White perspective): " 
              << (material_after.value(WHITE) - material_after.value(BLACK)).value() << " cp\n\n";
    
    // Evaluate position  
    eval::Score score_after = eval::evaluate(board_after);
    
    std::cout << "Evaluation:\n";
    std::cout << "From side-to-move (Black) perspective: " << score_after.value() << " cp\n";
    std::cout << "From White perspective: " << (-score_after).value() << " cp\n";
    std::cout << "In pawns (Black perspective): " << std::fixed << std::setprecision(2) 
              << (score_after.value() / 100.0) << " pawns\n";
    std::cout << "In pawns (White perspective): " << std::fixed << std::setprecision(2)
              << ((-score_after).value() / 100.0) << " pawns\n\n";
              
    std::cout << "--- SUMMARY ---\n";
    std::cout << "White lost a rook (510 cp) when Black played Nxa1\n";
    std::cout << "Expected evaluation change: approximately -510 cp for White\n";
    std::cout << "Actual evaluation change: from " << score_before.value() 
              << " to " << (-score_after).value() << " = " 
              << ((-score_after).value() - score_before.value()) << " cp\n";
              
    return 0;
}