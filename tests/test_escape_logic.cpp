#include <iostream>
#include <algorithm>
#include <vector>

// Simple test to verify escape route ordering logic
int main() {
    std::cout << "Testing Escape Route Prioritization Logic\n\n";
    
    // Simulate moves with priorities
    struct Move {
        int id;
        bool isKingMove;
        bool isCapture;
        const char* description;
    };
    
    std::vector<Move> moves = {
        {1, false, false, "Block with pawn"},
        {2, true, false, "King move away"},
        {3, false, true, "Knight captures checker"},
        {4, true, false, "King move to safety"},
        {5, false, false, "Another block"}
    };
    
    std::cout << "Original order:\n";
    for (const auto& m : moves) {
        std::cout << m.id << ". " << m.description << "\n";
    }
    
    // Sort according to escape prioritization logic
    std::sort(moves.begin(), moves.end(), [](const Move& a, const Move& b) {
        // King moves have highest priority
        if (a.isKingMove && !b.isKingMove) return true;
        if (!a.isKingMove && b.isKingMove) return false;
        
        // Then captures (which might capture the checking piece)
        if (a.isCapture && !b.isCapture) return true;
        if (!a.isCapture && b.isCapture) return false;
        
        // Rest maintain original order - return false for stable sort
        return false;
    });
    
    std::cout << "\nSorted order (escape prioritization):\n";
    for (const auto& m : moves) {
        std::cout << m.id << ". " << m.description;
        if (m.isKingMove) std::cout << " [KING]";
        if (m.isCapture) std::cout << " [CAPTURE]";
        std::cout << "\n";
    }
    
    // Verify ordering
    bool pass = true;
    if (!moves[0].isKingMove || !moves[1].isKingMove) {
        std::cout << "\nFAIL: King moves should be first\n";
        pass = false;
    }
    if (!moves[2].isCapture) {
        std::cout << "\nFAIL: Captures should come after king moves\n";
        pass = false;
    }
    if (moves[3].isCapture || moves[4].isCapture) {
        std::cout << "\nFAIL: Blocks should be last\n";
        pass = false;
    }
    
    if (pass) {
        std::cout << "\nPASS: Moves correctly ordered (King > Capture > Block)\n";
        std::cout << "This ordering improves alpha-beta cutoffs in check positions.\n";
    }
    
    return pass ? 0 : 1;
}