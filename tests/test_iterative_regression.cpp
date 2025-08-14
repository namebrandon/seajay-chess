#include <gtest/gtest.h>
#include "../src/core/board.h"
#include "../src/core/board_safety.h"
#include "../src/core/move_generation.h"
#include "../src/search/negamax.h"
#include "../src/search/types.h"
#include "../src/core/transposition_table.h"
#include <vector>
#include <string>

using namespace seajay;

// Regression test suite for Stage 13: Iterative Deepening
// These tests ensure we don't break existing functionality

class IterativeRegressionTest : public ::testing::Test {
protected:
    void SetUp() override {
        m_tt = std::make_unique<TranspositionTable>(16); // 16MB TT
    }
    
    std::unique_ptr<TranspositionTable> m_tt;
    
    // Helper to parse UCI move string to Move
    Move parseMove(const Board& board, const std::string& moveStr) {
        if (moveStr.length() < 4 || moveStr.length() > 5) {
            return NO_MOVE;
        }
        
        Square from = stringToSquare(moveStr.substr(0, 2));
        Square to = stringToSquare(moveStr.substr(2, 2));
        
        if (from == NO_SQUARE || to == NO_SQUARE) {
            return NO_MOVE;
        }
        
        // Generate legal moves and find matching move
        MoveList legalMoves;
        MoveGenerator::generateLegalMoves(const_cast<Board&>(board), legalMoves);
        
        for (size_t i = 0; i < legalMoves.size(); ++i) {
            Move move = legalMoves[i];
            if (moveFrom(move) == from && moveTo(move) == to) {
                return move;
            }
        }
        
        return NO_MOVE;
    }
};

// Test that basic search still works
TEST_F(IterativeRegressionTest, BasicSearchWorks) {
    Board board;
    board.setStartingPosition();
    
    search::SearchLimits limits;
    limits.maxDepth = 4;
    
    Move bestMove = search::search(board, limits, m_tt.get());
    
    // Should find a valid opening move
    EXPECT_NE(bestMove, NO_MOVE);
    
    // Common opening moves
    std::vector<Move> expectedMoves = {
        parseMove(board, "e2e4"),
        parseMove(board, "d2d4"),
        parseMove(board, "g1f3"),
        parseMove(board, "b1c3")
    };
    
    bool foundExpected = false;
    for (const auto& expected : expectedMoves) {
        if (bestMove == expected) {
            foundExpected = true;
            break;
        }
    }
    EXPECT_TRUE(foundExpected) << "Unexpected move: " << SafeMoveExecutor::moveToString(bestMove);
}

// Test that depth limit is respected
TEST_F(IterativeRegressionTest, DepthLimitRespected) {
    Board board;
    board.setStartingPosition();
    
    search::SearchLimits limits;
    limits.maxDepth = 3;
    
    // Capture search info by redirecting stderr
    testing::internal::CaptureStderr();
    [[maybe_unused]] Move bestMove = search::search(board, limits, m_tt.get());
    std::string output = testing::internal::GetCapturedStderr();
    
    // Should not search beyond depth 3
    EXPECT_EQ(output.find("info depth 4"), std::string::npos);
    EXPECT_NE(output.find("info depth 3"), std::string::npos);
}

// Test that time limit is respected (approximately)
TEST_F(IterativeRegressionTest, TimeLimitRespected) {
    Board board;
    board.setStartingPosition();
    
    search::SearchLimits limits;
    limits.movetime = std::chrono::milliseconds(100);
    
    auto start = std::chrono::steady_clock::now();
    Move bestMove = search::search(board, limits, m_tt.get());
    auto elapsed = std::chrono::steady_clock::now() - start;
    
    // Should finish within reasonable bounds (allow 50ms extra for safety)
    auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed);
    EXPECT_LE(elapsed_ms.count(), 150);
    EXPECT_NE(bestMove, NO_MOVE);
}

