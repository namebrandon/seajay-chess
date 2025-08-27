#include <iostream>
#include <vector>

// Test MVV-LVA scoring logic
int main() {
    // Victim values
    int PAWN = 100, KNIGHT = 325, BISHOP = 325, ROOK = 500, QUEEN = 900;
    
    // Attacker values  
    int P = 1, N = 3, B = 3, R = 5, Q = 9;
    
    std::cout << "MVV-LVA Scores:\n";
    std::cout << "===============\n";
    
    // High-value captures (should be > 300)
    std::cout << "High-value captures:\n";
    std::cout << "QxQ: " << QUEEN - Q << " = " << (QUEEN - Q) << "\n";
    std::cout << "QxR: " << ROOK - Q << " = " << (ROOK - Q) << "\n";
    std::cout << "RxQ: " << QUEEN - R << " = " << (QUEEN - R) << "\n";
    std::cout << "RxR: " << ROOK - R << " = " << (ROOK - R) << "\n";
    std::cout << "PxQ: " << QUEEN - P << " = " << (QUEEN - P) << "\n";
    std::cout << "PxR: " << ROOK - P << " = " << (ROOK - P) << "\n";
    
    std::cout << "\nMedium-value captures:\n";
    std::cout << "QxB: " << BISHOP - Q << " = " << (BISHOP - Q) << "\n";
    std::cout << "QxN: " << KNIGHT - Q << " = " << (KNIGHT - Q) << "\n";
    std::cout << "RxB: " << BISHOP - R << " = " << (BISHOP - R) << "\n";
    std::cout << "RxN: " << KNIGHT - R << " = " << (KNIGHT - R) << "\n";
    std::cout << "BxB: " << BISHOP - B << " = " << (BISHOP - B) << "\n";
    std::cout << "BxN: " << KNIGHT - B << " = " << (KNIGHT - B) << "\n";
    std::cout << "NxB: " << BISHOP - N << " = " << (BISHOP - N) << "\n";
    std::cout << "NxN: " << KNIGHT - N << " = " << (KNIGHT - N) << "\n";
    std::cout << "PxB: " << BISHOP - P << " = " << (BISHOP - P) << "\n";
    std::cout << "PxN: " << KNIGHT - P << " = " << (KNIGHT - P) << "\n";
    
    std::cout << "\nLow-value captures:\n";
    std::cout << "QxP: " << PAWN - Q << " = " << (PAWN - Q) << "\n";
    std::cout << "RxP: " << PAWN - R << " = " << (PAWN - R) << "\n";
    std::cout << "BxP: " << PAWN - B << " = " << (PAWN - B) << "\n";
    std::cout << "NxP: " << PAWN - N << " = " << (PAWN - N) << "\n";
    std::cout << "PxP: " << PAWN - P << " = " << (PAWN - P) << "\n";
    
    std::cout << "\n\nPROBLEM IDENTIFIED:\n";
    std::cout << "===================\n";
    std::cout << "Current threshold of 100 puts PxP (score 99) in low-value!\n";
    std::cout << "PxP should be medium-value (equal exchange)\n";
    std::cout << "\nSuggested thresholds:\n";
    std::cout << "High-value: > 300 (Queen/Rook captures)\n";
    std::cout << "Medium-value: 95-300 (Minor piece captures, equal exchanges)\n";
    std::cout << "Low-value: < 95 (Heavy pieces capturing pawns, excluding PxP)\n";
    
    return 0;
}