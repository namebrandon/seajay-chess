#include <iostream>
#include <iomanip>
#include "../../src/core/board.h"
#include "../../src/search/negamax.h"
#include "../../src/search/aspiration_window.h"
#include "../../src/evaluation/types.h"
#include "../../src/core/transposition_table.h"

using namespace seajay;
using namespace seajay::search;
using namespace seajay::eval;

// Test that aspiration windows widen progressively
void testProgressiveWidening() {
    std::cout << "Testing Progressive Widening Sequence...\n";
    
    // Initial window around score 150
    Score previousScore(150);
    AspirationWindow window = calculateInitialWindow(previousScore, 5);
    
    std::cout << "Initial window: [" << window.alpha.value() 
              << ", " << window.beta.value() << "] delta=" << window.delta << "\n";
    
    // Simulate a fail high (score = 200 > beta)
    Score failScore(200);
    
    // Test progressive widening sequence
    for (int i = 0; i < 5; i++) {
        window = widenWindow(window, failScore, true);
        
        std::cout << "Attempt " << window.attempts << ": [" 
                  << window.alpha.value() << ", " << window.beta.value() 
                  << "] delta=" << window.delta;
        
        if (window.isInfinite()) {
            std::cout << " (INFINITE)";
        }
        std::cout << "\n";
        
        if (window.exceedsMaxAttempts()) {
            break;
        }
    }
    
    // Verify final window is infinite after 5 attempts
    if (window.attempts >= 5 && window.isInfinite()) {
        std::cout << "✓ Window correctly becomes infinite after 5 attempts\n";
    } else {
        std::cout << "✗ Window should be infinite after 5 attempts\n";
    }
}

// Test in actual search
void testSearchWithAspiration() {
    std::cout << "\nTesting Aspiration Windows in Search...\n";
    
    Board board;
    board.fromFEN("r1bqkbnr/pppp1ppp/2n5/4p3/4P3/5N2/PPPP1PPP/RNBQKB1R w KQkq - 0 1");
    
    SearchLimits limits;
    limits.maxDepth = 6;  // Deep enough to trigger aspiration
    limits.movetime = std::chrono::milliseconds(1000);
    
    TranspositionTable tt(16);  // 16MB table
    
    // Run search with aspiration windows
    Move bestMove = searchIterativeTest(board, limits, &tt);
    
    std::cout << "Search completed with aspiration windows\n";
    std::cout << "Best move: " << SafeMoveExecutor::moveToString(bestMove) << "\n";
}

int main() {
    std::cout << "=== Stage 13, Deliverable 3.2d: Progressive Widening Test ===\n\n";
    
    testProgressiveWidening();
    testSearchWithAspiration();
    
    std::cout << "\n=== Tests Complete ===\n";
    return 0;
}