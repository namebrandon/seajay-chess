#include <iostream>
#include <cstdlib>

// Simple test to demonstrate the PST double-negation bug

int main() {
    std::cout << "PST Double Negation Bug Analysis\n";
    std::cout << "=================================\n\n";
    
    // Simulate what happens with a simple PST value
    int pstValue = 10;  // Example PST value for a piece
    
    std::cout << "Original PST value from table: " << pstValue << "\n\n";
    
    // What PST::value() does for Black pieces
    int blackPSTValue = -pstValue;  // PST::value negates for black
    std::cout << "After PST::value() for BLACK: " << blackPSTValue << "\n";
    
    // What board.cpp does with Black pieces
    int boardAccumulation = 0;
    boardAccumulation -= blackPSTValue;  // board.cpp subtracts the value
    
    std::cout << "After board.cpp subtracts it: m_pstScore -= " << blackPSTValue 
              << " = " << boardAccumulation << "\n\n";
    
    std::cout << "PROBLEM: Double negation!\n";
    std::cout << "- PST::value() returns -10 for Black\n";
    std::cout << "- board.cpp does: score -= (-10) = score + 10\n";
    std::cout << "- Result: Black pieces ADD to White's score instead of subtracting!\n\n";
    
    std::cout << "CORRECT BEHAVIOR should be:\n";
    std::cout << "- PST::value() returns 10 for Black (no negation)\n"; 
    std::cout << "- board.cpp does: score -= 10\n";
    std::cout << "- Result: Black pieces properly subtract from White's score\n\n";
    
    // Demonstration with starting position
    std::cout << "Starting Position Analysis:\n";
    std::cout << "- All pieces have symmetric PST values\n";
    std::cout << "- With bug: Black pieces add their PST values → negative eval\n";
    std::cout << "- Without bug: PST values cancel out → eval near 0\n";
    
    return 0;
}