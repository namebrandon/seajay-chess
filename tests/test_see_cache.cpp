#include <iostream>
#include <chrono>
#include <vector>
#include <iomanip>
#include "../src/core/see.h"
#include "../src/core/board.h"
#include "../src/core/move_generation.h"
#include "../src/core/magic_bitboards.h"

using namespace seajay;
using namespace std::chrono;

class SEECacheTester {
public:
    void testCacheEffectiveness() {
        std::cout << "\n=== SEE Cache Effectiveness Test ===\n" << std::endl;
        
        // Reset statistics
        g_seeCalculator.resetStatistics();
        g_seeCalculator.clearCache();
        
        Board board;
        board.fromFEN("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1");
        
        MoveList moves;
        MoveGenerator::generateLegalMoves(board, moves);
        
        // First pass - cold cache
        auto start = high_resolution_clock::now();
        for (int i = 0; i < 1000; ++i) {
            for (const auto& move : moves) {
                volatile SEEValue value = see(board, move);
                (void)value;
            }
        }
        auto end = high_resolution_clock::now();
        duration<double, std::milli> coldTime = end - start;
        
        const auto& stats1 = g_seeCalculator.statistics();
        std::cout << "Cold cache run:" << std::endl;
        std::cout << "  Time: " << std::fixed << std::setprecision(2) << coldTime.count() << " ms" << std::endl;
        std::cout << "  Calls: " << stats1.calls << std::endl;
        std::cout << "  Cache hits: " << stats1.cacheHits << std::endl;
        std::cout << "  Cache misses: " << stats1.cacheMisses << std::endl;
        std::cout << "  Hit rate: " << std::setprecision(1) << stats1.hitRate() << "%" << std::endl;
        std::cout << "  Early exits: " << stats1.earlyExits << std::endl;
        std::cout << "  Lazy evals: " << stats1.lazyEvals << std::endl;
        std::cout << "  X-ray checks: " << stats1.xrayChecks << std::endl;
        
        // Second pass - warm cache (should be much faster)
        g_seeCalculator.resetStatistics();
        
        start = high_resolution_clock::now();
        for (int i = 0; i < 1000; ++i) {
            for (const auto& move : moves) {
                volatile SEEValue value = see(board, move);
                (void)value;
            }
        }
        end = high_resolution_clock::now();
        duration<double, std::milli> warmTime = end - start;
        
        const auto& stats2 = g_seeCalculator.statistics();
        std::cout << "\nWarm cache run:" << std::endl;
        std::cout << "  Time: " << std::fixed << std::setprecision(2) << warmTime.count() << " ms" << std::endl;
        std::cout << "  Calls: " << stats2.calls << std::endl;
        std::cout << "  Cache hits: " << stats2.cacheHits << std::endl;
        std::cout << "  Cache misses: " << stats2.cacheMisses << std::endl;
        std::cout << "  Hit rate: " << std::setprecision(1) << stats2.hitRate() << "%" << std::endl;
        
        double speedup = coldTime.count() / warmTime.count();
        std::cout << "\nCache speedup: " << std::setprecision(2) << speedup << "x" << std::endl;
        
        if (stats2.hitRate() > 30) {
            std::cout << "✓ Cache hit rate target met (>30%)" << std::endl;
        } else {
            std::cout << "✗ Cache hit rate below target (<30%)" << std::endl;
        }
    }
    
    void testCacheCorrectness() {
        std::cout << "\n=== SEE Cache Correctness Test ===\n" << std::endl;
        
        Board board;
        board.fromFEN("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1");
        
        MoveList moves;
        MoveGenerator::generateLegalMoves(board, moves);
        
        // Store results without cache
        g_seeCalculator.clearCache();
        std::vector<SEEValue> noCacheResults;
        for (const auto& move : moves) {
            noCacheResults.push_back(see(board, move));
        }
        
        // Get results with cache (second call should hit cache)
        std::vector<SEEValue> cacheResults;
        for (const auto& move : moves) {
            cacheResults.push_back(see(board, move));
        }
        
        // Compare results
        bool allMatch = true;
        for (size_t i = 0; i < moves.size(); ++i) {
            if (noCacheResults[i] != cacheResults[i]) {
                std::cout << "Mismatch at move " << i << ": "
                          << "no-cache=" << noCacheResults[i] 
                          << " cached=" << cacheResults[i] << std::endl;
                allMatch = false;
            }
        }
        
        if (allMatch) {
            std::cout << "✓ All " << moves.size() << " moves produce identical results with cache" << std::endl;
        } else {
            std::cout << "✗ Cache produces different results!" << std::endl;
        }
    }
    
    void testMultiPosition() {
        std::cout << "\n=== Multi-Position Cache Test ===\n" << std::endl;
        
        std::vector<std::string> positions = {
            "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
            "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1",
            "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1",
            "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1"
        };
        
        g_seeCalculator.resetStatistics();
        g_seeCalculator.clearCache();
        
        size_t totalMoves = 0;
        for (const auto& fen : positions) {
            Board board;
            board.fromFEN(fen);
            
            MoveList moves;
            MoveGenerator::generateLegalMoves(board, moves);
            totalMoves += moves.size();
            
            // Evaluate each move 3 times
            for (int repeat = 0; repeat < 3; ++repeat) {
                for (const auto& move : moves) {
                    volatile SEEValue value = see(board, move);
                    (void)value;
                }
            }
        }
        
        const auto& stats = g_seeCalculator.statistics();
        std::cout << "Positions tested: " << positions.size() << std::endl;
        std::cout << "Total unique moves: " << totalMoves << std::endl;
        std::cout << "Total SEE calls: " << stats.calls << std::endl;
        std::cout << "Cache hits: " << stats.cacheHits << std::endl;
        std::cout << "Cache hit rate: " << std::fixed << std::setprecision(1) 
                  << stats.hitRate() << "%" << std::endl;
        
        // Expected: 2/3 of calls should be cache hits (since we call each 3 times)
        double expectedHitRate = 66.67;
        if (stats.hitRate() > expectedHitRate - 5) {
            std::cout << "✓ Cache working efficiently across positions" << std::endl;
        } else {
            std::cout << "✗ Cache hit rate lower than expected" << std::endl;
        }
    }
};

int main() {
    // Initialize magic bitboards
    magic::initMagics();
    
    SEECacheTester tester;
    
    // Test cache correctness first
    tester.testCacheCorrectness();
    
    // Test cache effectiveness
    tester.testCacheEffectiveness();
    
    // Test across multiple positions
    tester.testMultiPosition();
    
    std::cout << "\n=== SEE Cache Testing Complete ===" << std::endl;
    
    return 0;
}