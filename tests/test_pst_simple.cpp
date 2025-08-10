#include <iostream>
#include "../src/evaluation/pst.h"
#include "../src/core/types.h"

using namespace seajay;
using namespace seajay::eval;

int main() {
    std::cout << "Testing PST implementation...\n";
    
    // Test 1: Check pawn values
    std::cout << "Test 1: Pawn on rank 1 and 8 should have 0 value\n";
    for (Square sq = A1; sq <= H1; ++sq) {
        if (PST::rawValue(PAWN, sq).mg.value() != 0) {
            std::cerr << "ERROR: Pawn on rank 1 has non-zero value!\n";
            return 1;
        }
    }
    for (Square sq = A8; sq <= H8; ++sq) {
        if (PST::rawValue(PAWN, sq).mg.value() != 0) {
            std::cerr << "ERROR: Pawn on rank 8 has non-zero value!\n";
            return 1;
        }
    }
    std::cout << "  PASSED\n";
    
    // Test 2: Knights prefer center
    std::cout << "Test 2: Knights should prefer center squares\n";
    MgEgScore knightE4 = PST::rawValue(KNIGHT, E4);
    MgEgScore knightA1 = PST::rawValue(KNIGHT, A1);
    if (knightE4.mg.value() <= knightA1.mg.value()) {
        std::cerr << "ERROR: Knight on E4 should score higher than A1!\n";
        std::cerr << "  E4: " << knightE4.mg.value() << ", A1: " << knightA1.mg.value() << "\n";
        return 1;
    }
    std::cout << "  PASSED (E4=" << knightE4.mg.value() << ", A1=" << knightA1.mg.value() << ")\n";
    
    // Test 3: Rank mirroring
    std::cout << "Test 3: Rank mirroring for black pieces\n";
    MgEgScore whitePawnE4 = PST::value(PAWN, E4, WHITE);
    MgEgScore blackPawnE5 = PST::value(PAWN, E5, BLACK);
    if (whitePawnE4.mg.value() != blackPawnE5.mg.value()) {
        std::cerr << "ERROR: Rank mirroring failed!\n";
        std::cerr << "  White pawn E4: " << whitePawnE4.mg.value() 
                  << ", Black pawn E5: " << blackPawnE5.mg.value() << "\n";
        return 1;
    }
    std::cout << "  PASSED\n";
    
    // Test 4: Pawn advancement bonus
    std::cout << "Test 4: Pawns should get bonus for advancement\n";
    MgEgScore pawnE2 = PST::rawValue(PAWN, E2);
    MgEgScore pawnE4 = PST::rawValue(PAWN, E4);
    MgEgScore pawnE6 = PST::rawValue(PAWN, E6);
    if (pawnE6.mg.value() <= pawnE4.mg.value() || pawnE4.mg.value() <= pawnE2.mg.value()) {
        std::cerr << "ERROR: Pawns should score higher on advanced ranks!\n";
        std::cerr << "  E2: " << pawnE2.mg.value() << ", E4: " << pawnE4.mg.value() 
                  << ", E6: " << pawnE6.mg.value() << "\n";
        return 1;
    }
    std::cout << "  PASSED (E2=" << pawnE2.mg.value() << ", E4=" << pawnE4.mg.value() 
              << ", E6=" << pawnE6.mg.value() << ")\n";
    
    // Test 5: King safety (castled position)
    std::cout << "Test 5: King should prefer castled position\n";
    MgEgScore kingG1 = PST::rawValue(KING, G1);
    MgEgScore kingE4 = PST::rawValue(KING, E4);
    if (kingG1.mg.value() <= kingE4.mg.value()) {
        std::cerr << "ERROR: King should prefer G1 (castled) over E4 in middlegame!\n";
        std::cerr << "  G1: " << kingG1.mg.value() << ", E4: " << kingE4.mg.value() << "\n";
        return 1;
    }
    std::cout << "  PASSED (G1=" << kingG1.mg.value() << ", E4=" << kingE4.mg.value() << ")\n";
    
    std::cout << "\nAll PST tests passed!\n";
    return 0;
}