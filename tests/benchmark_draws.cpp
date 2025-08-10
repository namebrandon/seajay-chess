#include "../src/core/board.h"
#include "../src/core/move_generation.h"
#include <chrono>
#include <iostream>
#include <iomanip>
#include <vector>
#include <functional>
#include <algorithm>

using namespace seajay;

class DrawBenchmark {
private:
    struct BenchmarkResult {
        std::string testName;
        int iterations;
        double totalTime;
        double avgTime;
        double checksPerSecond;
    };
    
    std::vector<BenchmarkResult> results;
    
    void runBenchmark(const std::string& name, int iterations, 
                     const std::function<void(Board&)>& setup,
                     const std::function<void(Board&)>& operation) {
        Board board;
        setup(board);
        
        // Warmup
        for (int i = 0; i < 100; ++i) {
            operation(board);
        }
        
        auto start = std::chrono::high_resolution_clock::now();
        
        for (int i = 0; i < iterations; ++i) {
            operation(board);
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> elapsed = end - start;
        
        BenchmarkResult result;
        result.testName = name;
        result.iterations = iterations;
        result.totalTime = elapsed.count();
        result.avgTime = (elapsed.count() / iterations) * 1000; // microseconds
        result.checksPerSecond = (iterations / (elapsed.count() / 1000.0));
        
        results.push_back(result);
        
        std::cout << std::left << std::setw(35) << name << " | "
                  << std::right << std::setw(8) << iterations << " iter | "
                  << std::fixed << std::setprecision(2) << std::setw(8) << elapsed.count() << " ms | "
                  << std::setprecision(3) << std::setw(8) << result.avgTime << " µs/op | "
                  << std::setprecision(0) << std::setw(10) << result.checksPerSecond << " ops/sec"
                  << std::endl;
    }
    
public:
    void benchmarkDrawDetection() {
        std::cout << "=== SeaJay Draw Detection Performance Benchmark ===" << std::endl;
        std::cout << std::endl;
        std::cout << "Test Name                           |    Iters |     Time |   Avg/op |    Ops/sec" << std::endl;
        std::cout << "-------------------------------------------------------------------------------------" << std::endl;
        
        // Benchmark 1: isDraw() on starting position
        runBenchmark("isDraw() - Starting Position", 100000,
            [](Board& b) { b.setStartingPosition(); },
            [](Board& b) { b.isDraw(); }
        );
        
        // Benchmark 2: isDraw() on endgame position
        runBenchmark("isDraw() - K vs K", 100000,
            [](Board& b) { b.fromFEN("8/8/8/4k3/8/3K4/8/8 w - - 0 1"); },
            [](Board& b) { b.isDraw(); }
        );
        
        // Benchmark 3: Repetition check with empty history
        runBenchmark("isRepetitionDraw() - Empty History", 100000,
            [](Board& b) { 
                b.setStartingPosition(); 
                b.clearGameHistory();
            },
            [](Board& b) { b.isRepetitionDraw(); }
        );
        
        // Benchmark 4: Repetition check with full history
        runBenchmark("isRepetitionDraw() - Full History", 100000,
            [](Board& b) { 
                b.setStartingPosition();
                b.clearGameHistory();
                // Add 50 positions to history
                for (int i = 0; i < 50; ++i) {
                    b.pushGameHistory();
                }
            },
            [](Board& b) { b.isRepetitionDraw(); }
        );
        
        // Benchmark 5: Fifty-move rule check
        runBenchmark("isFiftyMoveRule() - At 50", 100000,
            [](Board& b) { b.fromFEN("8/8/8/4k3/8/3K4/8/8 w - - 50 1"); },
            [](Board& b) { b.isFiftyMoveRule(); }
        );
        
        runBenchmark("isFiftyMoveRule() - At 99", 100000,
            [](Board& b) { b.fromFEN("8/8/8/4k3/8/3K4/8/8 w - - 99 1"); },
            [](Board& b) { b.isFiftyMoveRule(); }
        );
        
        runBenchmark("isFiftyMoveRule() - At 100", 100000,
            [](Board& b) { b.fromFEN("8/8/8/4k3/8/3K4/8/8 w - - 100 1"); },
            [](Board& b) { b.isFiftyMoveRule(); }
        );
        
        // Benchmark 6: Insufficient material checks
        runBenchmark("isInsufficientMaterial() - K vs K", 100000,
            [](Board& b) { b.fromFEN("8/8/8/4k3/8/3K4/8/8 w - - 0 1"); },
            [](Board& b) { b.isInsufficientMaterial(); }
        );
        
        runBenchmark("isInsufficientMaterial() - KN vs K", 100000,
            [](Board& b) { b.fromFEN("8/8/8/4k3/8/3K4/8/N7 w - - 0 1"); },
            [](Board& b) { b.isInsufficientMaterial(); }
        );
        
        runBenchmark("isInsufficientMaterial() - KB vs KB", 100000,
            [](Board& b) { b.fromFEN("8/8/8/4k3/2b5/8/B7/3K4 w - - 0 1"); },
            [](Board& b) { b.isInsufficientMaterial(); }
        );
        
        runBenchmark("isInsufficientMaterial() - Complex", 100000,
            [](Board& b) { b.fromFEN("r1bqkbnr/pppppppp/2n5/8/4P3/5N2/PPPP1PPP/RNBQKB1R w KQkq - 0 1"); },
            [](Board& b) { b.isInsufficientMaterial(); }
        );
        
        // Benchmark 7: Combined test - all draw checks
        runBenchmark("All Draw Checks - Starting Pos", 25000,
            [](Board& b) { 
                b.setStartingPosition();
                b.clearGameHistory();
            },
            [](Board& b) { 
                b.isDraw();
                b.isRepetitionDraw();
                b.isFiftyMoveRule();
                b.isInsufficientMaterial();
            }
        );
        
        // Benchmark 8: Make/unmake with draw detection
        runBenchmark("makeMove + isDraw() Check", 10000,
            [](Board& b) { b.setStartingPosition(); },
            [](Board& b) { 
                MoveList moves = generateLegalMoves(b);
                if (!moves.empty()) {
                    Move move = moves[0];
                    Board::UndoInfo undo;
                    b.makeMove(move, undo);
                    b.isDraw();
                    b.unmakeMove(move, undo);
                }
            }
        );
        
        std::cout << std::endl;
        printSummary();
    }
    
    void printSummary() {
        std::cout << "=== Performance Summary ===" << std::endl;
        
        double totalChecks = 0;
        double totalTime = 0;
        
        for (const auto& result : results) {
            totalChecks += result.iterations;
            totalTime += result.totalTime;
        }
        
        std::cout << "Total operations: " << std::fixed << std::setprecision(0) << totalChecks << std::endl;
        std::cout << "Total time: " << std::setprecision(2) << totalTime << " ms" << std::endl;
        std::cout << "Average throughput: " << std::setprecision(0) 
                  << (totalChecks / (totalTime / 1000.0)) << " operations/second" << std::endl;
        
        // Find fastest and slowest operations
        if (!results.empty()) {
            auto fastest = std::min_element(results.begin(), results.end(),
                [](const BenchmarkResult& a, const BenchmarkResult& b) {
                    return a.avgTime < b.avgTime;
                });
            auto slowest = std::max_element(results.begin(), results.end(),
                [](const BenchmarkResult& a, const BenchmarkResult& b) {
                    return a.avgTime < b.avgTime;
                });
            
            std::cout << std::endl;
            std::cout << "Fastest operation: " << fastest->testName 
                      << " (" << std::setprecision(3) << fastest->avgTime << " µs/op)" << std::endl;
            std::cout << "Slowest operation: " << slowest->testName 
                      << " (" << std::setprecision(3) << slowest->avgTime << " µs/op)" << std::endl;
        }
        
        // Check if we meet performance requirements
        std::cout << std::endl;
        bool meetsRequirements = true;
        for (const auto& result : results) {
            if (result.testName.find("isDraw()") != std::string::npos) {
                if (result.avgTime > 5.0) {  // 5 microseconds per check
                    std::cout << "⚠️  Warning: " << result.testName 
                              << " is slower than target (5µs)" << std::endl;
                    meetsRequirements = false;
                }
            }
        }
        
        if (meetsRequirements) {
            std::cout << "✓ All operations meet performance requirements!" << std::endl;
        } else {
            std::cout << "Performance optimization may be needed for some operations." << std::endl;
        }
    }
};

int main() {
    std::cout << "SeaJay Chess Engine - Draw Detection Performance Benchmark" << std::endl;
    std::cout << "==========================================================" << std::endl;
    std::cout << std::endl;
    
    DrawBenchmark benchmark;
    benchmark.benchmarkDrawDetection();
    
    return 0;
}