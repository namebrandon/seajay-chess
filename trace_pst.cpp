#include <iostream>
#include <iomanip>
#include "src/evaluation/pst.h"
#include "src/core/types.h"

using namespace seajay;
using namespace seajay::eval;

int main() {
    // Trace what happens when Black moves a7 to a6
    Square a7 = static_cast<Square>(48);  // a7 = 48
    Square a6 = static_cast<Square>(40);  // a6 = 40
    
    std::cout << "Tracing Black pawn move a7 to a6:\n";
    std::cout << "==================================\n\n";
    
    // What PST::value returns for Black pawn on a7
    MgEgScore a7_value = PST::value(PAWN, a7, BLACK);
    std::cout << "PST::value(PAWN, a7, BLACK) = " << a7_value.mg.value() << "\n";
    
    // Break it down:
    Square a7_mirrored = a7 ^ 56;  // Should be 8 (a2)
    std::cout << "a7 (48) mirrored = " << static_cast<int>(a7_mirrored) << " (a2)\n";
    MgEgScore a7_raw = PST::rawValue(PAWN, a7_mirrored);
    std::cout << "PST table at position 8 (a2) = " << a7_raw.mg.value() << "\n\n";
    
    // What PST::value returns for Black pawn on a6
    MgEgScore a6_value = PST::value(PAWN, a6, BLACK);
    std::cout << "PST::value(PAWN, a6, BLACK) = " << a6_value.mg.value() << "\n";
    
    Square a6_mirrored = a6 ^ 56;  // Should be 16 (a3)
    std::cout << "a6 (40) mirrored = " << static_cast<int>(a6_mirrored) << " (a3)\n";
    MgEgScore a6_raw = PST::rawValue(PAWN, a6_mirrored);
    std::cout << "PST table at position 16 (a3) = " << a6_raw.mg.value() << "\n\n";
    
    std::cout << "When Black moves a7 to a6:\n";
    std::cout << "--------------------------\n";
    std::cout << "m_pstScore -= PST::value(PAWN, a7, BLACK) = -" << a7_value.mg.value() << "\n";
    std::cout << "m_pstScore += PST::value(PAWN, a6, BLACK) = +" << a6_value.mg.value() << "\n";
    std::cout << "Net change to m_pstScore = " << (a6_value.mg.value() - a7_value.mg.value()) << "\n\n";
    
    std::cout << "Interpretation:\n";
    std::cout << "m_pstScore is stored from White's perspective\n";
    std::cout << "Change of " << (a6_value.mg.value() - a7_value.mg.value()) 
              << " means position is " << ((a6_value.mg.value() - a7_value.mg.value()) > 0 ? "BETTER" : "WORSE or SAME") 
              << " for White\n";
    
    // Now test a good move: d7 to d5
    std::cout << "\n\nCompare with d7 to d5:\n";
    std::cout << "======================\n";
    
    Square d7 = static_cast<Square>(51);  // d7
    Square d5 = static_cast<Square>(35);  // d5
    
    MgEgScore d7_value = PST::value(PAWN, d7, BLACK);
    MgEgScore d5_value = PST::value(PAWN, d5, BLACK);
    
    std::cout << "PST::value(PAWN, d7, BLACK) = " << d7_value.mg.value() << "\n";
    std::cout << "PST::value(PAWN, d5, BLACK) = " << d5_value.mg.value() << "\n";
    std::cout << "Net change for d7-d5 = " << (d5_value.mg.value() - d7_value.mg.value()) << "\n";
    std::cout << "This change is " << ((d5_value.mg.value() - d7_value.mg.value()) > 0 ? "BETTER" : "WORSE") 
              << " for White (should be worse!)\n";
    
    return 0;
}
