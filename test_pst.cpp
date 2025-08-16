#include <iostream>
#include <iomanip>
#include "src/evaluation/pst.h"

using namespace seajay;
using namespace seajay::eval;

int main() {
    // Test PST values for Black pawns from their starting squares
    std::cout << "PST values for Black pawns at starting position:\n";
    std::cout << "Square | Raw PST | For Black\n";
    std::cout << "-------|---------|----------\n";
    
    // Black pawns are on rank 7 (index 48-55)
    for (int file = 0; file < 8; file++) {
        Square sq = static_cast<Square>(48 + file);  // Rank 7
        
        // Get raw PST value
        MgEgScore raw = PST::rawValue(PAWN, sq);
        
        // Get value for Black (should handle mirroring)
        MgEgScore blackValue = PST::value(PAWN, sq, BLACK);
        
        char fileName = 'a' + file;
        std::cout << fileName << "7    | " 
                  << std::setw(7) << raw.mg.value() 
                  << " | " << std::setw(9) << blackValue.mg.value() << "\n";
    }
    
    std::cout << "\nPST values for Black pawns moved one square forward:\n";
    std::cout << "Square | Raw PST | For Black | Diff from a7\n";
    std::cout << "-------|---------|-----------|-------------\n";
    
    // Black pawns moved to rank 6 (index 40-47)
    for (int file = 0; file < 8; file++) {
        Square sq = static_cast<Square>(40 + file);  // Rank 6
        
        // Get raw PST value
        MgEgScore raw = PST::rawValue(PAWN, sq);
        
        // Get value for Black (should handle mirroring)
        MgEgScore blackValue = PST::value(PAWN, sq, BLACK);
        
        // Calculate difference from starting position
        Square fromSq = static_cast<Square>(48 + file);
        MgEgScore fromValue = PST::value(PAWN, fromSq, BLACK);
        int diff = blackValue.mg.value() - fromValue.mg.value();
        
        char fileName = 'a' + file;
        std::cout << fileName << "6    | " 
                  << std::setw(7) << raw.mg.value() 
                  << " | " << std::setw(9) << blackValue.mg.value() 
                  << " | " << std::setw(11) << diff << "\n";
    }
    
    std::cout << "\nPST values for Black pawns moved two squares forward:\n";
    std::cout << "Square | Raw PST | For Black | Diff from a7\n";
    std::cout << "-------|---------|-----------|-------------\n";
    
    // Black pawns moved to rank 5 (index 32-39)
    for (int file = 0; file < 8; file++) {
        Square sq = static_cast<Square>(32 + file);  // Rank 5
        
        // Get raw PST value
        MgEgScore raw = PST::rawValue(PAWN, sq);
        
        // Get value for Black (should handle mirroring)
        MgEgScore blackValue = PST::value(PAWN, sq, BLACK);
        
        // Calculate difference from starting position
        Square fromSq = static_cast<Square>(48 + file);
        MgEgScore fromValue = PST::value(PAWN, fromSq, BLACK);
        int diff = blackValue.mg.value() - fromValue.mg.value();
        
        char fileName = 'a' + file;
        std::cout << fileName << "5    | " 
                  << std::setw(7) << raw.mg.value() 
                  << " | " << std::setw(9) << blackValue.mg.value() 
                  << " | " << std::setw(11) << diff << "\n";
    }
    
    // Now test how mirroring works
    std::cout << "\nMirroring test:\n";
    std::cout << "Black a7 (sq 48) should map to White a2 (sq 8) in PST table\n";
    Square blackA7 = static_cast<Square>(48);  // a7
    Square mirroredSq = static_cast<Square>(blackA7 ^ 56);  // Should be 8 (a2)
    std::cout << "a7 (48) ^ 56 = " << mirroredSq << " (should be 8, which is a2)\n";
    std::cout << "PST[PAWN][8] = " << PST::rawValue(PAWN, static_cast<Square>(8)).mg.value() << "\n";
    std::cout << "PST value for Black pawn on a7 = " << PST::value(PAWN, blackA7, BLACK).mg.value() << "\n";
    
    return 0;
}
