/**
 * Tactical Position Testing for Quiescence Search
 * Standalone validation program
 */

#include <iostream>
#include <iomanip>
#include <vector>
#include <string>
#include <chrono>
#include "core/board.h"
#include "core/move_generation.h"
#include "evaluation/evaluate.h"
#include "search/negamax.h"
#include "search/quiescence.h"
#include "search/types.h"

using namespace seajay;
using namespace seajay::search;

struct TacticalTest {
    std::string fen;
    std::string description;
    int minScore;
    int maxScore;
};

class TacticalValidator {
public:
    bool runTest(const TacticalTest& test) {
        std::cout << "\n" << std::string(60, '=') << std::endl;
        std::cout << "Test: " << test.description << std::endl;
        std::cout << "FEN: " << test.fen << std::endl;
        
        Board board(test.fen);
        std::cout << board.toPrettyString() << std::endl;
        
        // Static evaluation
        eval::Score staticEval = eval::evaluate(board);
        std::cout << "Static eval: " << staticEval << " cp" << std::endl;
        
        // Setup search with quiescence enabled
        SearchLimits limits;
        limits.maxDepth = 8;
        limits.movetime = std::chrono::milliseconds(2000);
        limits.useQuiescence = true;  // Enable quiescence
        
        // Search position
        TranspositionTable tt(16);  // Small TT for testing
        Move bestMove = search(board, limits, &tt);
        
        // For score, we need to do a direct evaluation after move
        // or call negamax directly
        SearchInfo searchInfo;
        searchInfo.clear();
        searchInfo.setRootHistorySize(board.gameHistorySize());
        
        SearchData searchData;
        searchData.useQuiescence = true;
        
        // Call negamax to get score
        eval::Score score = negamax(board, limits.maxDepth, 0, 
                                   eval::Score::minus_infinity(),
                                   eval::Score::plus_infinity(),
                                   searchInfo, searchData, &tt);
        
        // Display results
        std::cout << "Search score: " << score << " cp";
        if (test.minScore != -32000 || test.maxScore != 32000) {
            std::cout << " (expected: " << test.minScore << " to " << test.maxScore << ")";
        }
        std::cout << std::endl;
        
        std::cout << "Best move: " << bestMove.toString() << std::endl;
        std::cout << "Nodes searched: " << searchData.nodes << std::endl;
        std::cout << "Quiescence nodes: " << searchData.qsearchNodes;
        if (searchData.nodes > 0) {
            double ratio = (100.0 * searchData.qsearchNodes) / searchData.nodes;
            std::cout << " (" << std::fixed << std::setprecision(1) << ratio << "%)";
        }
        std::cout << std::endl;
        std::cout << "Q-search cutoffs: " << searchData.qsearchCutoffs << std::endl;
        std::cout << "Stand-pat cutoffs: " << searchData.standPatCutoffs << std::endl;
        
        // Validate results
        bool passed = true;
        if (score < test.minScore || score > test.maxScore) {
            std::cout << "FAILED: Score " << score << " outside expected range" << std::endl;
            passed = false;
        }
        
        if (searchData.nodes > 100 && searchData.qsearchNodes == 0) {
            std::cout << "WARNING: Quiescence not called" << std::endl;
        }
        
        std::cout << (passed ? "PASSED" : "FAILED") << std::endl;
        return passed;
    }
    
    void runAllTests() {
        std::vector<TacticalTest> tests = {
            // Starting position - baseline
            {
                "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
                "Starting position - should be roughly equal",
                -50, 50
            },
            
            // Simple hanging piece
            {
                "r1bqkb1r/pppp1ppp/2n2n2/4p3/2B1P3/5N2/PPPP1PPP/RNBQK2R w KQkq - 4 4",
                "Hanging knight on f6 - White should be winning",
                100, 600
            },
            
            // Fork position
            {
                "r1bqkb1r/pppp1ppp/5n2/4p3/3nP3/3P1N2/PPP2PPP/RNBQKB1R w KQkq - 0 5",
                "Black knight fork - Black winning",
                -600, -100
            },
            
            // Back rank threat
            {
                "6k1/5ppp/8/8/8/8/5PPP/3R2K1 w - - 0 1",
                "Back rank mate threat",
                10000, 32000
            },
            
            // Check evasion
            {
                "rnb1kbnr/pppp1ppp/8/4p3/6Pq/5P2/PPPPP2P/RNBQKBNR w KQkq - 1 3",
                "King in check - must evade",
                -1000, -500
            },
            
            // Promotion race  
            {
                "8/1P6/8/8/8/8/1p6/R6K b - - 0 1",
                "Promotion race - Black promotes first",
                -900, -800
            },
            
            // Complex tactical position
            {
                "r3k2r/pb1nqppp/1p2pn2/2p5/2PP4/1PN1PN2/PB2QPPP/R3K2R b KQkq - 0 10",
                "Complex position - test horizon effect",
                -100, 100
            },
            
            // Perpetual check position (critical)
            {
                "3Q4/8/3K4/8/8/3k4/8/3q4 b - - 0 1",
                "Perpetual check - should be draw",
                -50, 50
            },
            
            // Material imbalance
            {
                "r1bqkbnr/pppp1ppp/2n5/4p3/3PP3/5N2/PPP2PPP/RNBQKB1R b KQkq d3 0 3",
                "Pawn captures available",
                -50, 150
            },
            
            // Pin position
            {
                "r1bqk2r/pppp1ppp/2n2n2/2b1p3/2B1P3/3P1N2/PPP2PPP/RNBQ1RK1 b kq - 0 6",
                "Bishop pins knight",
                -100, 100
            }
        };
        
        int passed = 0;
        int failed = 0;
        
        std::cout << "\n" << std::string(60, '=') << std::endl;
        std::cout << "TACTICAL QUIESCENCE VALIDATION" << std::endl;
        std::cout << "Running " << tests.size() << " tests..." << std::endl;
        std::cout << std::string(60, '=') << std::endl;
        
        for (const auto& test : tests) {
            if (runTest(test)) {
                passed++;
            } else {
                failed++;
            }
        }
        
        std::cout << "\n" << std::string(60, '=') << std::endl;
        std::cout << "RESULTS: " << passed << " passed, " << failed << " failed" << std::endl;
        
        if (failed == 0) {
            std::cout << "SUCCESS: All tests passed!" << std::endl;
        } else {
            std::cout << "FAILURE: Some tests failed" << std::endl;
        }
        std::cout << std::string(60, '=') << std::endl;
    }
};

// Validate a single position with Stockfish
void validateWithStockfish(const std::string& fen, int depth = 10) {
    std::cout << "\nValidating with Stockfish:" << std::endl;
    std::cout << "FEN: " << fen << std::endl;
    
    std::string cmd = "echo -e \"position fen " + fen + "\\ngo depth " + 
                     std::to_string(depth) + "\\nquit\" | " +
                     "/workspace/external/engines/stockfish/stockfish 2>/dev/null | " +
                     "grep -E 'bestmove|score cp|score mate'";
    
    system(cmd.c_str());
}

int main(int argc, char* argv[]) {
    std::cout << "SeaJay Tactical Quiescence Validation" << std::endl;
    std::cout << "Stage 14 - Phase 1.11" << std::endl;
    
    if (argc > 1) {
        if (std::string(argv[1]) == "stockfish" && argc > 2) {
            // Validate specific position with Stockfish
            int depth = (argc > 3) ? std::atoi(argv[3]) : 10;
            validateWithStockfish(argv[2], depth);
            return 0;
        }
    }
    
    TacticalValidator validator;
    validator.runAllTests();
    
    return 0;
}