/**
 * Tactical Position Testing for Quiescence Search
 * 
 * This test suite validates that quiescence search correctly:
 * 1. Resolves tactical sequences to quiet positions
 * 2. Avoids horizon effect in critical positions
 * 3. Finds basic tactics (forks, pins, skewers)
 * 4. Handles check evasion correctly
 * 5. Doesn't fall into perpetual check traps
 */

#include <gtest/gtest.h>
#include "board.h"
#include "move_generator.h"
#include "evaluation.h"
#include "search/negamax.h"
#include "search/quiescence.h"
#include "search/types.h"
#include <iostream>
#include <iomanip>

using namespace seajay;
using namespace seajay::search;

class TacticalQuiescenceTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Enable quiescence for these tests
        searchInfo.useQuiescence = true;
        searchInfo.maxDepth = 8;  // Reasonable depth for tactical tests
        searchInfo.maxTime = std::chrono::milliseconds(1000); // 1 second max per position
    }

    void analyzePosition(const std::string& fen, const std::string& description,
                        int minExpectedScore = -32000, int maxExpectedScore = 32000) {
        Board board(fen);
        SearchData searchData;
        searchData.startTime = std::chrono::steady_clock::now();
        
        std::cout << "\n=== " << description << " ===" << std::endl;
        std::cout << "FEN: " << fen << std::endl;
        std::cout << board.toPrettyString() << std::endl;
        
        // Get static evaluation
        eval::Score staticEval = eval::evaluate(board);
        std::cout << "Static eval: " << staticEval << " cp" << std::endl;
        
        // Search with quiescence
        auto [score, bestMove] = negamax(board, searchInfo, searchData);
        
        std::cout << "Search result: " << score << " cp" << std::endl;
        std::cout << "Best move: " << (bestMove.has_value() ? bestMove->toString() : "none") << std::endl;
        std::cout << "Total nodes: " << searchData.nodes << std::endl;
        std::cout << "Quiescence nodes: " << searchData.qsearchNodes 
                  << " (" << std::fixed << std::setprecision(1) 
                  << searchData.qsearchRatio() << "%)" << std::endl;
        
        // Validate score is within expected range
        EXPECT_GE(score, minExpectedScore) << "Score too low for: " << description;
        EXPECT_LE(score, maxExpectedScore) << "Score too high for: " << description;
        
        // Ensure quiescence was actually used
        if (searchData.nodes > 100) {  // Only check if significant search occurred
            EXPECT_GT(searchData.qsearchNodes, 0) << "Quiescence not called for: " << description;
        }
    }

    SearchInfo searchInfo;
};

// Test 1: Queen can capture hanging rook - simple tactical win
TEST_F(TacticalQuiescenceTest, SimpleHangingPiece) {
    analyzePosition(
        "r1bqkb1r/pppp1ppp/2n2n2/4p3/2B1P3/3P1N2/PPP2PPP/RNBQK2R w KQkq - 4 4",
        "Queen can capture hanging knight on f6",
        100, 600  // Should find winning material
    );
}

// Test 2: Perpetual check detection - critical safety test
TEST_F(TacticalQuiescenceTest, PerpetualCheckPrevention) {
    analyzePosition(
        "3Q4/8/3K4/8/8/3k4/8/3q4 b - - 0 1",
        "Black queen giving perpetual check - should detect draw",
        -50, 50  // Should evaluate as drawn
    );
}

// Test 3: Back rank mate threat
TEST_F(TacticalQuiescenceTest, BackRankMate) {
    analyzePosition(
        "6k1/5ppp/8/8/8/8/5PPP/3R2K1 w - - 0 1",
        "White threatens back rank mate with Rd8#",
        15000, 32000  // Should find mate
    );
}

// Test 4: Fork detection
TEST_F(TacticalQuiescenceTest, KnightFork) {
    analyzePosition(
        "r1bqkb1r/pppp1ppp/5n2/4p3/3nP3/3P1N2/PPP2PPP/RNBQKB1R w KQkq - 0 5",
        "Black knight on d4 forks queen and rook",
        -600, -100  // Black is winning material
    );
}

// Test 5: Pin exploitation
TEST_F(TacticalQuiescenceTest, AbsolutePin) {
    analyzePosition(
        "r1bqk2r/pppp1ppp/2n2n2/2b1p3/2B1P3/3P1N2/PPP2PPP/RNBQ1RK1 b kq - 0 6",
        "Bishop pins knight to king - tactical opportunity",
        -100, 100  // Roughly equal after tactics resolve
    );
}

// Test 6: Capture sequence resolution
TEST_F(TacticalQuiescenceTest, CaptureSequence) {
    analyzePosition(
        "r1bqkbnr/pppp1ppp/2n5/4p3/3PP3/5N2/PPP2PPP/RNBQKB1R b KQkq d3 0 3",
        "Pawn captures lead to tactical sequence",
        -50, 150  // Slight advantage after captures
    );
}

