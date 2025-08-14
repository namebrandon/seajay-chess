/**
 * SeaJay Chess Engine - Perft Testing Tool
 * Stage 12: Transposition Table Integration
 * 
 * Command-line tool for perft testing with optional TT caching.
 * Usage:
 *   ./perft_tool [options] [fen] [depth]
 * 
 * Options:
 *   --tt          Enable transposition table caching
 *   --tt-size N   Set TT size in MB (default: 128)
 *   --divide      Show perft divide (nodes per move)
 *   --suite       Run standard test suite
 *   --compare     Compare with and without TT
 *   --help        Show this help message
 */

#include "../core/board.h"
#include "../core/perft.h"
#include "../core/transposition_table.h"
#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <cstring>

using namespace seajay;

void printUsage(const char* progName) {
    std::cout << "SeaJay Perft Testing Tool\n";
    std::cout << "Usage: " << progName << " [options] [fen] [depth]\n\n";
    std::cout << "Options:\n";
    std::cout << "  --tt          Enable transposition table caching\n";
    std::cout << "  --tt-size N   Set TT size in MB (default: 128)\n";
    std::cout << "  --divide      Show perft divide (nodes per move)\n";
    std::cout << "  --suite       Run standard test suite\n";
    std::cout << "  --compare     Compare with and without TT\n";
    std::cout << "  --max-depth N Maximum depth for suite (default: 5)\n";
    std::cout << "  --help        Show this help message\n\n";
    std::cout << "Examples:\n";
    std::cout << "  " << progName << " --suite\n";
    std::cout << "  " << progName << " --tt \"startpos\" 6\n";
    std::cout << "  " << progName << " --compare \"r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -\" 4\n";
}

void runComparison(Board& board, int depth, TranspositionTable& tt) {
    std::cout << "\nPerft Comparison at depth " << depth << "\n";
    std::cout << "=========================================\n\n";
    
    // Run without TT
    std::cout << "Without TT: ";
    std::cout.flush();
    auto result1 = Perft::runPerft(board, depth, false, nullptr);
    std::cout << result1.nodes << " nodes in " 
              << std::fixed << std::setprecision(3) << result1.timeSeconds << "s";
    std::cout << " (" << std::fixed << std::setprecision(0) 
              << result1.nps() << " nps)\n";
    
    // Clear TT for fair comparison
    tt.clear();
    tt.resetStats();
    
    // Run with TT (first run)
    std::cout << "With TT (cold): ";
    std::cout.flush();
    auto result2 = Perft::runPerft(board, depth, true, &tt);
    std::cout << result2.nodes << " nodes in " 
              << std::fixed << std::setprecision(3) << result2.timeSeconds << "s";
    std::cout << " (" << std::fixed << std::setprecision(0) 
              << result2.nps() << " nps)\n";
    
    // Print TT stats from first run
    auto& stats = tt.stats();
    std::cout << "  TT Stats: " << stats.stores.load() << " stores, "
              << stats.hits.load() << " hits (" 
              << std::fixed << std::setprecision(1) << stats.hitRate() << "%)\n";
    
    // Run with TT again (warm cache)
    std::cout << "With TT (warm): ";
    std::cout.flush();
    auto result3 = Perft::runPerft(board, depth, true, &tt);
    std::cout << result3.nodes << " nodes in " 
              << std::fixed << std::setprecision(3) << result3.timeSeconds << "s";
    std::cout << " (" << std::fixed << std::setprecision(0) 
              << result3.nps() << " nps)\n";
    
    // Print TT stats from second run
    std::cout << "  TT Stats: " << stats.stores.load() << " stores, "
              << stats.hits.load() << " hits (" 
              << std::fixed << std::setprecision(1) << stats.hitRate() << "%)\n";
    
    // Speedup analysis
    std::cout << "\nSpeedup Analysis:\n";
    double speedup1 = result1.timeSeconds / result2.timeSeconds;
    double speedup2 = result1.timeSeconds / result3.timeSeconds;
    std::cout << "  Cold cache speedup: " << std::fixed << std::setprecision(2) 
              << speedup1 << "x\n";
    std::cout << "  Warm cache speedup: " << std::fixed << std::setprecision(2) 
              << speedup2 << "x\n";
    
    // Verify correctness
    if (result1.nodes != result2.nodes || result1.nodes != result3.nodes) {
        std::cerr << "\n✗ ERROR: Node counts don't match!\n";
        std::cerr << "  Without TT: " << result1.nodes << "\n";
        std::cerr << "  With TT (cold): " << result2.nodes << "\n";
        std::cerr << "  With TT (warm): " << result3.nodes << "\n";
    } else {
        std::cout << "\n✓ Node counts match - TT implementation correct\n";
    }
    
    // Collision analysis
    if (stats.collisions > 0) {
        double collisionRate = 100.0 * stats.collisions.load() / stats.stores.load();
        std::cout << "\nCollision Rate: " << std::fixed << std::setprecision(2) 
                  << collisionRate << "% (" << stats.collisions.load() 
                  << " collisions)\n";
    }
}

