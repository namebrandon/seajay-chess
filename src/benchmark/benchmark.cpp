#include "benchmark.h"
#include "../core/move_generation.h"
#include "../core/move_list.h"
#include "../search/negamax.h"
#include "../core/transposition_table.h"
#include "../search/types.h"
#include "../search/countermove_history.h"
#include <iostream>
#include <iomanip>

namespace seajay {

// Forward declaration of perft function
static uint64_t perft(Board& board, int depth);

BenchmarkSuite::BenchmarkResult BenchmarkSuite::runBenchmark(int overrideDepth, bool verbose) {
    BenchmarkResult result;
    
    if (verbose) {
        std::cout << "\n=================================================\n";
        std::cout << "SeaJay Chess Engine - Benchmark Suite\n";
        std::cout << "=================================================\n\n";
    }
    
    for (size_t i = 0; i < POSITIONS.size(); ++i) {
        const auto& pos = POSITIONS[i];
        
        // Parse FEN and set up board
        Board board;
        bool parseResult = board.fromFEN(std::string(pos.fen));
        if (!parseResult) {
            std::cerr << "Error parsing position " << (i + 1) << std::endl;
            continue;
        }
        
        // Use override depth if provided, otherwise use position's default
        int depth = (overrideDepth > 0) ? overrideDepth : pos.defaultDepth;
        
        if (verbose) {
            std::cout << "Position " << std::setw(2) << (i + 1) << "/12: " 
                     << pos.description << "\n";
            std::cout << "FEN: " << pos.fen << "\n";
            std::cout << "Depth: " << depth << " - ";
            std::cout.flush();
        }
        
        // Run perft and measure time
        Result posResult = runPerft(board, depth);
        result.positionResults.push_back(posResult);
        result.totalNodes += posResult.nodes;
        result.totalTime += posResult.time;
        
        if (verbose) {
            double timeSeconds = posResult.time.count() / 1'000'000'000.0;
            std::cout << posResult.nodes << " nodes in " 
                     << std::fixed << std::setprecision(3) << timeSeconds << "s ("
                     << std::fixed << std::setprecision(0) << posResult.nps() 
                     << " nps)\n\n";
        }
    }
    
    if (verbose) {
        std::cout << "=================================================\n";
        std::cout << "Total: " << result.totalNodes << " nodes in "
                 << std::fixed << std::setprecision(3) 
                 << (result.totalTime.count() / 1'000'000'000.0) << "s ("
                 << std::fixed << std::setprecision(0) << result.averageNps() 
                 << " nps)\n";
        std::cout << "=================================================\n";
    }
    
    return result;
}

BenchmarkSuite::BenchmarkResult BenchmarkSuite::runSearchBenchmark(int overrideDepth, bool verbose) {
    BenchmarkResult result;

    if (verbose) {
        std::cout << "\n=================================================\n";
        std::cout << "SeaJay Chess Engine - Search Benchmark\n";
        std::cout << "=================================================\n\n";
        std::cout << "Depth: " << ((overrideDepth > 0) ? overrideDepth : SEARCH_POSITIONS[0].defaultDepth) << "\n\n";
    }

    // Compute formatting widths for aligned output
    size_t fenWidth = 0;
    for (const auto& p : SEARCH_POSITIONS) {
        fenWidth = std::max(fenWidth, std::string(p.fen).size());
    }
    const int idxWidth = static_cast<int>(std::to_string(SEARCH_POSITIONS.size()).size());

    struct BenchLine { std::string fen; bool ok; uint64_t nodes; uint32_t rootMoves; long long ms; };
    std::vector<BenchLine> lines;

    for (size_t i = 0; i < SEARCH_POSITIONS.size(); ++i) {
        const auto& pos = SEARCH_POSITIONS[i];

        Board board;
        bool fenOk = board.fromFEN(std::string(pos.fen));

        int depth = (overrideDepth > 0) ? overrideDepth : pos.defaultDepth;

        Result r;
        if (fenOk) {
            r = runSearch(board, depth);
        } else {
            r.nodes = 0;
            r.time = std::chrono::nanoseconds{0};
        }
        result.positionResults.push_back(r);
        result.totalNodes += r.nodes;
        result.totalTime += r.time;

        // Store for aligned printing later
        lines.push_back(BenchLine{std::string(pos.fen), fenOk, r.nodes, r.rootMoves,
                                  std::chrono::duration_cast<std::chrono::milliseconds>(r.time).count()});
    }

    if (verbose) {
        // Determine widths for numeric columns
        size_t nodesWidth = 0;
        size_t rootWidth = 0;
        size_t timeWidth = 0;
        for (const auto& ln : lines) {
            nodesWidth = std::max(nodesWidth, std::to_string(ln.nodes).size());
            rootWidth = std::max(rootWidth, std::to_string(ln.rootMoves).size());
            timeWidth = std::max(timeWidth, std::to_string(ln.ms).size());
        }
        // Print aligned lines
        for (size_t i = 0; i < lines.size(); ++i) {
            const auto& ln = lines[i];
            std::cout << "POSITION " << std::setw(idxWidth) << (i + 1) << ": "
                      << std::left << std::setw(static_cast<int>(fenWidth)) << ln.fen << std::right
                      << "  " << (ln.ok ? "[OK]" : "[ERR]")
                      << "  NODES: " << std::setw(static_cast<int>(nodesWidth)) << ln.nodes
                      << "  ROOT MOVES: " << std::setw(static_cast<int>(rootWidth)) << ln.rootMoves
                      << "  TIME: " << std::setw(static_cast<int>(timeWidth)) << ln.ms << " ms" << std::endl;
        }
        std::cout << "=================================================\n";
        auto totalMs = std::chrono::duration_cast<std::chrono::milliseconds>(result.totalTime).count();
        std::cout << "Total: " << result.totalNodes << " nodes in "
                  << totalMs << " ms ("
                  << std::fixed << std::setprecision(0) << result.averageNps()
                  << " nps)\n";
        std::cout << "=================================================\n";
    }

    return result;
}

BenchmarkSuite::Result BenchmarkSuite::runPerft(Board& board, int depth) {
    Result result;
    
    auto start = Clock::now();
    result.nodes = perft(board, depth);
    auto end = Clock::now();
    
    result.time = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
    
    return result;
}

// Local perft implementation
static uint64_t perft(Board& board, int depth) {
    if (depth == 0) return 1;
    
    MoveList moves;
    MoveGenerator::generateLegalMoves(board, moves);
    
    // Bulk counting optimization for depth 1
    if (depth == 1) {
        return moves.size();
    }
    
    uint64_t nodes = 0;
    for (Move move : moves) {
        Board::UndoInfo undo;
        board.makeMove(move, undo);
        nodes += perft(board, depth - 1);
        board.unmakeMove(move, undo);
    }
    
    return nodes;
}

BenchmarkSuite::Result BenchmarkSuite::runSearch(Board& board, int depth) {
    Result r{0, std::chrono::nanoseconds{0}};

    using namespace seajay::search;

    // Prepare deterministic limits
    SearchLimits limits;
    limits.maxDepth = depth;
    limits.useQuiescence = true;
    limits.useAspirationWindows = false; // negamax-only path; not used
    limits.stopFlag = nullptr;
    limits.useRankedMovePicker = true;
    limits.useInCheckClassOrdering = true;
    limits.useRankAwareGates = true;
    limits.suppressDebugOutput = true; // Silence debug during bench

    // Thread-local tables
    KillerMoves killers;
    HistoryHeuristic history;
    CounterMoves counterMoves;
    std::unique_ptr<CounterMoveHistory> cmh = std::make_unique<CounterMoveHistory>();

    SearchInfo searchInfo;
    searchInfo.clear();
    searchInfo.setRootHistorySize(board.gameHistorySize());

    SearchData info;
    info.reset();
    info.killers = &killers;
    info.history = &history;
    info.counterMoves = &counterMoves;
    info.counterMoveHistory = cmh.get();
    info.useQuiescence = limits.useQuiescence;
    info.timeLimit = std::chrono::milliseconds::max(); // never stop on time
    info.rootSideToMove = board.sideToMove();

    // Local TT per position for determinism
    TranspositionTable tt;
    tt.setEnabled(true);
    tt.setClustered(true);
    tt.resize(128);
    tt.newSearch();

    // Precompute root moves for reporting
    MoveList rootMoves;
    MoveGenerator::generateLegalMoves(board, rootMoves);
    r.rootMoves = static_cast<uint32_t>(rootMoves.size());

    auto start = Clock::now();
    // Full-window negamax at fixed depth
    (void)negamax(board, depth, 0,
                  eval::Score::minus_infinity(), eval::Score::infinity(),
                  searchInfo, info, limits, &tt, nullptr, true);
    auto end = Clock::now();

    // Include qsearch nodes for total searched signature
    r.nodes = info.nodes + info.qsearchNodes;
    r.time = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
    return r;
}

} // namespace seajay
