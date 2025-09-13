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
        uint32_t rootMoves = 0;
        
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

    // Search-based benchmark (deterministic search signature)
    static BenchmarkResult runSearchBenchmark(int depth = 0, bool verbose = true);
    
    /**
     * Run perft on a single position
     * @param board The board to test
     * @param depth Perft depth
     * @return Result with nodes and time
     */
    static Result runPerft(Board& board, int depth);
    static Result runSearch(Board& board, int depth);

private:
    using Clock = std::chrono::steady_clock;

public:
    // Search benchmark positions with origin comments retained
    static constexpr std::array<Position, 29> SEARCH_POSITIONS = {{
        {"r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1", "kiwipete", 8},
        {"r7/1p1n1rpk/p1bBp2p/3p4/6p1/2N5/PPP2PPP/R3R1K1 b - - 5 24", "That time David beat me at Starbucks", 8},
        {"Q4bk1/1nB2r1p/4N1p1/3P1p1n/8/3pP2P/PP3PP1/R3K2R b KQ - 0 23", "That time I really kicked Ken's butt on chess.com", 8},
        {"r1b1k2r/pp4pp/3Bpp2/3p4/6q1/8/PQ3PPP/2R1R1K1 b kq - 3 17", "SeaJay problem position from back in the day", 8},
        {"6k1/2qr1pp1/2n1bb1p/1p2p3/2p1P3/2P1N2P/1PBNQPP1/R5K1 b - - 1 23", "Kolishkin - Karpov 1962", 8},
        {"r1b2b1r/ppppkPpp/4p3/3PP3/8/5N2/PPP1K2P/RNB4R b - - 0 13", "Early SeaJay Development Game", 8},
        {"r2rb1k1/pp1q1p1p/2n1p1p1/2bp4/5P2/PP1BPR1Q/1BPN2PP/R5K1 w - - 0 1", "WACNEW.014", 8},
        {"r1bqk2r/pppp1ppp/5n2/2b1n3/4P3/1BP3Q1/PP3PPP/RNB1K1NR b KQkq - 0 1", "WACNEW.056", 8},
        {"5rk1/p5pp/8/8/2Pbp3/1P4P1/7P/4RN1K b - - 0 1", "WACNEW.101", 8},
        {"r3kbnr/2q2ppp/2p1p3/P1PpP3/6b1/2N2N2/P1PB1PPP/R2QK2R b KQkq - 0 11", "Generic K-C game", 8},
        {"r4rk1/4qppp/p1p1n3/2ppPb2/8/BPN2P2/P1PQ2PP/R3R1K1 b - - 1 17", "Vujakociv - Karpov 1968", 8},
        {"rnb1k2r/ppp1bppp/3p1n2/6B1/8/2NP1N2/PPP1BPPP/R3K2R b KQkq - 2 9", "Addison - Karpov 1970", 8},
        {"8/4bk1p/1p3np1/p2r1p2/6n1/5N2/PP3PKP/2RRB3 b - - 1 29", "Petrosian - Karpov 1973", 8},
        {"5b2/rnp2pk1/1p2n1pp/4p3/4P3/P3BP2/1P3NPP/R4BK1 b - - 5 26", "Larsen - Fischer", 8},
        // Arasan 2023 additions (15 positions)
        {"r2r1k2/p3qpp1/1p1ppn1p/n5B1/P1PNbP2/2P3Q1/4B1PP/4RRK1 w - - 0 1", "Arasan 2023 #3 — Minic-Weiss, TCEC 2020", 8},
        {"R4bk1/2Bbp2p/2p2pp1/1rPp4/3P4/4P2P/4BPPK/1q1Q4 w - - 0 1", "Arasan 2023 #7 — Gurevich-Bareev, Cap d’Agde KO 2002", 8},
        {"1rb2k1r/2q2pp1/p2b3p/2n3B1/2QN4/3B4/PpP3PP/1K2R2R w - - 0 1", "Arasan 2023 #12 — Volokitin-Mamedyarov, EU Club Cup 2012", 8},
        {"r1q2rk1/ppnbbpp1/n4P1p/4P3/3p4/2N1B1PP/PP4BK/R2Q1R2 w - - 0 1", "Arasan 2023 #18 — Shirazi-Guichard, Malakoff op 2009", 8},
        {"3q1rk1/pr1b1p1p/1bp2p2/2ppP3/8/2P1BN2/PPQ3PP/R4RK1 w - - 0 1", "Arasan 2023 #20 — Shredder-Rybka, WCCC 2006", 8},
        {"2b2rk1/r3q1pp/1nn1p3/3pP1NP/p1pP2Q1/2P1N3/1P1KBP2/R5R1 w - - 0 1", "Arasan 2023 #24 — Vincent-Sebagh, corr FRA 2001", 8},
        {"2bq1rk1/rpb2p2/2p1p1p1/p1N3Np/P2P1P1P/1Q2R1P1/1P3P2/3R2K1 w - - 0 1", "Arasan 2023 #27 — Soloman-MarioDeMonti, Infinity Chess 2016", 8},
        {"2kr1b1r/1pp1ppp1/p7/q2P3n/2BB1pb1/2NQ4/P1P1N3/1R3RK1 w - - 0 1", "Arasan 2023 #31 — Critter-Rybka, Scandinavian thematic 2013", 8},
        {"br4k1/1qrnbppp/pp1ppn2/8/NPPBP3/PN3P2/5QPP/2RR1B1K w - - 0 1", "Arasan 2023 #33 — Anand-Illescas Cordoba, Linares 1992", 8},
        {"8/6p1/P1b1pp2/2p1p3/1k4P1/3PP3/1PK5/5B2 w - - 0 1", "Arasan 2023 #22 — Vincent Lejeune #36", 8},
        {"r4rk1/p4ppp/qp2p3/b5B1/n1R5/5N2/PP2QPPP/1R4K1 w - - 0 1", "Arasan 2023 #38 — Alekhine-Sterk, Budapest 1921", 8},
        {"7k/3q1pp1/1p3r2/p1bP4/P1P2p2/1P2rNpP/2Q3P1/4RR1K b - - 0 1", "Arasan 2023 #122 — Darkraider- Pastorale, playchess 2011", 8},
        {"1nr3k1/q4rpp/1p1p1n2/3Pp3/1PQ1P1b1/4B1P1/2R2NBP/2R3K1 w - - 0 1", "Arasan 2023 #129 — Runting-Barnsley, Purdy Jubilee 2003", 8},
        {"1r4k1/4pp1p/pp1pq1p1/r2R4/PbP1P3/1P1QBP1P/R5P1/7K w - - 0 1", "Arasan 2023 #124 — Almeida-Chronopoulos, ICCF 2022", 8},
        {"r1r3k1/1ppn2bp/p1q1p1p1/3pP3/3PB1P1/PQ3NP1/3N4/2BK3R w - - 0 1", "Arasan 2023 #133 — Torres-Graudins, LAT-USA ICCF 2015", 8},
    }};
};

} // namespace seajay
