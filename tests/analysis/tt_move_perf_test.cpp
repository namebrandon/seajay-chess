// Test to demonstrate TT move reordering performance overhead
// This test shows the cost of the redundant std::find after move ordering

#include <chrono>
#include <iostream>
#include <vector>
#include <algorithm>
#include "../../src/core/types.h"
#include "../../src/core/move_list.h"

using namespace seajay;

void testTTMoveOrdering() {
    std::cout << "=== TT Move Reordering Performance Test ===\n\n";
    
    // Test with different move list sizes
    std::vector<int> listSizes = {10, 20, 30, 40, 50};
    
    for (int size : listSizes) {
        MoveList moves;
        
        // Generate pseudo-moves
        for (int i = 0; i < size; ++i) {
            Square from = Square(i % 64);
            Square to = Square((i + 8) % 64);
            moves.add(makeMove(from, to));
        }
        
        // TT move is in the middle of the list (worst case for search)
        Move ttMove = moves[size / 2];
        
        const int iterations = 1000000;
        
        // Measure the redundant search operation
        auto start = std::chrono::high_resolution_clock::now();
        
        for (int iter = 0; iter < iterations; ++iter) {
            // This is the redundant code from negamax.cpp lines 100-108
            auto it = std::find(moves.begin(), moves.end(), ttMove);
            if (it != moves.end() && it != moves.begin()) {
                Move temp = *it;
                std::move_backward(moves.begin(), it, it + 1);
                *moves.begin() = temp;
            }
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        
        double us_per_op = double(duration.count()) / iterations;
        
        std::cout << "Move list size: " << size << "\n";
        std::cout << "  Total time: " << duration.count() << " µs for " << iterations << " iterations\n";
        std::cout << "  Time per operation: " << us_per_op << " µs\n";
        std::cout << "  Operations per second: " << (1000000.0 / us_per_op) << "\n\n";
    }
    
    std::cout << "Analysis:\n";
    std::cout << "- This redundant search is performed at EVERY node in the search tree\n";
    std::cout << "- At 1M nodes/second, this overhead is significant\n";
    std::cout << "- The search is O(n) where n is the number of moves\n";
    std::cout << "- Solution: Trust the move ordering or use a simpler check\n";
}

int main() {
    testTTMoveOrdering();
    return 0;
}