// Test that mate is found and search stops early
TEST_F(IterativeRegressionTest, MateFoundEarly) {
    // Mate in 2 position
    Board board;
    board.fromFEN("7k/3Q4/8/8/8/8/8/K7 w - - 0 1");
    
    search::SearchLimits limits;
    limits.maxDepth = 10;
    
    testing::internal::CaptureStderr();
    Move bestMove = search::search(board, limits, m_tt.get());
    std::string output = testing::internal::GetCapturedStderr();
    
    // Should find Qd8+ (mate in 2)
    Move expectedMove = parseMove(board, "d7d8");
    EXPECT_EQ(bestMove, expectedMove);
    
    // Should stop searching after finding mate
    EXPECT_NE(output.find("score mate"), std::string::npos);
    
    // Shouldn't search all the way to depth 10
    EXPECT_EQ(output.find("info depth 10"), std::string::npos);
}

// Test that TT is being used between iterations
TEST_F(IterativeRegressionTest, TTUsedBetweenIterations) {
    Board board;
    board.setStartingPosition();
    
    search::SearchLimits limits;
    limits.maxDepth = 4;
    
    // Clear TT stats
    m_tt->clear();
    
    [[maybe_unused]] Move bestMove = search::search(board, limits, m_tt.get());
    
    // Check TT was used
    const auto& stats = m_tt->stats();
    EXPECT_GT(stats.hits.load(), 0u) << "TT should have hits between iterations";
    EXPECT_GT(stats.stores.load(), 0u) << "TT should store positions";
}

// Test canary positions from the plan
TEST_F(IterativeRegressionTest, CanaryPosition1_StartPos) {
    Board board;
    board.setStartingPosition();
    
    search::SearchLimits limits;
    limits.maxDepth = 4;
    
    Move bestMove = search::search(board, limits, m_tt.get());
    
    // Should find e2e4 or d2d4
    Move e2e4 = parseMove(board, "e2e4");
    Move d2d4 = parseMove(board, "d2d4");
    EXPECT_TRUE(bestMove == e2e4 || bestMove == d2d4)
        << "Found: " << SafeMoveExecutor::moveToString(bestMove);
}

TEST_F(IterativeRegressionTest, CanaryPosition2_SimpleTactic) {
    Board board;
    board.fromFEN("6k1/5ppp/8/8/8/7P/5PPK/8 w - - 0 1");
    
    search::SearchLimits limits;
    limits.maxDepth = 6;
    
    search::SearchData info;
    SearchInfo searchInfo;
    
    eval::Score score = search::negamax(board, 6, 0,
                               eval::Score::minus_infinity(),
                               eval::Score::infinity(),
                               searchInfo, info, m_tt.get());
    
    // White should be winning (positive score)
    EXPECT_GT(score.value(), 0) << "Score: " << score.value();
}

// Test position with stable evaluation (for aspiration windows later)
TEST_F(IterativeRegressionTest, StableEvaluationPosition) {
    // Lasker's Sacrifice - stable around +0.30
    Board board;
    board.fromFEN("r1bqkb1r/pppp1ppp/2n2n2/1B2p3/4P3/5N2/PPPP1PPP/RNBQK2R w KQkq - 4 4");
    
    search::SearchLimits limits;
    limits.maxDepth = 8;
    
    search::SearchData info;
    SearchInfo searchInfo;
    
    // Search at different depths and check score stability
    std::vector<eval::Score> scores;
    for (int depth = 4; depth <= 8; depth += 2) {
        info.reset();
        eval::Score score = negamax(board, depth, 0,
                                   eval::Score::minus_infinity(),
                                   eval::Score::infinity(),
                                   searchInfo, info, m_tt.get());
        scores.push_back(score);
    }
    
    // Scores should be relatively stable (within 50 cp)
    for (size_t i = 1; i < scores.size(); ++i) {
        int diff = std::abs(scores[i].value() - scores[i-1].value());
        EXPECT_LE(diff, 50) << "Score instability between depths";
    }
}

// Add more regression tests as needed

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}