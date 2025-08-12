/**
 * Magic Bitboards Performance Testing
 * Stage 10 - Phase 4A: Performance Benchmarking
 * 
 * This test suite measures and compares the performance of:
 * 1. Ray-based attack generation (baseline)
 * 2. Magic bitboards attack generation
 * 3. Move generation speed (perft)
 * 4. Cache performance characteristics
 */

#include <iostream>
#include <iomanip>
#include <chrono>
#include <vector>
#include <random>
#include <cstring>
#include "../src/core/bitboard.h"
#include "../src/core/board.h"
#include "../src/core/move_generation.h"
#include "../src/core/move_list.h"
#include "../src/core/magic_bitboards.h"
#include "../src/benchmark/benchmark.h"

using namespace seajay;
using Clock = std::chrono::high_resolution_clock;
using Duration = std::chrono::duration<double>;

// Performance test configuration
constexpr int WARMUP_ITERATIONS = 1000;
constexpr int TEST_ITERATIONS = 1000000;
constexpr int PERFT_TEST_DEPTH = 5;
constexpr int NUM_RANDOM_POSITIONS = 100;

/**
 * Generate random bitboard for occupancy testing
 */
Bitboard generateRandomOccupancy(std::mt19937& rng) {
    std::uniform_int_distribution<uint64_t> dist(0, UINT64_MAX);
    // Generate somewhat sparse occupancy (more realistic)
    return dist(rng) & dist(rng);
}

/**
 * Test raw attack generation speed - Ray-based
 */
double benchmarkRayAttacks(const std::vector<Bitboard>& occupancies) {
    // Warmup
    volatile Bitboard result = 0;
    for (int i = 0; i < WARMUP_ITERATIONS; ++i) {
        for (Square sq = A1; sq <= H8; ++sq) {
            result = rookAttacks(sq, occupancies[i % occupancies.size()]);
            result = bishopAttacks(sq, occupancies[i % occupancies.size()]);
        }
    }
    
    // Actual test
    auto start = Clock::now();
    for (int i = 0; i < TEST_ITERATIONS; ++i) {
        Bitboard occ = occupancies[i % occupancies.size()];
        for (Square sq = A1; sq <= H8; ++sq) {
            result = rookAttacks(sq, occ);
            result = bishopAttacks(sq, occ);
        }
    }
    auto end = Clock::now();
    
    Duration elapsed = end - start;
    return elapsed.count();
}

/**
 * Test raw attack generation speed - Magic bitboards
 */
double benchmarkMagicAttacks(const std::vector<Bitboard>& occupancies) {
    // Ensure magic tables are initialized
    magic::initMagics();
    
    // Warmup
    volatile Bitboard result = 0;
    for (int i = 0; i < WARMUP_ITERATIONS; ++i) {
        for (Square sq = A1; sq <= H8; ++sq) {
            result = magicRookAttacks(sq, occupancies[i % occupancies.size()]);
            result = magicBishopAttacks(sq, occupancies[i % occupancies.size()]);
        }
    }
    
    // Actual test
    auto start = Clock::now();
    for (int i = 0; i < TEST_ITERATIONS; ++i) {
        Bitboard occ = occupancies[i % occupancies.size()];
        for (Square sq = A1; sq <= H8; ++sq) {
            result = magicRookAttacks(sq, occ);
            result = magicBishopAttacks(sq, occ);
        }
    }
    auto end = Clock::now();
    
    Duration elapsed = end - start;
    return elapsed.count();
}

/**
 * Measure cache characteristics using different access patterns
 */
