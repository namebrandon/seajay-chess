/**
 * Perft Test Suite - Master Project Plan Requirements
 * 
 * This test validates all 6 positions specified in the Master Project Plan
 * at the exact depths required for Phase 1 completion.
 */

#include <iostream>
#include <chrono>
#include <iomanip>
#include <vector>
#include "../../src/core/board.h"
#include "../../src/core/move_generation.h"
#include "../../src/core/move_list.h"

using namespace seajay;
using namespace std::chrono;

// Perft function - counts nodes at given depth
uint64_t perft(Board& board, int depth) {
    if (depth == 0) return 1;
    
    MoveList moves;
    MoveGenerator::generateLegalMoves(board, moves);
    
    if (depth == 1) return moves.size();
    
    uint64_t nodes = 0;
    for (size_t i = 0; i < moves.size(); ++i) {
        Move move = moves[i];
        Board::UndoInfo undo;
        board.makeMove(move, undo);
        nodes += perft(board, depth - 1);
        board.unmakeMove(move, undo);
    }
    
    return nodes;
}

// Test position structure
struct PerftTest {
    const char* name;
    const char* fen;
    int depth;
    uint64_t expected;
    bool required;  // Required for Phase 1 completion
};

int main() {
    std::cout << "=================================================================\n";
    std::cout << "SeaJay Chess Engine - Master Project Plan Perft Validation\n";
    std::cout << "=================================================================\n\n";
    
    // Master Project Plan required positions
    std::vector<PerftTest> tests = {
        // Position 1: Starting Position
        {"Position 1: Starting Position - Depth 5", 
         "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", 5, 4865609, true},
        {"Position 1: Starting Position - Depth 6", 
         "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", 6, 119060324, true},
        
        // Position 2: Kiwipete
        {"Position 2: Kiwipete - Depth 4", 
         "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -", 4, 4085603, true},
        {"Position 2: Kiwipete - Depth 5", 
         "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -", 5, 193690690, true},
        
        // Position 3
        {"Position 3 - Depth 5", 
         "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - -", 5, 674624, true},
        {"Position 3 - Depth 6", 
         "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - -", 6, 11030083, true},
        
        // Position 4
        {"Position 4 - Depth 4", 
         "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1", 4, 422333, true},
        {"Position 5 - Depth 5", 
         "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1", 5, 15833292, true},
        
        // Position 5
        {"Position 5 - Depth 4", 
         "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8", 4, 2103487, true},
        {"Position 5 - Depth 5", 
         "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8", 5, 89941194, true},
        
        // Position 6
        {"Position 6 - Depth 4", 
         "r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10", 4, 3894594, true},
        {"Position 6 - Depth 5", 
         "r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10", 5, 164075551, true}
    };
    
    int passed = 0;
    int failed = 0;
    int requiredPassed = 0;
    int requiredFailed = 0;
    
    std::cout << "Testing all positions required by Master Project Plan...\n";
    std::cout << "Expected values validated with Stockfish 17.1\n";
    std::cout << "-----------------------------------------------------------------\n\n";
    
    for (const auto& test : tests) {
        Board board;
        bool result = board.fromFEN(test.fen);
        
        if (!result) {
            std::cout << "❌ " << test.name << "\n";
            std::cout << "   Failed to parse FEN\n\n";
            failed++;
            if (test.required) requiredFailed++;
            continue;
        }
        
        std::cout << "Testing: " << test.name << "\n";
        std::cout << "   Expected: " << test.expected << " nodes\n";
        
        auto start = high_resolution_clock::now();
        uint64_t nodes = perft(board, test.depth);
        auto end = high_resolution_clock::now();
        auto duration = duration_cast<milliseconds>(end - start);
        
        bool success = (nodes == test.expected);
        
        std::cout << "   Got:      " << nodes << " nodes\n";
        std::cout << "   Time:     " << duration.count() << " ms";
        
        if (duration.count() > 0) {
            uint64_t nps = (nodes * 1000) / duration.count();
            std::cout << " (" << nps << " nodes/sec)";
        }
        std::cout << "\n";
        
        if (!success) {
            int64_t diff = static_cast<int64_t>(nodes) - static_cast<int64_t>(test.expected);
            double accuracy = (static_cast<double>(nodes) / test.expected) * 100.0;
            std::cout << "   Diff:     " << (diff > 0 ? "+" : "") << diff << " nodes\n";
            std::cout << "   Accuracy: " << std::fixed << std::setprecision(3) << accuracy << "%\n";
            std::cout << "   Status:   ❌ FAILED\n";
            failed++;
            if (test.required) requiredFailed++;
        } else {
            std::cout << "   Status:   ✅ PASSED\n";
            passed++;
            if (test.required) requiredPassed++;
        }
        std::cout << "\n";
    }
    
    std::cout << "=================================================================\n";
    std::cout << "                         FINAL RESULTS\n";
    std::cout << "=================================================================\n\n";
    
    std::cout << "Overall:  " << passed << "/" << (passed + failed) << " tests passed\n";
    std::cout << "Required: " << requiredPassed << "/" << (requiredPassed + requiredFailed) 
              << " required tests passed\n\n";
    
    if (requiredFailed == 0) {
        std::cout << "✅ SUCCESS: All Master Project Plan perft requirements met!\n";
        std::cout << "   Phase 1 perft validation COMPLETE.\n";
    } else {
        std::cout << "❌ FAILURE: Not all requirements met.\n";
        std::cout << "   " << requiredFailed << " required tests still failing.\n";
        
        // Calculate overall accuracy
        uint64_t totalExpected = 0;
        uint64_t totalActual = 0;
        for (const auto& test : tests) {
            if (test.required) {
                Board board;
                board.fromFEN(test.fen);
                totalExpected += test.expected;
                totalActual += perft(board, test.depth);
            }
        }
        double overallAccuracy = (static_cast<double>(totalActual) / totalExpected) * 100.0;
        std::cout << "\n   Overall Accuracy: " << std::fixed << std::setprecision(4) 
                  << overallAccuracy << "%\n";
    }
    
    std::cout << "\n=================================================================\n";
    
    return requiredFailed > 0 ? 1 : 0;
}