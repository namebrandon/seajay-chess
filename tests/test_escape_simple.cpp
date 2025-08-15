#include <iostream>
#include <algorithm>
#include "../src/core/types.h"

using namespace seajay;

// Simple test to verify escape route ordering logic
void testEscapeOrdering() {
    std::cout << "Testing Escape Route Prioritization Logic\n\n";
    
    // Create sample moves
    struct TestMove {
        Move move;
        bool isKingMove;
        bool isCapture;
        const char* description;
    };
    
    // Simulate moves from a check position
    Square kingSquare = SQ_E1;
    
    TestMove testMoves[] = {
        {encodeMove(SQ_D2, SQ_D3), false, false, "Block with pawn"},
        {encodeMove(SQ_E1, SQ_F1), true, false, "King move away"},
        {encodeMove(SQ_B1, SQ_C3), false, true, "Knight captures checker"},
        {encodeMove(SQ_E1, SQ_D1), true, false, "King move to d1"},
        {encodeMove(SQ_F2, SQ_F3), false, false, "Another block"}
    };
    
    int numMoves = 5;
    
    // Sort according to escape prioritization logic
    std::sort(testMoves, testMoves + numMoves, [kingSquare](const TestMove& a, const TestMove& b) {
        // King moves have highest priority
        if (a.isKingMove && !b.isKingMove) return true;
        if (!a.isKingMove && b.isKingMove) return false;
        
        // Then captures (which might capture the checking piece)
        if (a.isCapture && !b.isCapture) return true;
        if (!a.isCapture && b.isCapture) return false;
        
        // Rest maintain original order (blocks)
        return false;
    });
    
    std::cout << "Sorted move order (best first):\n";
    for (int i = 0; i < numMoves; i++) {
        std::cout << (i+1) << ". " << testMoves[i].description;
        if (testMoves[i].isKingMove) std::cout << " [KING]";
        if (testMoves[i].isCapture) std::cout << " [CAPTURE]";
        std::cout << "\n";
    }
    
    // Verify ordering
    bool kingMovesFirst = testMoves[0].isKingMove && testMoves[1].isKingMove;
    bool captureBeforeBlocks = testMoves[2].isCapture;
    
    if (kingMovesFirst && captureBeforeBlocks) {
        std::cout << "\nPASS: Moves correctly ordered (King > Capture > Block)\n";
    } else {
        std::cout << "\nFAIL: Incorrect move ordering\n";
    }
}

int main() {
    testEscapeOrdering();
    return 0;
}