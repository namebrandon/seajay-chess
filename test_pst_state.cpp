// Quick test to understand PST state issue
#include <iostream>

int main() {
    std::cout << "The issue is that when we call updateEndgameValue,\n";
    std::cout << "we ARE modifying the s_pstTables array correctly.\n";
    std::cout << "The dumpPST command shows the updated values (91, 150, etc).\n";
    std::cout << "\nBut the evaluation still shows PST: 50.\n";
    std::cout << "This suggests recalculatePSTScore() might not be working.\n";
    std::cout << "\nLet's trace the actual evaluation...\n";
    return 0;
}
