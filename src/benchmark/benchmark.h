#pragma once

#include <array>
#include <string_view>
#include <chrono>
#include <cstdint>
#include "../core/board.h"

namespace seajay {

/**
 * Benchmark Suite for SeaJay Chess Engine
 * 
 * Provides standardized performance testing using perft on a set of
 * carefully selected positions covering opening, middlegame, and endgame.
 */
class BenchmarkSuite {
public:
    struct Position {
        std::string_view fen;
        std::string_view description;
        int defaultDepth;  // Recommended perft depth for this position
    };
    
    struct Result {
        uint64_t nodes;
        std::chrono::nanoseconds time;
        
        double nps() const noexcept {
            if (time.count() == 0) return 0.0;
            return nodes * 1'000'000'000.0 / time.count();
        }
    };
    
    struct BenchmarkResult {
        std::vector<Result> positionResults;
        uint64_t totalNodes = 0;
        std::chrono::nanoseconds totalTime{0};
        
        double averageNps() const noexcept {
            if (totalTime.count() == 0) return 0.0;
            return totalNodes * 1'000'000'000.0 / totalTime.count();
        }
    };
    
    // Standard benchmark positions - mix of opening, middle, and endgame
    static constexpr std::array<Position, 12> POSITIONS = {{
        {"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", "Starting position", 5},
        {"r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1", "Kiwipete", 4},
        {"r1bqk1nr/pppp1ppp/2n5/2b1p3/2B1P3/5N2/PPPP1PPP/RNBQK2R w KQkq - 4 4", "Italian Game", 4},
        {"r1bqkbnr/pppp1ppp/2n5/1B2p3/4P3/5N2/PPPP1PPP/RNBQK2R b KQkq - 3 3", "Spanish", 4},
        {"8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1", "Endgame", 5},
        {"8/pp3p1k/2p2q1p/3r1P2/5R2/7P/P1P1QP2/7K b - - 0 1", "Queen endgame", 4},
        {"r1bq1rk1/pp2nppp/4n3/3ppP2/1b1P4/3BP3/PP2N1PP/R1BQNRK1 b - - 1 8", "Closed center", 4},
        {"4k3/8/8/8/8/8/4P3/4K3 w - - 0 1", "KP vs K", 6},
        {"r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10", "Complex middlegame", 3},
        {"8/k7/3p4/p2P1p2/P2P1P2/8/8/K7 w - - 0 1", "Pawn endgame", 5},
        {"r2q1rk1/ppp2ppp/2n1bn2/2bpp3/3P4/3QPN2/PPP1BPPP/R1B1K2R w KQ - 0 8", "Ruy Lopez", 4},
        {"rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8", "Position 5", 4}
    }};
    
    /**
     * Run the standard benchmark suite
     * @param depth Override depth (0 = use default depths for each position)
     * @param verbose Print progress for each position
     * @return Benchmark results
     */
    static BenchmarkResult runBenchmark(int depth = 0, bool verbose = true);
    
    /**
     * Run perft on a single position
     * @param board The board to test
     * @param depth Perft depth
     * @return Result with nodes and time
     */
    static Result runPerft(Board& board, int depth);
    
private:
    using Clock = std::chrono::steady_clock;
};

} // namespace seajay