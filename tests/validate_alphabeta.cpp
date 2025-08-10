// Quick validation program for alpha-beta metrics
#include "../src/core/board.h"
#include "../src/search/negamax.h"
#include "../src/search/types.h"
#include <iostream>
#include <iomanip>
#include <vector>
#include <string>

using namespace seajay;
using namespace seajay::search;

struct TestResult {
    std::string name;
    std::string fen;
    int depth;
    uint64_t nodes;
    uint64_t betaCutoffs;
    uint64_t firstMoveCutoffs;
    double ebf;
    double moveOrderingEff;
};

void runTest(const std::string& name, const std::string& fen, int depth) {
    Board board;
    board.fromFEN(fen);
    
    SearchInfo info;
    info.timeLimit = std::chrono::milliseconds::max(); // No time limit
    
    std::cout << "\nTesting: " << name << std::endl;
    std::cout << "FEN: " << fen << std::endl;
    std::cout << "Depth: " << depth << std::endl;
    
    eval::Score score = negamax(board, depth, 0,
                                eval::Score::minus_infinity(),
                                eval::Score::infinity(),
                                info);
    
    std::cout << "Results:" << std::endl;
    std::cout << "  Nodes searched: " << info.nodes << std::endl;
    std::cout << "  Beta cutoffs: " << info.betaCutoffs << std::endl;
    std::cout << "  First-move cutoffs: " << info.betaCutoffsFirst << std::endl;
    std::cout << "  EBF: " << std::fixed << std::setprecision(2) 
              << info.effectiveBranchingFactor() << std::endl;
    std::cout << "  Move ordering efficiency: " << std::fixed << std::setprecision(1)
              << info.moveOrderingEfficiency() << "%" << std::endl;
    std::cout << "  Score: " << score.to_cp() << " cp" << std::endl;
    std::cout << std::endl;
}

int main() {
    std::cout << "=== Alpha-Beta Pruning Validation ===" << std::endl;
    
    // Test positions
    runTest("Starting Position (depth 4)", 
            "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", 4);
    
    runTest("Starting Position (depth 5)",
            "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", 5);
    
    runTest("Kiwipete (depth 3)",
            "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1", 3);
    
    runTest("Kiwipete (depth 4)",
            "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1", 4);
    
    runTest("Endgame (depth 6)",
            "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1", 6);
    
    runTest("Tactical Position (depth 4)",
            "r2q1rk1/ppp2ppp/2n1bn2/2bpp3/3PP3/2N2N2/PPP1BPPP/R1BQK2R w KQ - 0 8", 4);
    
    // Analysis of results
    std::cout << "\n=== Analysis ===" << std::endl;
    std::cout << "Expected characteristics of correct alpha-beta:" << std::endl;
    std::cout << "1. EBF should be significantly less than average branching factor (~35)" << std::endl;
    std::cout << "2. Move ordering efficiency > 90% indicates good move ordering" << std::endl;
    std::cout << "3. Node count should be much less than b^d (where b~35, d=depth)" << std::endl;
    std::cout << "4. First-move cutoffs should be close to total cutoffs" << std::endl;
    
    return 0;
}