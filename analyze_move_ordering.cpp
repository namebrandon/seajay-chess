// Move Ordering Analysis Tool for SeaJay
// This tool analyzes move ordering efficiency at different depths

#include <iostream>
#include <iomanip>
#include <vector>
#include <string>
#include <chrono>
#include "src/core/board.h"
#include "src/search/negamax.h"
#include "src/search/types.h"
#include "src/tt/transposition_table.h"
#include "src/eval/evaluation.h"

using namespace seajay;

struct MoveOrderingAnalysis {
    int depth;
    uint64_t nodes;
    uint64_t betaCutoffs;
    uint64_t betaCutoffsFirst;
    uint64_t betaCutoffsSecond;
    uint64_t betaCutoffsThird;
    uint64_t totalMoves;
    double efficiency;
    double timeMs;
    double nps;
};

void analyzePosition(const std::string& name, const std::string& fen) {
    std::cout << "\n================================================\n";
    std::cout << "Position: " << name << "\n";
    std::cout << "FEN: " << fen << "\n";
    std::cout << "================================================\n\n";
    
    Board board;
    if (!board.setFromFEN(fen)) {
        std::cerr << "Invalid FEN!\n";
        return;
    }
    
    // Test depths
    std::vector<int> depths = {6, 8, 10, 12};
    std::vector<MoveOrderingAnalysis> results;
    
    TranspositionTable tt(64);  // 64MB TT
    
    for (int depth : depths) {
        SearchInfo info;
        SearchData data;
        SearchLimits limits;
        limits.depth = depth;
        
        auto startTime = std::chrono::steady_clock::now();
        
        // Run search
        Move bestMove = search(board, limits, info, &tt);
        
        auto endTime = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
        
        MoveOrderingAnalysis analysis;
        analysis.depth = depth;
        analysis.nodes = data.nodes;
        analysis.betaCutoffs = data.betaCutoffs;
        analysis.betaCutoffsFirst = data.betaCutoffsFirst;
        analysis.totalMoves = data.totalMoves;
        analysis.efficiency = data.moveOrderingEfficiency();
        analysis.timeMs = elapsed.count();
        analysis.nps = (data.nodes * 1000.0) / (elapsed.count() + 1);
        
        // Calculate additional cutoff positions (we'll approximate for now)
        // These would need to be tracked in the actual search
        analysis.betaCutoffsSecond = 0;
        analysis.betaCutoffsThird = 0;
        
        results.push_back(analysis);
    }
    
    // Display results table
    std::cout << "Depth | Nodes      | Beta Cuts  | First Move | Efficiency | Time(ms) | NPS\n";
    std::cout << "------|------------|------------|------------|------------|----------|----------\n";
    
    for (const auto& r : results) {
        std::cout << std::setw(5) << r.depth << " | "
                  << std::setw(10) << r.nodes << " | "
                  << std::setw(10) << r.betaCutoffs << " | "
                  << std::setw(10) << r.betaCutoffsFirst << " | "
                  << std::fixed << std::setprecision(1) << std::setw(9) << r.efficiency << "% | "
                  << std::setw(8) << std::fixed << std::setprecision(0) << r.timeMs << " | "
                  << std::setw(8) << static_cast<uint64_t>(r.nps) << "\n";
    }
    
    // Analyze efficiency degradation
    std::cout << "\n--- Efficiency Analysis ---\n";
    if (results.size() >= 2) {
        double startEff = results[0].efficiency;
        double endEff = results.back().efficiency;
        double degradation = startEff - endEff;
        
        std::cout << "Efficiency at depth " << results[0].depth << ": " 
                  << std::fixed << std::setprecision(1) << startEff << "%\n";
        std::cout << "Efficiency at depth " << results.back().depth << ": " 
                  << std::fixed << std::setprecision(1) << endEff << "%\n";
        std::cout << "Degradation: " << std::fixed << std::setprecision(1) 
                  << degradation << " percentage points\n";
        
        if (endEff < 75.0) {
            std::cout << "WARNING: Poor move ordering efficiency (<75%) at depth " 
                      << results.back().depth << "\n";
        } else if (endEff < 85.0) {
            std::cout << "NOTE: Below optimal move ordering efficiency (<85%) at depth " 
                      << results.back().depth << "\n";
        }
    }
    
    // Calculate effective branching factor trend
    std::cout << "\n--- Branching Factor ---\n";
    for (size_t i = 1; i < results.size(); i++) {
        double ebf = std::pow(static_cast<double>(results[i].nodes) / results[i-1].nodes, 
                             1.0 / (results[i].depth - results[i-1].depth));
        std::cout << "Depth " << results[i-1].depth << "->" << results[i].depth 
                  << ": EBF = " << std::fixed << std::setprecision(2) << ebf << "\n";
    }
}

int main() {
    std::cout << "===========================================\n";
    std::cout << "SeaJay Move Ordering Efficiency Analysis\n";
    std::cout << "===========================================\n\n";
    
    // Test positions
    std::vector<std::pair<std::string, std::string>> positions = {
        {"Starting Position", "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"},
        {"Kiwipete (Tactical)", "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -"},
        {"Complex Middlegame", "r1bq1rk1/pppp1ppp/2n2n2/1B2p3/1b2P3/3P1N2/PPP2PPP/RNBQK2R w KQ -"},
        {"Endgame", "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - -"},
        {"Sharp Position", "r1bqk2r/pppp1ppp/2n2n2/1B2p3/1b2P3/5N2/PPPP1PPP/RNBQ1RK1 b kq -"}
    };
    
    for (const auto& [name, fen] : positions) {
        analyzePosition(name, fen);
    }
    
    std::cout << "\n===========================================\n";
    std::cout << "Analysis Complete\n";
    std::cout << "===========================================\n";
    
    return 0;
}