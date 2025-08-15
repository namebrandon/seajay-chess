// Stage 15 Day 6.4: SEE Pruning Performance Validation
#include <iostream>
#include <chrono>
#include <iomanip>
#include <vector>
#include <string>
#include "../src/core/board.h"
#include "../src/search/negamax.h"
#include "../src/search/quiescence.h"
#include "../src/search/types.h"
#include "../src/core/transposition_table.h"

using namespace seajay;
using namespace seajay::search;

struct TestPosition {
    std::string fen;
    std::string description;
    int depth;
};

struct TestResult {
    std::string description;
    uint64_t nodes;
    uint64_t qnodes;
    double timeMs;
    double nps;
    uint64_t pruned;
    double pruneRate;
};

TestResult runTest(const TestPosition& pos, SEEPruningMode mode) {
    Board board;
    if (!board.fromFEN(pos.fen)) {
        std::cerr << "Invalid FEN: " << pos.fen << std::endl;
        return {};
    }
    
    // Set pruning mode
    g_seePruningMode = mode;
    g_seePruningStats.reset();
    
    // Setup search
    SearchLimits limits;
    limits.maxDepth = pos.depth;
    limits.infinite = false;
    
    SearchData data;
    data.depth = pos.depth;
    data.useQuiescence = true;
    
    SearchInfo searchInfo;
    TranspositionTable tt(16);  // 16MB TT
    tt.setEnabled(true);
    
    auto startTime = std::chrono::steady_clock::now();
    
    // Run search  
    eval::Score score = negamax(board, pos.depth, 0, eval::Score(-30000), 
                                eval::Score(30000), searchInfo, data, &tt);
    
    auto endTime = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
    
    TestResult result;
    result.description = pos.description;
    result.nodes = data.nodes;
    result.qnodes = data.qsearchNodes;
    result.timeMs = elapsed;
    result.nps = elapsed > 0 ? (data.nodes * 1000.0) / elapsed : 0;
    result.pruned = g_seePruningStats.seePruned;
    result.pruneRate = g_seePruningStats.pruneRate();
    
    return result;
}

