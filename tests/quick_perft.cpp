#include <iostream>
#include "../src/core/board.h"
#include "../src/core/perft.h"

using namespace seajay;

int main() {
    Board board;
    board.setStartingPosition();
    
    // Test perft at depth 4 (should be 197,281)
    Perft perftRunner;
    uint64_t result = perftRunner.perft(board, 4);
    uint64_t expected = 197281;
    
    std::cout << "Perft(4) from startpos: " << result << std::endl;
    std::cout << "Expected: " << expected << std::endl;
    std::cout << "Status: " << (result == expected ? "PASS" : "FAIL") << std::endl;
    
    return result == expected ? 0 : 1;
}