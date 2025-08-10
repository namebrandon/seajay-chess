// Debug pawn attack patterns
#include <iostream>
#include "../src/core/board.h"
#include "../src/core/types.h"
#include "../src/core/bitboard.h"

using namespace seajay;

int main() {
    std::cout << "========================================\n";
    std::cout << "Understanding Pawn Attack Patterns\n";
    std::cout << "========================================\n\n";
    
    std::cout << "CHESS RULES REMINDER:\n";
    std::cout << "- Pawns move FORWARD one square (or two from starting position)\n";
    std::cout << "- Pawns capture DIAGONALLY forward\n";
    std::cout << "- Pawns CANNOT capture straight ahead\n\n";
    
    std::cout << "Test case #9: rn2k3/P7/...\n";
    std::cout << "- White pawn on a7\n";
    std::cout << "- Black rook on a8 (straight ahead)\n";
    std::cout << "- Black knight on b8 (diagonal)\n\n";
    
    std::cout << "CORRECT behavior:\n";
    std::cout << "- Pawn CANNOT capture rook on a8 (straight ahead)\n";
    std::cout << "- Pawn CAN capture knight on b8 (diagonal)\n";
    std::cout << "- Total: 4 promotion captures to b8 + 5 king moves = 9 moves\n\n";
    
    std::cout << "The test expectation of 13 moves is WRONG.\n";
    std::cout << "It incorrectly assumes the pawn can capture straight ahead.\n\n";
    
    std::cout << "Similarly for other test cases:\n";
    std::cout << "- Test #2: Pawn can capture b8 knight (diagonal) = 9 moves ✓\n";
    std::cout << "- Test #5: Pawn can capture a8 bishop (diagonal) + move to b8 = 13 moves ✓\n";
    
    return 0;
}