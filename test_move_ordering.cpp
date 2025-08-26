#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include "src/core/board.h"
#include "src/search/search_core.h"
#include "src/search/types.h"
#include "src/search/negamax.h"
#include "src/tt/transposition_table.h"

using namespace seajay;

void testMoveOrdering(const std::string& fen, int depth) {
    Board board;
    if (!board.setFromFEN(fen)) {
        std::cerr << "Invalid FEN: " << fen << std::endl;
        return;
    }
    
    std::cout << "\n=== Testing Move Ordering ===" << std::endl;
    std::cout << "Position: " << fen << std::endl;
    std::cout << "Depth: " << depth << std::endl;
    
    // Set up search
    SearchInfo info;
    SearchData data;
    SearchLimits limits;
    limits.depth = depth;
    
    // Create TT
    TranspositionTable tt(16);  // 16 MB
    
    // Run search
    auto startTime = std::chrono::steady_clock::now();
    Move bestMove = search(board, limits, info, &tt);
    auto endTime = std::chrono::steady_clock::now();
    
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    
    // Display results
    std::cout << "\nResults:" << std::endl;
    std::cout << "  Nodes: " << data.nodes << std::endl;
    std::cout << "  Beta cutoffs: " << data.betaCutoffs << std::endl;
    std::cout << "  First move cutoffs: " << data.betaCutoffsFirst << std::endl;
    std::cout << "  Move ordering efficiency: " << std::fixed << std::setprecision(1) 
              << data.moveOrderingEfficiency() << "%" << std::endl;
    std::cout << "  Time: " << elapsed.count() << " ms" << std::endl;
    std::cout << "  NPS: " << (data.nodes * 1000) / (elapsed.count() + 1) << std::endl;
    
    // Additional detailed stats
    if (data.betaCutoffs > 0) {
        std::cout << "\nDetailed Move Ordering Stats:" << std::endl;
        std::cout << "  Total moves examined: " << data.totalMoves << std::endl;
        std::cout << "  Average moves per node: " << std::fixed << std::setprecision(2)
                  << static_cast<double>(data.totalMoves) / data.nodes << std::endl;
        std::cout << "  Effective branching factor: " << std::fixed << std::setprecision(2)
                  << data.effectiveBranchingFactor() << std::endl;
    }
}

int main() {
    // Test positions
    std::vector<std::pair<std::string, std::string>> positions = {
        {"Starting position", "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"},
        {"Kiwipete (tactical)", "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -"},
        {"Complex middlegame", "r1bq1rk1/pppp1ppp/2n2n2/1B2p3/1b2P3/3P1N2/PPP2PPP/RNBQK2R w KQ -"},
    };
    
    for (const auto& [name, fen] : positions) {
        std::cout << "\n================================================" << std::endl;
        std::cout << "Testing: " << name << std::endl;
        std::cout << "================================================" << std::endl;
        
        // Test at different depths
        for (int depth : {8, 10, 12}) {
            testMoveOrdering(fen, depth);
        }
    }
    
    return 0;
}