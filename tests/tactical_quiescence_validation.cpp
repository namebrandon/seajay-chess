/**
 * Tactical Position Testing for Quiescence Search
 * 
 * Standalone validation program that tests quiescence search
 * against known tactical positions.
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
    std::string expectedMove;  // Optional expected best move
};

class TacticalValidator {
public:
    TacticalValidator() {
        searchInfo.useQuiescence = true;
        searchInfo.maxDepth = 8;
        searchInfo.maxTime = std::chrono::milliseconds(2000);
    }
    
    bool runTest(const TacticalTest& test) {
        std::cout << "\n" << std::string(60, '=') << std::endl;
        std::cout << "Test: " << test.description << std::endl;
        std::cout << "FEN: " << test.fen << std::endl;
        
        Board board(test.fen);
        std::cout << board.toPrettyString() << std::endl;
        
        // Get static evaluation
        eval::Score staticEval = eval::evaluate(board);
        std::cout << "Static eval: " << staticEval << " cp" << std::endl;
        
        // Search with quiescence
        SearchData searchData;
        searchData.startTime = std::chrono::steady_clock::now();
        
        auto [score, bestMove] = negamax(board, searchInfo, searchData);
        
        // Display results
        std::cout << "Search score: " << score << " cp";
        if (test.minScore != -32000 || test.maxScore != 32000) {
            std::cout << " (expected: " << test.minScore << " to " << test.maxScore << ")";
        }
        std::cout << std::endl;
        
        std::cout << "Best move: " << (bestMove.has_value() ? bestMove->toString() : "none");
        if (!test.expectedMove.empty()) {
            std::cout << " (expected: " << test.expectedMove << ")";
        }
        std::cout << std::endl;
        
        std::cout << "Nodes searched: " << searchData.nodes << std::endl;
        std::cout << "Quiescence nodes: " << searchData.qsearchNodes 
                  << " (" << std::fixed << std::setprecision(1) 
                  << searchData.qsearchRatio() << "%)" << std::endl;
        std::cout << "Q-search cutoffs: " << searchData.qsearchCutoffs << std::endl;
        std::cout << "Stand-pats: " << searchData.qsearchStandPats << std::endl;
        
        // Validate results
        bool passed = true;
        if (score < test.minScore || score > test.maxScore) {
            std::cout << "FAILED: Score " << score << " outside expected range ["
                     << test.minScore << ", " << test.maxScore << "]" << std::endl;
            passed = false;
        }
        
        if (!test.expectedMove.empty() && bestMove.has_value()) {
            if (bestMove->toString() != test.expectedMove) {
                std::cout << "WARNING: Different move found (may still be correct)" << std::endl;
            }
        }
        
        if (searchData.nodes > 100 && searchData.qsearchNodes == 0) {
            std::cout << "WARNING: Quiescence not called despite significant search" << std::endl;
        }
        
        std::cout << (passed ? "PASSED" : "FAILED") << std::endl;
        return passed;
    }
    
    void runAllTests() {
        std::vector<TacticalTest> tests = {
            // Basic hanging piece tests
            {
                "r1bqkb1r/pppp1ppp/2n2n2/4p3/2B1P3/3P1N2/PPP2PPP/RNBQK2R w KQkq - 4 4",
                "Simple hanging piece - should find material win",
                100, 600, ""
            },
            
            // Fork detection
            {
                "r1bqkb1r/pppp1ppp/5n2/4p3/3nP3/3P1N2/PPP2PPP/RNBQKB1R w KQkq - 0 5",
                "Knight fork on d4 - Black winning",
                -600, -100, ""
            },
            
            // Back rank mate threat
            {
                "6k1/5ppp/8/8/8/8/5PPP/3R2K1 w - - 0 1",
                "Back rank mate in 1",
                15000, 32000, "d1d8"
            },
            
            // Check evasion
            {
                "rnb1kbnr/pppp1ppp/8/4p3/6Pq/5P2/PPPPP2P/RNBQKBNR w KQkq - 1 3",
                "King in check - must evade",
                -1000, -500, ""
            },
            
            // Promotion race
            {
                "8/1P6/8/8/8/8/1p6/R6K b - - 0 1",
                "Promotion race - Black queens first",
                -900, -800, "b2b1q"
            },
            
            // Quiet position - quiescence shouldn't change much
            {
                "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
                "Starting position - should be roughly equal",
                -50, 50, ""
            },
            
            // Perpetual check avoidance (critical)
            {
                "3Q4/8/3K4/8/8/3k4/8/3q4 b - - 0 1",
                "Perpetual check position - should find draw",
                -50, 50, ""
            },
            
            // Complex tactical position
            {
                "r3k2r/pb1nqppp/1p2pn2/2p5/2PP4/1PN1PN2/PB2QPPP/R3K2R b KQkq - 0 10",
                "Complex position - horizon effect test",
                -100, 100, ""
            },
            
            // Capture sequence
            {
                "r1bqkbnr/pppp1ppp/2n5/4p3/3PP3/5N2/PPP2PPP/RNBQKB1R b KQkq d3 0 3",
                "Pawn capture sequence",
                -50, 150, ""
            },
            
            // Pin exploitation
            {
                "r1bqk2r/pppp1ppp/2n2n2/2b1p3/2B1P3/3P1N2/PPP2PPP/RNBQ1RK1 b kq - 0 6",
                "Bishop pins knight - tactical opportunity",
                -100, 100, ""
            }
        };
        
        int passed = 0;
        int failed = 0;
        
        std::cout << "\n" << std::string(60, '=') << std::endl;
        std::cout << "TACTICAL QUIESCENCE VALIDATION SUITE" << std::endl;
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
            std::cout << "All tests PASSED!" << std::endl;
        } else {
            std::cout << "Some tests FAILED - review output above" << std::endl;
        }
        std::cout << std::string(60, '=') << std::endl;
    }
    
private:
    SearchInfo searchInfo;
};

// Standalone function to validate a single position with Stockfish
void validateWithStockfish(const std::string& fen, int depth = 10) {
    std::cout << "\nValidating with Stockfish:" << std::endl;
    std::cout << "FEN: " << fen << std::endl;
    
    std::string cmd = "echo -e \"position fen " + fen + "\\ngo depth " + 
                     std::to_string(depth) + "\\nquit\" | " +
                     "/workspace/external/engines/stockfish/stockfish 2>/dev/null | " +
                     "grep -E 'info depth " + std::to_string(depth) + " |bestmove'";
    
    std::cout << "Running: " << cmd << std::endl;
    system(cmd.c_str());
}

int main(int argc, char* argv[]) {
    std::cout << "SeaJay Tactical Quiescence Validation" << std::endl;
    std::cout << "Version: " << VERSION_STRING << std::endl;
    
    if (argc > 1 && std::string(argv[1]) == "validate") {
        // Validate specific position with Stockfish
        if (argc < 3) {
            std::cout << "Usage: " << argv[0] << " validate \"<FEN>\" [depth]" << std::endl;
            return 1;
        }
        int depth = (argc > 3) ? std::atoi(argv[3]) : 10;
        validateWithStockfish(argv[2], depth);
        return 0;
    }
    
    TacticalValidator validator;
    validator.runAllTests();
    
    return 0;
}