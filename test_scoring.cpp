#include <iostream>
#include <sstream>
#include "../src/core/board.h"
#include "../src/evaluation/evaluate.h"
#include "../src/search/search.h"

using namespace seajay;

// Test internal scoring to understand how the engine represents scores internally
void testInternalScoring() {
    std::cout << "Testing Internal Scoring (not UCI output)\n";
    std::cout << "=========================================\n\n";
    
    // Test 1: Black to move, Black is winning
    {
        Board board("rqq2k1r/1ppbpp1p/3b4/8/8/3P4/1PPBBPPP/2R1R1K1 b - - 5 17");
        eval::Score score = eval::evaluate(board);
        
        std::cout << "Test 1: Black to move, Black is winning\n";
        std::cout << "FEN: rqq2k1r/1ppbpp1p/3b4/8/8/3P4/1PPBBPPP/2R1R1K1 b - - 5 17\n";
        std::cout << "Internal score (from Black's perspective): " << score.value() << "\n";
        std::cout << "Expected: POSITIVE value (Black is winning)\n\n";
    }
    
    // Test 2: Black to move, Black is losing
    {
        Board board("8/8/4k3/8/4P3/4K3/8/8 b - - 0 1");
        eval::Score score = eval::evaluate(board);
        
        std::cout << "Test 2: Black to move, Black is losing\n";
        std::cout << "FEN: 8/8/4k3/8/4P3/4K3/8/8 b - - 0 1\n";
        std::cout << "Internal score (from Black's perspective): " << score.value() << "\n";
        std::cout << "Expected: NEGATIVE value (Black is losing)\n\n";
    }
    
    // Test 3: White to move, White is winning
    {
        Board board("2R1R1K1/1PPBBPPP/3P4/8/8/3b4/1ppbpp1p/rqq2k1r w - - 5 17");
        eval::Score score = eval::evaluate(board);
        
        std::cout << "Test 3: White to move, White is winning\n";
        std::cout << "FEN: 2R1R1K1/1PPBBPPP/3P4/8/8/3b4/1ppbpp1p/rqq2k1r w - - 5 17\n";
        std::cout << "Internal score (from White's perspective): " << score.value() << "\n";
        std::cout << "Expected: POSITIVE value (White is winning)\n\n";
    }
    
    // Test 4: White to move, White is losing
    {
        Board board("8/8/4K3/8/4p3/4k3/8/8 w - - 0 1");
        eval::Score score = eval::evaluate(board);
        
        std::cout << "Test 4: White to move, White is losing\n";
        std::cout << "FEN: 8/8/4K3/8/4p3/4k3/8/8 w - - 0 1\n";
        std::cout << "Internal score (from White's perspective): " << score.value() << "\n";
        std::cout << "Expected: NEGATIVE value (White is losing)\n\n";
    }
}

int main() {
    testInternalScoring();
    return 0;
}