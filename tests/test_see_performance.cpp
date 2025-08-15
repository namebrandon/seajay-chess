#include <iostream>
#include <chrono>
#include <vector>
#include <iomanip>
#include "../src/core/see.h"
#include "../src/core/board.h"
#include "../src/core/move_generation.h"
#include "../src/core/bitboard.h"
#include "../src/core/magic_bitboards.h"

using namespace seajay;
using namespace std::chrono;

struct TestPosition {
    std::string fen;
    std::string description;
    int expectedMoves;  // Expected number of legal moves for validation
};

class SEEPerformanceTester {
public:
    struct BenchmarkResult {
        std::string description;
        size_t totalEvaluations;
        double totalTime;  // milliseconds
        double timePerEval;  // microseconds
        double evalsPerSecond;
    };
    
    void runBenchmark() {
        std::cout << "\n=== SEE Performance Benchmark ===\n" << std::endl;
        std::cout << "Establishing baseline performance before optimizations...\n" << std::endl;
        
        // Test positions covering various scenarios
        std::vector<TestPosition> positions = {
            {"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", "Starting position", 20},
            {"r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1", "Complex middlegame", 48},
            {"8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1", "Endgame position", 14},
            {"r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1", "Tactical position", 6},
            {"rnbqkb1r/pp1p1pPp/8/2p1pP2/1P1P4/3P3P/P1P1P3/RNBQKBNR w KQkq e6 0 1", "En passant position", 45},
            {"r2q1rk1/ppp2ppp/2n1bn2/2bpp3/3P4/3Q1N2/PPP1NPPP/R1B2RK1 b - - 0 1", "Center tension", 43}
        };
        
        std::vector<BenchmarkResult> results;
        size_t totalEvals = 0;
        double totalTime = 0;
        
        for (const auto& pos : positions) {
            Board board;
            if (!board.fromFEN(pos.fen)) {
                std::cerr << "Failed to parse FEN: " << pos.fen << std::endl;
                continue;
            }
            
            // Generate all legal moves
            MoveList moves;
            MoveGenerator::generateLegalMoves(board, moves);
            
            // Warm-up run
            for (int i = 0; i < 100; ++i) {
                for (const auto& move : moves) {
                    volatile SEEValue value = see(board, move);
                    (void)value;
                }
            }
            
            // Benchmark run - evaluate each move many times
            const int iterations = 10000;
            auto start = high_resolution_clock::now();
            
            for (int i = 0; i < iterations; ++i) {
                for (const auto& move : moves) {
                    volatile SEEValue value = see(board, move);
                    (void)value;
                }
            }
            
            auto end = high_resolution_clock::now();
            duration<double, std::milli> elapsed = end - start;
            
            size_t numEvals = moves.size() * iterations;
            double timeMs = elapsed.count();
            double timePerEval = (timeMs * 1000) / numEvals;  // microseconds
            double evalsPerSec = numEvals / (timeMs / 1000);
            
            results.push_back({
                pos.description,
                numEvals,
                timeMs,
                timePerEval,
                evalsPerSec
            });
            
            totalEvals += numEvals;
            totalTime += timeMs;
            
            std::cout << "Position: " << std::left << std::setw(20) << pos.description 
                      << " | Moves: " << std::setw(3) << moves.size()
                      << " | Time: " << std::fixed << std::setprecision(2) 
                      << std::setw(8) << timeMs << " ms"
                      << " | Per eval: " << std::setprecision(3) 
                      << std::setw(6) << timePerEval << " μs"
                      << " | Rate: " << std::setprecision(0)
                      << std::setw(10) << evalsPerSec << " evals/sec"
                      << std::endl;
        }
        
        // Summary
        std::cout << "\n=== Summary ===" << std::endl;
        std::cout << "Total evaluations: " << totalEvals << std::endl;
        std::cout << "Total time: " << std::fixed << std::setprecision(2) << totalTime << " ms" << std::endl;
        std::cout << "Average time per eval: " << std::setprecision(3) 
                  << (totalTime * 1000) / totalEvals << " μs" << std::endl;
        std::cout << "Evaluations per second: " << std::setprecision(0)
                  << totalEvals / (totalTime / 1000) << std::endl;
        
        // Check against target (< 500ms for 1M evaluations)
        double timeFor1M = (totalTime / totalEvals) * 1000000;
        std::cout << "\nProjected time for 1M evaluations: " 
                  << std::setprecision(2) << timeFor1M << " ms" << std::endl;
        
        if (timeFor1M < 500) {
            std::cout << "✓ MEETS performance target (< 500ms)" << std::endl;
        } else {
            std::cout << "✗ DOES NOT meet performance target (> 500ms)" << std::endl;
            std::cout << "  Need " << std::setprecision(1) 
                      << ((timeFor1M / 500) - 1) * 100 << "% improvement" << std::endl;
        }
    }
    
    void runDetailedProfiler() {
        std::cout << "\n=== Detailed SEE Profiling ===\n" << std::endl;
        
        // Profile different types of captures
        Board board;
        board.fromFEN("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1");
        
        MoveList moves;
        MoveGenerator::generateLegalMoves(board, moves);
        
        // Categorize moves
        std::vector<Move> quietMoves;
        std::vector<Move> equalCaptures;
        std::vector<Move> winningCaptures;
        std::vector<Move> losingCaptures;
        
        for (const auto& move : moves) {
            if (!board.pieceAt(moveTo(move))) {
                quietMoves.push_back(move);
            } else {
                SEEValue value = see(board, move);
                if (value > 0) winningCaptures.push_back(move);
                else if (value < 0) losingCaptures.push_back(move);
                else equalCaptures.push_back(move);
            }
        }
        
        // Profile each category
        profileCategory(board, "Quiet moves", quietMoves);
        profileCategory(board, "Equal captures", equalCaptures);
        profileCategory(board, "Winning captures", winningCaptures);
        profileCategory(board, "Losing captures", losingCaptures);
    }
    
private:
    void profileCategory(const Board& board, const std::string& category, 
                         const std::vector<Move>& moves) {
        if (moves.empty()) {
            std::cout << category << ": No moves in this category" << std::endl;
            return;
        }
        
        const int iterations = 100000;
        auto start = high_resolution_clock::now();
        
        for (int i = 0; i < iterations; ++i) {
            for (const auto& move : moves) {
                volatile SEEValue value = see(board, move);
                (void)value;
            }
        }
        
        auto end = high_resolution_clock::now();
        duration<double, std::milli> elapsed = end - start;
        
        size_t numEvals = moves.size() * iterations;
        double timeMs = elapsed.count();
        double timePerEval = (timeMs * 1000) / numEvals;
        
        std::cout << std::left << std::setw(20) << category 
                  << " | Count: " << std::setw(3) << moves.size()
                  << " | Time per eval: " << std::fixed << std::setprecision(3)
                  << std::setw(6) << timePerEval << " μs"
                  << " | Rate: " << std::setprecision(0)
                  << std::setw(10) << (numEvals / (timeMs / 1000)) << " evals/sec"
                  << std::endl;
    }
};

int main(int argc, char* argv[]) {
    // Initialize magic bitboards
    magic::initMagics();
    
    SEEPerformanceTester tester;
    
    // Run main benchmark
    tester.runBenchmark();
    
    // Run detailed profiling
    tester.runDetailedProfiler();
    
    std::cout << "\n=== Baseline Performance Established ===" << std::endl;
    std::cout << "This data will be used to measure optimization improvements." << std::endl;
    
    return 0;
}