int main() {
    std::cout << "=== Stage 15 Day 6.4: SEE Pruning Performance Validation ===" << std::endl;
    std::cout << std::endl;
    
    // Test positions - tactical and positional
    std::vector<TestPosition> positions = {
        {"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", "Starting position", 7},
        {"r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1", "Kiwipete (tactical)", 6},
        {"r1bqkb1r/pppp1ppp/2n2n2/1B2p3/4P3/5N2/PPPP1PPP/RNBQK2R w KQkq - 4 4", "Italian Game", 7},
        {"rnbqkb1r/pp1ppppp/5n2/2p5/4P3/5N2/PPPP1PPP/RNBQKB1R w KQkq c6 0 4", "Sicilian Defense", 7},
        {"r3k2r/ppp1ppbp/2n3p1/8/3PP1b1/2N2N2/PPP2PPP/R1B1KB1R w KQkq - 0 1", "Complex tactical", 6}
    };
    
    // Test each mode
    std::vector<SEEPruningMode> modes = {
        SEEPruningMode::OFF,
        SEEPruningMode::CONSERVATIVE,
        SEEPruningMode::AGGRESSIVE
    };
    
    std::vector<std::string> modeNames = {"OFF", "CONSERVATIVE", "AGGRESSIVE"};
    
    // Store results for comparison
    std::vector<std::vector<TestResult>> allResults(modes.size());
    
    for (size_t m = 0; m < modes.size(); ++m) {
        std::cout << "Testing mode: " << modeNames[m] << std::endl;
        std::cout << "----------------------------------------" << std::endl;
        
        for (const auto& pos : positions) {
            std::cout << "Position: " << pos.description << " (depth " << pos.depth << ")..." << std::flush;
            
            TestResult result = runTest(pos, modes[m]);
            allResults[m].push_back(result);
            
            std::cout << " Done" << std::endl;
            std::cout << "  Nodes: " << result.nodes 
                      << ", QNodes: " << result.qnodes
                      << ", Time: " << result.timeMs << "ms"
                      << ", NPS: " << std::fixed << std::setprecision(0) << result.nps;
            
            if (modes[m] != SEEPruningMode::OFF) {
                std::cout << ", Pruned: " << result.pruned 
                          << " (" << std::fixed << std::setprecision(1) 
                          << result.pruneRate << "%)";
            }
            std::cout << std::endl;
        }
        std::cout << std::endl;
    }
    
    // Performance comparison
    std::cout << "=== Performance Comparison ===" << std::endl;
    std::cout << std::endl;
    
    for (size_t p = 0; p < positions.size(); ++p) {
        std::cout << positions[p].description << ":" << std::endl;
        
        const auto& baseline = allResults[0][p];  // OFF mode
        
        for (size_t m = 0; m < modes.size(); ++m) {
            const auto& result = allResults[m][p];
            
            std::cout << "  " << std::setw(12) << modeNames[m] << ": ";
            std::cout << "Nodes=" << std::setw(8) << result.nodes;
            std::cout << ", Time=" << std::setw(6) << std::fixed 
                      << std::setprecision(0) << result.timeMs << "ms";
            std::cout << ", NPS=" << std::setw(7) << std::fixed 
                      << std::setprecision(0) << result.nps;
            
            if (m > 0) {  // Compare to baseline
                double nodeReduction = 100.0 * (1.0 - (double)result.nodes / baseline.nodes);
                double speedup = baseline.timeMs / result.timeMs;
                
                std::cout << " (";
                if (nodeReduction > 0) {
                    std::cout << "-" << std::fixed << std::setprecision(1) 
                              << nodeReduction << "% nodes";
                } else {
                    std::cout << "+" << std::fixed << std::setprecision(1) 
                              << -nodeReduction << "% nodes";
                }
                std::cout << ", " << std::fixed << std::setprecision(2) 
                          << speedup << "x speed)";
            }
            
            std::cout << std::endl;
        }
        std::cout << std::endl;
    }
    
    // Summary statistics
    std::cout << "=== Summary ===" << std::endl;
    
    double totalNodesOff = 0, totalNodesConservative = 0, totalNodesAggressive = 0;
    double totalTimeOff = 0, totalTimeConservative = 0, totalTimeAggressive = 0;
    
    for (size_t p = 0; p < positions.size(); ++p) {
        totalNodesOff += allResults[0][p].nodes;
        totalNodesConservative += allResults[1][p].nodes;
        totalNodesAggressive += allResults[2][p].nodes;
        
        totalTimeOff += allResults[0][p].timeMs;
        totalTimeConservative += allResults[1][p].timeMs;
        totalTimeAggressive += allResults[2][p].timeMs;
    }
    
    std::cout << "Total nodes searched:" << std::endl;
    std::cout << "  OFF:          " << std::setw(10) << (uint64_t)totalNodesOff << std::endl;
    std::cout << "  CONSERVATIVE: " << std::setw(10) << (uint64_t)totalNodesConservative 
              << " (" << std::fixed << std::setprecision(1) 
              << (100.0 * (1.0 - totalNodesConservative/totalNodesOff)) << "% reduction)" << std::endl;
    std::cout << "  AGGRESSIVE:   " << std::setw(10) << (uint64_t)totalNodesAggressive
              << " (" << std::fixed << std::setprecision(1) 
              << (100.0 * (1.0 - totalNodesAggressive/totalNodesOff)) << "% reduction)" << std::endl;
    
    std::cout << std::endl;
    std::cout << "Average speedup:" << std::endl;
    std::cout << "  CONSERVATIVE: " << std::fixed << std::setprecision(2) 
              << (totalTimeOff / totalTimeConservative) << "x" << std::endl;
    std::cout << "  AGGRESSIVE:   " << std::fixed << std::setprecision(2) 
              << (totalTimeOff / totalTimeAggressive) << "x" << std::endl;
    
    std::cout << std::endl;
    std::cout << "Performance validation complete!" << std::endl;
    
    return 0;
}