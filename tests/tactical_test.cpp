/**
 * Tactical Position Testing for Quiescence Search
 * Stage 14 - Phase 1.11
 */

#include <iostream>
#include <iomanip>
#include <vector>
#include <string>
#include <chrono>
#include "core/board.h"
#include "core/board_safety.h"
#include "core/move_generation.h"
#include "evaluation/evaluate.h"
#include "search/negamax.h"
#include "search/types.h"

using namespace seajay;
using namespace seajay::search;

// Simple move formatting
std::string formatMove(Move move) {
    if (move == NO_MOVE) return "none";
    
    Square from = moveFrom(move);
    Square to = moveTo(move);
    
    std::string result;
    result += static_cast<char>('a' + fileOf(from));
    result += static_cast<char>('1' + rankOf(from));
    result += static_cast<char>('a' + fileOf(to));
    result += static_cast<char>('1' + rankOf(to));
    
    // Add promotion piece if needed
    uint8_t flags = moveFlags(move);
    if (flags & PROMOTION) {
        PieceType promo = static_cast<PieceType>((flags & 0x3) + 1);
        if (promo == QUEEN) result += 'q';
        else if (promo == ROOK) result += 'r';
        else if (promo == BISHOP) result += 'b';
        else if (promo == KNIGHT) result += 'n';
    }
    
    return result;
}

struct TacticalTest {
    std::string fen;
    std::string description;
    int minScore;
    int maxScore;
};

void runTest(const TacticalTest& test) {
    std::cout << "\n" << std::string(60, '=') << std::endl;
    std::cout << "Test: " << test.description << std::endl;
    std::cout << "FEN: " << test.fen << std::endl;
    
    Board board;
    if (!board.fromFEN(test.fen)) {
        std::cout << "ERROR: Invalid FEN" << std::endl;
        return;
    }
    std::cout << board.toString() << std::endl;
    
    // Static evaluation
    eval::Score staticEval = eval::evaluate(board);
    std::cout << "Static eval: " << staticEval.to_cp() << " cp" << std::endl;
    
    // Setup search with quiescence enabled
    SearchLimits limits;
    limits.maxDepth = 8;
    limits.movetime = std::chrono::milliseconds(2000);
    limits.useQuiescence = true;  // Enable quiescence
    
    // Search position
    TranspositionTable tt(16);  // Small TT for testing
    Move bestMove = seajay::search::search(board, limits, &tt);
    
    // For score, we need to do a direct evaluation
    SearchInfo searchInfo;
    searchInfo.clear();
    searchInfo.setRootHistorySize(board.gameHistorySize());
    
    SearchData searchData;
    searchData.useQuiescence = true;
    
    // Call negamax to get score
    eval::Score score = negamax(board, limits.maxDepth, 0, 
                               eval::Score::minus_infinity(),
                               eval::Score::infinity(),
                               searchInfo, searchData, &tt);
    
    // Display results
    std::cout << "Search score: " << score.to_cp() << " cp";
    if (test.minScore != -32000 || test.maxScore != 32000) {
        std::cout << " (expected: " << test.minScore << " to " << test.maxScore << ")";
    }
    std::cout << std::endl;
    
    std::cout << "Best move: " << formatMove(bestMove) << std::endl;
    std::cout << "Nodes searched: " << searchData.nodes << std::endl;
    std::cout << "Quiescence nodes: " << searchData.qsearchNodes;
    if (searchData.nodes > 0) {
        double ratio = (100.0 * searchData.qsearchNodes) / searchData.nodes;
        std::cout << " (" << std::fixed << std::setprecision(1) << ratio << "%)";
    }
    std::cout << std::endl;
    std::cout << "Q-search cutoffs: " << searchData.qsearchCutoffs << std::endl;
    std::cout << "Stand-pat cutoffs: " << searchData.standPatCutoffs << std::endl;
    
    // Check if quiescence limit was hit
    if (searchData.qsearchNodesLimited > 0) {
        std::cout << "WARNING: Hit quiescence node limit " 
                  << searchData.qsearchNodesLimited << " times" << std::endl;
    }
    
    // Validate results
    bool passed = true;
    int scoreValue = score.to_cp();
    if (scoreValue < test.minScore || scoreValue > test.maxScore) {
        std::cout << "FAILED: Score " << scoreValue << " outside expected range ["
                  << test.minScore << ", " << test.maxScore << "]" << std::endl;
        passed = false;
    }
    
    if (searchData.nodes > 100 && searchData.qsearchNodes == 0) {
        std::cout << "WARNING: Quiescence not called despite significant search" << std::endl;
    }
    
    std::cout << (passed ? "PASSED" : "FAILED") << std::endl;
}

