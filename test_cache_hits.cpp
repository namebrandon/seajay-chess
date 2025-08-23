#include <iostream>
#include <iomanip>
#include "src/core/board.h"
#include "src/core/move_generation.h"
#include "src/core/move_list.h"
#include "src/evaluation/pawn_structure.h"
#include "src/evaluation/evaluate.h"
#include "src/search/search.h"

using namespace seajay;

void measureCacheHitRate() {
    Board board;
    board.setStartingPosition();
    
    std::cout << "Measuring Pawn Hash Cache Hit Rate\n";
    std::cout << "===================================\n\n";
    
    // Clear cache and counters
    g_pawnStructure.clear();
#ifdef DEBUG
    g_pawnStructure.m_cacheHits = 0;
    g_pawnStructure.m_cacheMisses = 0;
#endif
    
    // Simulate a short search
    SearchInfo searchInfo;
    searchInfo.depth = 5;
    searchInfo.maxDepth = 5;
    searchInfo.nodes = 0;
    searchInfo.startTime = std::chrono::steady_clock::now();
    searchInfo.timeLimit = std::chrono::milliseconds(1000);
    searchInfo.nodeLimit = 100000;
    searchInfo.quit = false;
    
    board.setSearchMode(true);
    
    // Perform search
    search::SearchResult result = search::searchPosition(board, searchInfo);
    
    board.setSearchMode(false);
    
#ifdef DEBUG
    size_t totalProbes = g_pawnStructure.m_cacheHits + g_pawnStructure.m_cacheMisses;
    double hitRate = totalProbes > 0 ? (100.0 * g_pawnStructure.m_cacheHits / totalProbes) : 0.0;
    
    std::cout << "Search Results:\n";
    std::cout << "  Depth: " << searchInfo.depth << "\n";
    std::cout << "  Nodes: " << searchInfo.nodes << "\n\n";
    
    std::cout << "Pawn Hash Statistics:\n";
    std::cout << "  Cache hits:   " << std::setw(8) << g_pawnStructure.m_cacheHits << "\n";
    std::cout << "  Cache misses: " << std::setw(8) << g_pawnStructure.m_cacheMisses << "\n";
    std::cout << "  Total probes: " << std::setw(8) << totalProbes << "\n";
    std::cout << "  Hit rate:     " << std::fixed << std::setprecision(2) << hitRate << "%\n\n";
    
    if (hitRate > 90.0) {
        std::cout << "✓ EXCELLENT: Cache hit rate > 90% indicates pawn hash is working efficiently!\n";
    } else if (hitRate > 70.0) {
        std::cout << "✓ GOOD: Cache hit rate > 70% shows pawn hash is helping.\n";
    } else if (hitRate > 50.0) {
        std::cout << "⚠ WARNING: Cache hit rate " << hitRate << "% is lower than expected.\n";
    } else {
        std::cout << "✗ ERROR: Cache hit rate " << hitRate << "% suggests pawn hash is not working!\n";
    }
#else
    std::cout << "Build in Debug mode to see cache statistics.\n";
    std::cout << "Use: cmake -DCMAKE_BUILD_TYPE=Debug ..\n";
#endif
}

int main() {
    measureCacheHitRate();
    return 0;
}