void benchmarkCachePerformance() {
    std::cout << "\n=== Cache Performance Analysis ===" << std::endl;
    
    std::mt19937 rng(12345);
    std::vector<Bitboard> occupancies;
    
    // Generate test occupancies
    for (int i = 0; i < 1000; ++i) {
        occupancies.push_back(generateRandomOccupancy(rng));
    }
    
    // Sequential access pattern
    auto start = Clock::now();
    volatile Bitboard result = 0;
    for (int iter = 0; iter < 100000; ++iter) {
        for (Square sq = A1; sq <= H8; ++sq) {
            result = magicRookAttacks(sq, occupancies[iter % occupancies.size()]);
        }
    }
    auto end = Clock::now();
    Duration sequential = end - start;
    
    // Random access pattern
    std::uniform_int_distribution<int> sqDist(0, 63);
    start = Clock::now();
    for (int iter = 0; iter < 100000; ++iter) {
        for (int i = 0; i < 64; ++i) {
            Square sq = static_cast<Square>(sqDist(rng));
            result = magicRookAttacks(sq, occupancies[iter % occupancies.size()]);
        }
    }
    end = Clock::now();
    Duration random = end - start;
    
    std::cout << "Sequential access: " << std::fixed << std::setprecision(6) 
              << sequential.count() << " seconds" << std::endl;
    std::cout << "Random access:     " << std::fixed << std::setprecision(6) 
              << random.count() << " seconds" << std::endl;
    std::cout << "Cache penalty:     " << std::fixed << std::setprecision(2) 
              << (random.count() / sequential.count() - 1.0) * 100 << "%" << std::endl;
}

/**
 * Perft-based move generation benchmark
 */
uint64_t perft(Board& board, int depth) {
    if (depth == 0) return 1;
    
    MoveList moves;
    MoveGenerator::generateLegalMoves(board, moves);
    
    if (depth == 1) return moves.size();
    
    uint64_t nodes = 0;
    for (Move move : moves) {
        Board::UndoInfo undo;
        board.makeMove(move, undo);
        nodes += perft(board, depth - 1);
        board.unmakeMove(move, undo);
    }
    
    return nodes;
}

/**
 * Compare perft performance between implementations
 */
void benchmarkPerftPerformance() {
    std::cout << "\n=== Perft Performance Comparison ===" << std::endl;
    
    // Test positions
    std::vector<std::pair<std::string, int>> testPositions = {
        {"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", 5},
        {"r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1", 4},
        {"8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1", 5}
    };
    
    for (const auto& [fen, depth] : testPositions) {
        Board board;
        board.fromFEN(fen);
        
        std::cout << "\nPosition: " << fen.substr(0, 30) << "..." << std::endl;
        std::cout << "Depth: " << depth << std::endl;
        
        // Run perft and measure time
        auto start = Clock::now();
        uint64_t nodes = perft(board, depth);
        auto end = Clock::now();
        
        Duration elapsed = end - start;
        double nps = nodes / elapsed.count();
        
        std::cout << "Nodes: " << nodes << std::endl;
        std::cout << "Time:  " << std::fixed << std::setprecision(3) 
                  << elapsed.count() << " seconds" << std::endl;
        std::cout << "NPS:   " << std::fixed << std::setprecision(0) 
                  << nps << std::endl;
    }
}

/**
 * Detailed attack generation benchmark
 */