void runDivide(Board& board, int depth, bool useTT, TranspositionTable* tt) {
    std::cout << "\nPerft Divide at depth " << depth;
    if (useTT) std::cout << " (with TT)";
    std::cout << "\n================================\n";
    
    auto result = useTT ? Perft::perftDivideWithTT(board, depth, *tt) 
                        : Perft::perftDivide(board, depth);
    
    for (const auto& [move, nodes] : result.moveNodes) {
        std::cout << std::setw(6) << std::left << move << ": " 
                  << std::setw(12) << std::right << nodes << "\n";
    }
    
    std::cout << "\nTotal: " << result.totalNodes << " nodes\n";
}

int main(int argc, char* argv[]) {
    // Parse command line arguments
    bool useTT = false;
    bool showDivide = false;
    bool runSuite = false;
    bool runCompare = false;
    int ttSizeMB = 128;
    int maxDepth = 5;
    std::string fen;
    int depth = 0;
    
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        
        if (arg == "--tt") {
            useTT = true;
        } else if (arg == "--tt-size" && i + 1 < argc) {
            ttSizeMB = std::stoi(argv[++i]);
        } else if (arg == "--divide") {
            showDivide = true;
        } else if (arg == "--suite") {
            runSuite = true;
        } else if (arg == "--compare") {
            runCompare = true;
        } else if (arg == "--max-depth" && i + 1 < argc) {
            maxDepth = std::stoi(argv[++i]);
        } else if (arg == "--help" || arg == "-h") {
            printUsage(argv[0]);
            return 0;
        } else if (fen.empty()) {
            fen = arg;
        } else if (depth == 0) {
            depth = std::stoi(arg);
        }
    }
    
    // Initialize TT if needed
    std::unique_ptr<TranspositionTable> tt;
    if (useTT || runCompare || runSuite) {
        std::cout << "Initializing " << ttSizeMB << "MB transposition table...\n";
        tt = std::make_unique<TranspositionTable>(ttSizeMB);
        std::cout << "TT initialized with " << tt->size() << " entries\n";
    }
    
    // Run standard test suite
    if (runSuite) {
        bool passed = Perft::runStandardTests(maxDepth, useTT, tt.get());
        
        if (useTT && tt) {
            std::cout << "\nTransposition Table Statistics:\n";
            std::cout << "================================\n";
            auto& stats = tt->stats();
            std::cout << "Probes:     " << stats.probes.load() << "\n";
            std::cout << "Hits:       " << stats.hits.load() << "\n";
            std::cout << "Hit Rate:   " << std::fixed << std::setprecision(1) 
                      << stats.hitRate() << "%\n";
            std::cout << "Stores:     " << stats.stores.load() << "\n";
            std::cout << "Collisions: " << stats.collisions.load() << "\n";
            std::cout << "Fill Rate:  " << std::fixed << std::setprecision(1)
                      << tt->fillRate() << "%\n";
        }
        
        return passed ? 0 : 1;
    }
    
    // Need FEN and depth for other operations
    if (fen.empty() || depth == 0) {
        if (!runSuite) {
            std::cerr << "Error: FEN and depth required\n\n";
            printUsage(argv[0]);
            return 1;
        }
    }
    
    // Handle special FEN strings
    if (fen == "startpos") {
        fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
    } else if (fen == "kiwipete") {
        fen = "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1";
    }
    
    // Parse position
    Board board;
    if (!board.fromFEN(fen)) {
        std::cerr << "Error: Invalid FEN string\n";
        return 1;
    }
    
    std::cout << "Position: " << fen << "\n";
    
    // Run comparison
    if (runCompare) {
        runComparison(board, depth, *tt);
        return 0;
    }
    
    // Run divide
    if (showDivide) {
        runDivide(board, depth, useTT, tt.get());
        
        if (useTT && tt) {
            auto& stats = tt->stats();
            std::cout << "\nTT Hit Rate: " << std::fixed << std::setprecision(1) 
                      << stats.hitRate() << "% (" << stats.hits.load() 
                      << " hits / " << stats.probes.load() << " probes)\n";
        }
        return 0;
    }
    
    // Regular perft run
    std::cout << "Running perft(" << depth << ")";
    if (useTT) std::cout << " with TT";
    std::cout << "...\n";
    
    auto result = Perft::runPerft(board, depth, useTT, tt.get());
    
    std::cout << "\nResult: " << result.nodes << " nodes\n";
    std::cout << "Time:   " << std::fixed << std::setprecision(3) 
              << result.timeSeconds << " seconds\n";
    std::cout << "Speed:  " << std::fixed << std::setprecision(0) 
              << result.nps() << " nps\n";
    
    if (useTT && tt) {
        std::cout << "\nTransposition Table Statistics:\n";
        auto& stats = tt->stats();
        std::cout << "  Probes:     " << stats.probes.load() << "\n";
        std::cout << "  Hits:       " << stats.hits.load() << "\n";
        std::cout << "  Hit Rate:   " << std::fixed << std::setprecision(1) 
                  << stats.hitRate() << "%\n";
        std::cout << "  Stores:     " << stats.stores.load() << "\n";
        std::cout << "  Collisions: " << stats.collisions.load() << "\n";
    }
    
    return 0;
}