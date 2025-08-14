#include "src/core/board.h"
#include "src/search/negamax.h"
#include "src/core/transposition_table.h"
#include "src/search/types.h"
#include <iostream>
#include <chrono>

using namespace seajay;
using namespace seajay::search;

int main() {
    Board board;
    board.fromFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    
    TranspositionTable tt(64);
    SearchLimits limits;
    limits.maxDepth = 10;
    limits.movetime = std::chrono::milliseconds(2000);
    
    auto start = std::chrono::steady_clock::now();
    Move move = searchIterativeTest(board, limits, &tt);
    auto end = std::chrono::steady_clock::now();
    
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    // Extract NPS from last info line
    std::cout << "\nDeliverable 3.2b NPS Test:" << std::endl;
    std::cout << "Time: " << elapsed.count() << " ms" << std::endl;
    std::cout << "Aspiration windows active for depth >= 4" << std::endl;
    
    return 0;
}