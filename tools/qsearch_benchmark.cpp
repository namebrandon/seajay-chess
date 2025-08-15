// Phase 2.3 - Missing Item 3: Quiescence Search Performance Benchmark Tool
// Implements performance profiling and hot path analysis for quiescence search

#include "../src/search/quiescence_performance.h"
#include <iostream>
#include <iomanip>
#include <string>
#include <cstring>

using namespace seajay::search;

void printUsage() {
    std::cout << "Quiescence Search Performance Benchmark Tool\n";
    std::cout << "Phase 2.3 - Missing Item 3 from original Stage 14 plan\n\n";
    std::cout << "Usage: qsearch_benchmark [command]\n\n";
    std::cout << "Commands:\n";
    std::cout << "  full       - Run full benchmark suite on tactical positions\n";
    std::cout << "  compare    - Compare performance with/without quiescence\n";
    std::cout << "  profile    - Profile hot paths in quiescence search\n";
    std::cout << "  stack      - Analyze stack usage patterns\n";
    std::cout << "  position   - Benchmark a specific position\n";
    std::cout << "  help       - Show this help message\n\n";
    std::cout << "Examples:\n";
    std::cout << "  qsearch_benchmark full\n";
    std::cout << "  qsearch_benchmark profile\n";
    std::cout << "  qsearch_benchmark position \"fen_string\" depth\n";
}

int main(int argc, char* argv[]) {
    std::cout << "SeaJay Chess Engine - Quiescence Performance Benchmark\n";
    std::cout << "Phase 2.3 Implementation - Performance Profiling\n";
    std::cout << std::string(60, '=') << std::endl;
    
    if (argc < 2) {
        printUsage();
        return 1;
    }
    
    const std::string command = argv[1];
    
    try {
        if (command == "full") {
            QuiescencePerformanceBenchmark::runFullBenchmark();
            
        } else if (command == "compare") {
            QuiescencePerformanceBenchmark::compareQuiescenceImpact();
            
        } else if (command == "profile") {
            QuiescencePerformanceBenchmark::profileHotPaths();
            
        } else if (command == "stack") {
            QuiescencePerformanceBenchmark::measureStackUsage();
            
        } else if (command == "position") {
            if (argc < 4) {
                std::cerr << "Error: position command requires FEN string and depth\\n";
                std::cerr << "Usage: qsearch_benchmark position \"fen_string\" depth\\n";
                return 1;
            }
            
            const std::string fen = argv[2];
            int depth = std::atoi(argv[3]);
            
            if (depth < 1 || depth > 10) {
                std::cerr << "Error: depth must be between 1 and 10\\n";
                return 1;
            }
            
            std::cout << "Benchmarking position: " << fen << std::endl;
            std::cout << "Search depth: " << depth << std::endl;
            
            auto data = QuiescencePerformanceBenchmark::benchmarkPosition(fen, depth, true);
            
            auto timeMs = std::chrono::duration_cast<std::chrono::milliseconds>(data.totalTime).count();
            
            std::cout << "\\nResults:" << std::endl;
            std::cout << "  Total nodes: " << data.totalNodes << std::endl;
            std::cout << "  Quiescence nodes: " << data.qsearchNodes 
                      << " (" << std::fixed << std::setprecision(1) 
                      << (data.getQsearchRatio() * 100) << "%)" << std::endl;
            std::cout << "  Main search nodes: " << data.mainSearchNodes << std::endl;
            std::cout << "  Time: " << timeMs << "ms" << std::endl;
            std::cout << "  NPS: " << data.getNodesPerSecond() << std::endl;
            
            if (data.qsearchNodes > 0) {
                std::cout << "  Quiescence NPS: " << data.getQsearchNPS() << std::endl;
                std::cout << "  Node increase: " << std::fixed << std::setprecision(1) 
                          << (data.getNodeIncrease() * 100) << "%" << std::endl;
            }
            
        } else if (command == "help") {
            printUsage();
            
        } else {
            std::cerr << "Error: Unknown command '" << command << "'\\n\\n";
            printUsage();
            return 1;
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}