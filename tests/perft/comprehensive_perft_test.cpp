// SeaJay Chess Engine - Comprehensive Perft Test Suite
// Tests all required positions per Master Project Plan

#include <iostream>
#include <chrono>
#include <iomanip>
#include <vector>
#include <string>
#include "../../src/core/board.h"
#include "../../src/core/move_generation.h"
#include "../../src/core/move_list.h"

using namespace seajay;
using namespace std::chrono;

struct PerftPosition {
    std::string name;
    std::string fen;
    std::vector<uint64_t> expected;  // Expected node counts at each depth
};

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

int main() {
    std::cout << "\n" << std::string(80, '=') << "\n";
    std::cout << "SEAJAY CHESS ENGINE - COMPREHENSIVE PERFT TEST SUITE\n";
    std::cout << "Phase 1 Move Generation Validation Requirements\n";
    std::cout << std::string(80, '=') << "\n\n";
    
    // All positions required by Master Project Plan
    std::vector<PerftPosition> positions = {
        {
            "Starting Position",
            "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
            {20, 400, 8902, 197281, 4865609, 119060324}  // Depth 1-6
        },
        {
            "Kiwipete",
            "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1",
            {48, 2039, 97862, 4085603, 193690690}  // Depth 1-5
        },
        {
            "Position 3 (Promotions)",
            "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1",
            {14, 191, 2812, 43238, 674624, 11030083}  // Depth 1-6
        },
        {
            "Position 4 (Check Evasions)",
            "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1",
            {6, 264, 9467, 422333, 15833292}  // Depth 1-5
        },
        {
            "Position 5 (Middle Game)",
            "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8",
            {44, 1486, 62379, 2103487, 89941194}  // Depth 1-5
        },
        {
            "Position 6 (En Passant & Complex)",
            "r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10",
            {46, 2079, 89890, 3894594, 164075551}  // Depth 1-5 (Stockfish validated)
        },
        {
            "Edwards Position (Castling Edge Cases)",
            "r4rk1/2p2ppp/p7/q2Pp3/1n2P1n1/4QP2/PPP3PP/R1B1K2R w KQ - 0 1",
            {43, 1610, 69515, 2516598}  // Depth 1-4
        },
        {
            "Empty Board (Edge Case)",
            "8/8/8/8/8/8/8/8 w - - 0 1",
            {0}  // No moves possible
        },
        {
            "Single King (Endgame)",
            "8/8/8/4k3/8/8/8/4K3 w - - 0 1",
            {8, 64, 393, 3136}  // Depth 1-4
        },
        {
            "Two Kings with Pawns",
            "8/2p5/8/KP6/8/8/8/k7 w - - 0 1",
            {5, 39, 237, 2002}  // Depth 1-4
        }
    };
    
    int totalTests = 0;
    int passedTests = 0;
    int failedTests = 0;
    
    std::cout << "Running perft tests on " << positions.size() << " positions...\n\n";
    
    for (const auto& pos : positions) {
        std::cout << "Testing: " << pos.name << "\n";
        std::cout << "FEN: " << pos.fen << "\n";
        std::cout << std::string(60, '-') << "\n";
        
        Board board;
        if (!board.fromFEN(pos.fen)) {
            std::cout << "❌ Failed to parse FEN!\n\n";
            failedTests++;
            continue;
        }
        
        bool positionPassed = true;
        
        for (size_t depth = 1; depth <= pos.expected.size(); ++depth) {
            totalTests++;
            
            auto start = high_resolution_clock::now();
            uint64_t result = perft(board, depth);
            auto end = high_resolution_clock::now();
            auto duration = duration_cast<milliseconds>(end - start);
            
            uint64_t expected = pos.expected[depth - 1];
            bool passed = (result == expected);
            
            if (passed) {
                passedTests++;
                std::cout << "  ✅ Depth " << depth << ": " 
                         << std::setw(12) << result 
                         << " (expected: " << std::setw(12) << expected << ")"
                         << " - " << duration.count() << "ms\n";
            } else {
                failedTests++;
                positionPassed = false;
                std::cout << "  ❌ Depth " << depth << ": " 
                         << std::setw(12) << result 
                         << " (expected: " << std::setw(12) << expected << ")"
                         << " - DIFF: " << (int64_t)(result - expected)
                         << " (" << std::fixed << std::setprecision(3) 
                         << (100.0 * result / expected) << "%)\n";
            }
            
            // Skip deeper depths if already failing (saves time)
            if (!passed && depth < pos.expected.size()) {
                std::cout << "  ⏭️  Skipping remaining depths due to failure\n";
                for (size_t d = depth + 1; d <= pos.expected.size(); ++d) {
                    totalTests++;
                    failedTests++;
                }
                break;
            }
        }
        
        if (positionPassed) {
            std::cout << "✅ Position PASSED all depths\n";
        } else {
            std::cout << "❌ Position FAILED\n";
        }
        std::cout << "\n";
    }
    
    // Summary
    std::cout << std::string(80, '=') << "\n";
    std::cout << "PERFT TEST SUMMARY\n";
    std::cout << std::string(80, '=') << "\n";
    std::cout << "Total Tests:  " << totalTests << "\n";
    std::cout << "Passed:       " << passedTests << " (" 
              << std::fixed << std::setprecision(1) 
              << (100.0 * passedTests / totalTests) << "%)\n";
    std::cout << "Failed:       " << failedTests << " (" 
              << std::fixed << std::setprecision(1)
              << (100.0 * failedTests / totalTests) << "%)\n";
    
    // Calculate overall accuracy
    double accuracy = (double)passedTests / totalTests * 100.0;
    std::cout << "\n";
    
    if (accuracy == 100.0) {
        std::cout << "🎉 PERFECT SCORE! All perft tests passed!\n";
        std::cout << "Move generation is 100% accurate.\n";
    } else if (accuracy >= 99.0) {
        std::cout << "✅ EXCELLENT! Move generation accuracy: " 
                  << std::fixed << std::setprecision(3) << accuracy << "%\n";
        std::cout << "Minor issues remain but core implementation is solid.\n";
    } else if (accuracy >= 95.0) {
        std::cout << "⚠️  GOOD: Move generation accuracy: " 
                  << std::fixed << std::setprecision(3) << accuracy << "%\n";
        std::cout << "Some issues need attention.\n";
    } else {
        std::cout << "❌ NEEDS WORK: Move generation accuracy: " 
                  << std::fixed << std::setprecision(3) << accuracy << "%\n";
        std::cout << "Significant issues must be resolved.\n";
    }
    
    // Master Project Plan Requirements Check
    std::cout << "\n" << std::string(80, '-') << "\n";
    std::cout << "MASTER PROJECT PLAN - PHASE 1 REQUIREMENTS CHECK\n";
    std::cout << std::string(80, '-') << "\n";
    
    bool startingDepth6 = false;
    bool kiwipeteDepth5 = false;
    
    // Check specific requirements
    for (const auto& pos : positions) {
        if (pos.name == "Starting Position") {
            Board board;
            board.fromFEN(pos.fen);
            uint64_t result = perft(board, 6);
            startingDepth6 = (result == 119060324);
            std::cout << "Starting Position Depth 6 (119,060,324): " 
                      << (startingDepth6 ? "✅ PASS" : "❌ FAIL") << "\n";
        }
        if (pos.name == "Kiwipete") {
            Board board;
            board.fromFEN(pos.fen);
            uint64_t result = perft(board, 5);
            kiwipeteDepth5 = (result == 193690690);
            std::cout << "Kiwipete Position Depth 5 (193,690,690): " 
                      << (kiwipeteDepth5 ? "✅ PASS" : "❌ FAIL") << "\n";
        }
    }
    
    std::cout << "\n";
    if (startingDepth6 && kiwipeteDepth5) {
        std::cout << "✅ PHASE 1 MOVE GENERATION REQUIREMENTS MET!\n";
        std::cout << "   Ready to proceed to Phase 2.\n";
    } else {
        std::cout << "⚠️  Phase 1 requirements not fully met.\n";
        std::cout << "   Critical positions must pass before proceeding.\n";
    }
    
    std::cout << std::string(80, '=') << "\n\n";
    
    return (failedTests > 0) ? 1 : 0;
}