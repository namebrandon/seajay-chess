#include <iostream>

int main() {
    // Test position: 8/4P3/4K3/8/8/8/4k3/8 w - - 0 1
    // White pawn on e7, white king on e6, black king on e2
    
    // With pawn_eg_r7_center = 150, the pawn on e7 should give +150 in endgame
    // But we're seeing PST of only +50
    
    std::cout << "Debug info for PST calculation:\n";
    std::cout << "Pawn on e7 should have endgame value: 150\n";
    std::cout << "King values are typically small\n";
    std::cout << "Phase should be 0 (pure endgame) with only kings and one pawn\n";
    
    return 0;
}