void benchmarkAttackGeneration() {
    std::cout << "\n=== Attack Generation Performance ===" << std::endl;
    
    std::mt19937 rng(42);
    std::vector<Bitboard> occupancies;
    
    // Generate diverse occupancy patterns
    for (int i = 0; i < 100; ++i) {
        occupancies.push_back(generateRandomOccupancy(rng));
    }
    
    // Add specific patterns
    occupancies.push_back(0);  // Empty board
    occupancies.push_back(~0ULL);  // Full board
    occupancies.push_back(0xFF00000000000000ULL);  // Rank 8 occupied
    occupancies.push_back(0x00000000000000FFULL);  // Rank 1 occupied
    
    std::cout << "Testing with " << TEST_ITERATIONS << " iterations..." << std::endl;
    std::cout << "Operations per iteration: 128 (64 squares x 2 piece types)" << std::endl;
    
    // Ray-based benchmark
    std::cout << "\nRay-based attack generation:" << std::endl;
    double rayTime = benchmarkRayAttacks(occupancies);
    double rayOpsPerSec = (TEST_ITERATIONS * 128.0) / rayTime;
    std::cout << "Time: " << std::fixed << std::setprecision(6) << rayTime << " seconds" << std::endl;
    std::cout << "Operations/second: " << std::scientific << std::setprecision(2) 
              << rayOpsPerSec << std::endl;
    
    // Magic bitboards benchmark
    std::cout << "\nMagic bitboards attack generation:" << std::endl;
    double magicTime = benchmarkMagicAttacks(occupancies);
    double magicOpsPerSec = (TEST_ITERATIONS * 128.0) / magicTime;
    std::cout << "Time: " << std::fixed << std::setprecision(6) << magicTime << " seconds" << std::endl;
    std::cout << "Operations/second: " << std::scientific << std::setprecision(2) 
              << magicOpsPerSec << std::endl;
    
    // Comparison
    std::cout << "\n=== Performance Improvement ===" << std::endl;
    double speedup = rayTime / magicTime;
    std::cout << "Speedup: " << std::fixed << std::setprecision(2) << speedup << "x" << std::endl;
    std::cout << "Time saved: " << std::fixed << std::setprecision(2) 
              << ((1.0 - magicTime/rayTime) * 100) << "%" << std::endl;
}

/**
 * Run comprehensive benchmark suite using SeaJay's standard positions
 */
void runStandardBenchmark() {
    std::cout << "\n=== Standard Benchmark Suite ===" << std::endl;
    std::cout << "Running SeaJay benchmark with magic bitboards..." << std::endl;
    
    // Run standard benchmark at depth 4 for consistency
    BenchmarkSuite::BenchmarkResult result = BenchmarkSuite::runBenchmark(4, true);
    
    std::cout << "\nSummary:" << std::endl;
    std::cout << "Average NPS: " << std::fixed << std::setprecision(0) 
              << result.averageNps() << std::endl;
}

/**
 * Memory usage analysis
 */
void analyzeMemoryUsage() {
    std::cout << "\n=== Memory Usage Analysis ===" << std::endl;
    
    // Calculate memory used by magic tables
    size_t rookTableSize = 262144 * sizeof(Bitboard);  // 4096 * 64 squares
    size_t bishopTableSize = 32768 * sizeof(Bitboard);  // 512 * 64 squares
    size_t magicEntrySize = sizeof(magic::MagicEntry) * 128;  // 64 rook + 64 bishop
    
    size_t totalMemory = rookTableSize + bishopTableSize + magicEntrySize;
    
    std::cout << "Rook attack tables:   " << std::fixed << std::setprecision(2) 
              << rookTableSize / (1024.0 * 1024.0) << " MB" << std::endl;
    std::cout << "Bishop attack tables: " << std::fixed << std::setprecision(2) 
              << bishopTableSize / (1024.0 * 1024.0) << " MB" << std::endl;
    std::cout << "Magic entries:        " << std::fixed << std::setprecision(2) 
              << magicEntrySize / 1024.0 << " KB" << std::endl;
    std::cout << "Total memory:         " << std::fixed << std::setprecision(2) 
              << totalMemory / (1024.0 * 1024.0) << " MB" << std::endl;
}

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "   Magic Bitboards Performance Test    " << std::endl;
    std::cout << "      Stage 10 - Phase 4A              " << std::endl;
    std::cout << "========================================" << std::endl;
    
    // Initialize magic bitboards
    magic::initMagics();
    
    // Run all benchmarks
    analyzeMemoryUsage();
    benchmarkAttackGeneration();
    benchmarkCachePerformance();
    benchmarkPerftPerformance();
    runStandardBenchmark();
    
    std::cout << "\n========================================" << std::endl;
    std::cout << "Performance benchmarking complete!" << std::endl;
    std::cout << "========================================" << std::endl;
    
    return 0;
}