// Test 7: Check evasion in quiescence
TEST_F(TacticalQuiescenceTest, CheckEvasion) {
    analyzePosition(
        "rnb1kbnr/pppp1ppp/8/4p3/6Pq/5P2/PPPPP2P/RNBQKBNR w KQkq - 1 3",
        "White king in check - must evade correctly",
        -1000, -500  // Black has advantage after check
    );
}

// Test 8: Promotion tactics
TEST_F(TacticalQuiescenceTest, PromotionTactics) {
    analyzePosition(
        "8/1P6/8/8/8/8/1p6/R6K b - - 0 1",
        "Race to promote - critical timing",
        -900, -800  // Black queens first
    );
}

// Test 9: Discovered attack
TEST_F(TacticalQuiescenceTest, DiscoveredAttack) {
    analyzePosition(
        "r1bqk2r/pppp1ppp/2n2n2/2b1p3/2BPP3/3P1N2/PP3PPP/RNBQK2R b KQkq - 0 6",
        "Moving pieces can discover attacks",
        -100, 100  // Balanced after tactics
    );
}

// Test 10: Overloaded piece
TEST_F(TacticalQuiescenceTest, OverloadedDefender) {
    analyzePosition(
        "r1bqk2r/ppp2ppp/2np1n2/2b1p3/2B1P3/3P1N2/PPP2PPP/RNBQR1K1 b kq - 0 7",
        "Defender is overloaded - tactical vulnerability",
        -50, 150  // Slightly better for white
    );
}

// Test horizon effect - position where shallow search misses key tactic
TEST_F(TacticalQuiescenceTest, HorizonEffect) {
    // Position where a piece appears to be hanging but is actually defended by a counter-threat
    analyzePosition(
        "r3k2r/pb1nqppp/1p2pn2/2p5/2PP4/1PN1PN2/PB2QPPP/R3K2R b KQkq - 0 10",
        "Complex tactical position - horizon effect test",
        -100, 100  // Should be roughly equal after all tactics resolve
    );
}

// Validate quiescence doesn't change evaluation sign incorrectly
TEST_F(TacticalQuiescenceTest, EvaluationConsistency) {
    // Quiet position - quiescence shouldn't dramatically change eval
    Board board("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    SearchData searchData;
    searchData.startTime = std::chrono::steady_clock::now();
    
    eval::Score staticEval = eval::evaluate(board);
    
    // Call quiescence directly
    eval::Score qscore = quiescence(board, 0, -32000, 32000, searchInfo, searchData);
    
    std::cout << "\nQuiet position consistency test:" << std::endl;
    std::cout << "Static eval: " << staticEval << " cp" << std::endl;
    std::cout << "Quiescence eval: " << qscore << " cp" << std::endl;
    std::cout << "Difference: " << abs(qscore - staticEval) << " cp" << std::endl;
    
    // In a quiet position, quiescence should return close to static eval
    EXPECT_NEAR(qscore, staticEval, 50) << "Quiescence changed quiet position eval too much";
}

// Test statistics tracking
TEST_F(TacticalQuiescenceTest, StatisticsTracking) {
    Board board("r1bqkb1r/pppp1ppp/2n2n2/4p3/2B1P3/3P1N2/PPP2PPP/RNBQK2R w KQkq - 4 4");
    SearchData searchData;
    searchData.startTime = std::chrono::steady_clock::now();
    
    auto [score, bestMove] = negamax(board, searchInfo, searchData);
    
    // Verify statistics are being tracked
    EXPECT_GT(searchData.nodes, 0) << "No nodes searched";
    EXPECT_GT(searchData.qsearchNodes, 0) << "No quiescence nodes";
    EXPECT_GE(searchData.qsearchCutoffs, 0) << "Invalid cutoff count";
    EXPECT_GE(searchData.qsearchStandPats, 0) << "Invalid stand-pat count";
    
    // Ratio should be reasonable (0-100%)
    double ratio = searchData.qsearchRatio();
    EXPECT_GE(ratio, 0.0) << "Invalid quiescence ratio";
    EXPECT_LE(ratio, 100.0) << "Invalid quiescence ratio";
    
    std::cout << "\nStatistics for tactical position:" << std::endl;
    std::cout << "Total nodes: " << searchData.nodes << std::endl;
    std::cout << "Quiescence nodes: " << searchData.qsearchNodes << std::endl;
    std::cout << "Quiescence cutoffs: " << searchData.qsearchCutoffs << std::endl;
    std::cout << "Stand-pats: " << searchData.qsearchStandPats << std::endl;
    std::cout << "Q-search ratio: " << ratio << "%" << std::endl;
}