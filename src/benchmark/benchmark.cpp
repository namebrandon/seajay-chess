#include "benchmark.h"
#include "../core/move_generation.h"
#include "../core/move_list.h"
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

} // namespace seajay