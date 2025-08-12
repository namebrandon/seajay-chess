/**
 * Quick Performance Test for Magic Bitboards
 * Stage 10 - Phase 4A: Performance Validation
 */

#include <iostream>
#include <iomanip>
#include <chrono>
#include <vector>
#include "../src/core/board.h"
#include "../src/core/move_generation.h"
#include "../src/core/move_list.h"
#include "../src/core/magic_bitboards.h"

using namespace seajay;
using Clock = std::chrono::high_resolution_clock;

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

int main() {
    std::cout << "Magic Bitboards - Quick Performance Test\n";
    std::cout << "=========================================\n\n";
    
    // Initialize magic bitboards
    magic::initMagics();
    
    // Test positions with expected results
    struct TestPosition {
        std::string fen;
        std::string name;
        int depth;
        uint64_t expected;
    };
    
    std::vector<TestPosition> positions = {
        {"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", "Starting", 5, 4865609},
        {"r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1", "Kiwipete", 3, 97862},
        {"8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1", "Endgame", 4, 43238}
    };
    
    double totalTime = 0;
    uint64_t totalNodes = 0;
    
    for (const auto& pos : positions) {
        Board board;
        board.fromFEN(pos.fen);
        
        std::cout << "Testing: " << pos.name << " (depth " << pos.depth << ")\n";
        
        auto start = Clock::now();
        uint64_t nodes = perft(board, pos.depth);
        auto end = Clock::now();
        
        std::chrono::duration<double> elapsed = end - start;
        double nps = nodes / elapsed.count();
        
        totalTime += elapsed.count();
        totalNodes += nodes;
        
        std::cout << "  Nodes: " << nodes;
        if (nodes == pos.expected) {
            std::cout << " ✓";
        } else {
            std::cout << " (expected " << pos.expected << ")";
        }
        std::cout << "\n";
        std::cout << "  Time:  " << std::fixed << std::setprecision(3) << elapsed.count() << "s\n";
        std::cout << "  NPS:   " << std::fixed << std::setprecision(0) << nps << "\n\n";
    }
    
    std::cout << "=========================================\n";
    std::cout << "Total nodes: " << totalNodes << "\n";
    std::cout << "Total time:  " << std::fixed << std::setprecision(3) << totalTime << "s\n";
    std::cout << "Average NPS: " << std::fixed << std::setprecision(0) << (totalNodes / totalTime) << "\n";
    
    return 0;
}