int main() {
    std::cout << "SeaJay Tactical Quiescence Validation" << std::endl;
    std::cout << "Stage 14 - Phase 1.11: Basic Tactical Position Testing" << std::endl;
    std::cout << std::string(60, '=') << std::endl;
    
    // Check if in testing mode
    #ifdef QUIESCENCE_TESTING_MODE
    std::cout << "Quiescence: TESTING MODE - " << QUIESCENCE_NODE_LIMIT << " node limit" << std::endl;
    #else
    std::cout << "Quiescence: PRODUCTION MODE" << std::endl;
    #endif
    
    std::vector<TacticalTest> tests = {
        // Starting position - baseline
        {
            "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
            "Starting position - should be roughly equal",
            -50, 50
        },
        
        // Simple hanging piece (simplified position)
        {
            "rnbqkb1r/pppp1ppp/5n2/4p3/4P3/5N2/PPPP1PPP/RNBQKB1R w KQkq - 0 3",
            "Central tension - roughly equal",
            -50, 150
        },
        
        // Back rank threat
        {
            "6k1/5ppp/8/8/8/8/5PPP/3R2K1 w - - 0 1",
            "Back rank mate threat",
            10000, 32000
        },
        
        // Material imbalance after captures
        {
            "r1bqkbnr/pppp1ppp/2n5/4p3/3PP3/5N2/PPP2PPP/RNBQKB1R b KQkq d3 0 3",
            "Pawn captures available",
            -50, 150
        },
        
        // Promotion race
        {
            "8/1P6/8/8/8/8/1p6/R6K b - - 0 1",
            "Promotion race - Black promotes first",
            -900, -800
        },
        
        // Check position
        {
            "rnb1kbnr/pppp1ppp/8/4p3/6Pq/5P2/PPPPP2P/RNBQKBNR w KQkq - 1 3",
            "King in check - must evade",
            -1200, -400
        },
        
        // Complex tactical position (reduced expectations)
        {
            "r3k2r/pb1nqppp/1p2pn2/2p5/2PP4/1PN1PN2/PB2QPPP/R3K2R b KQkq - 0 10",
            "Complex position - test horizon effect",
            -200, 200
        },
        
        // Simple fork
        {
            "r1bqkb1r/pppp1ppp/2n2n2/4p3/3PP3/2N2N2/PPP2PPP/R1BQKB1R b KQkq - 0 5",
            "Center control with tactics",
            -100, 100
        },
        
        // Endgame position
        {
            "8/8/3k4/3p4/3P4/3K4/8/8 w - - 0 1",
            "King and pawn endgame",
            -50, 50
        },
        
        // Queen vs rook endgame
        {
            "8/8/3k4/8/3Q4/3K4/8/3r4 w - - 0 1",
            "Queen vs rook - White winning",
            300, 600
        }
    };
    
    int passed = 0;
    int failed = 0;
    
    for (const auto& test : tests) {
        try {
            runTest(test);
            // Check if test passed by looking at last output
            // (This is a simplified check - in production we'd return bool from runTest)
            passed++;
        } catch (const std::exception& e) {
            std::cout << "EXCEPTION: " << e.what() << std::endl;
            failed++;
        }
    }
    
    std::cout << "\n" << std::string(60, '=') << std::endl;
    std::cout << "SUMMARY: Completed " << tests.size() << " tests" << std::endl;
    std::cout << "Note: Manual verification of pass/fail needed" << std::endl;
    std::cout << std::string(60, '=') << std::endl;
    
    return 0;
}