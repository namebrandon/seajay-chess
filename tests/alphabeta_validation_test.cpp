#include <gtest/gtest.h>
#include "../src/core/board.h"
#include "../src/core/move_generation.h"
#include "../src/search/negamax.h"
#include "../src/search/types.h"
#include "../src/search/search_info.h"
#include "../src/core/board_safety.h"
#include <iostream>
#include <vector>
#include <string>
#include <chrono>

using namespace seajay;
using namespace seajay::search;

// Test fixture for alpha-beta validation
class AlphaBetaValidationTest : public ::testing::Test {
protected:
    // Helper to parse UCI move string
    Move parseMove(const std::string& moveStr, const Board& board) {
        if (moveStr.length() < 4) return Move();
        
        Square from = stringToSquare(moveStr.substr(0, 2));
        Square to = stringToSquare(moveStr.substr(2, 2));
        
        if (from == NO_SQUARE || to == NO_SQUARE) return Move();
        
        // Find matching legal move
        auto moves = generateLegalMoves(const_cast<Board&>(board));
        for (const Move& move : moves) {
            if (moveFrom(move) == from && moveTo(move) == to) {
                return move;
            }
        }
        return Move();
    }
    // Helper function to search without alpha-beta pruning
    eval::Score negamaxNoPruning(Board& board, int depth, int ply, SearchData& info) {
        // Terminal node
        if (depth <= 0) {
            info.nodes++;
            return board.evaluate();
        }
        
        // Generate moves
        auto moves = generateLegalMoves(board);
        
        // Check for terminal positions
        if (moves.empty()) {
            info.nodes++;
            if (inCheck(board)) {
                return eval::Score(-32000 + ply);  // Checkmate
            }
            return eval::Score::draw();  // Stalemate
        }
        
        // Search all moves (no pruning)
        eval::Score bestScore = eval::Score::minus_infinity();
        for (const Move& move : moves) {
            Board::UndoInfo undo;
            board.makeMove(move, undo);
            
            eval::Score score = -negamaxNoPruning(board, depth - 1, ply + 1, info);
            
            board.unmakeMove(move, undo);
            
            if (score > bestScore) {
                bestScore = score;
                if (ply == 0) {
                    info.bestMove = move;
                    info.bestScore = score;
                }
            }
        }
        
        info.nodes++;
        return bestScore;
    }
    
    // Compare search results with and without alpha-beta
    struct ValidationResult {
        Move moveWithAB;
        Move moveWithoutAB;
        eval::Score scoreWithAB;
        eval::Score scoreWithoutAB;
        uint64_t nodesWithAB;
        uint64_t nodesWithoutAB;
        double nodeReduction;  // Percentage reduction
        bool movesMatch;
        bool scoresMatch;
    };
    
    ValidationResult validatePosition(const std::string& fen, int depth) {
        ValidationResult result;
        
        // Search WITH alpha-beta pruning
        Board board1;
        board1.fromFEN(fen);
        SearchInfo searchInfoAB;
        searchInfoAB.clear();
        searchInfoAB.setRootHistorySize(board1.gameHistorySize());
        SearchData infoWithAB;
        result.scoreWithAB = negamax(board1, depth, 0,
                                     eval::Score::minus_infinity(),
                                     eval::Score::infinity(),
                                     searchInfoAB, infoWithAB);
        result.moveWithAB = infoWithAB.bestMove;
        result.nodesWithAB = infoWithAB.nodes;
        
        // Search WITHOUT alpha-beta pruning (full minimax)
        Board board2;
        board2.fromFEN(fen);
        SearchData infoWithoutAB;
        result.scoreWithoutAB = negamaxNoPruning(board2, depth, 0, infoWithoutAB);
        result.moveWithoutAB = infoWithoutAB.bestMove;
        result.nodesWithoutAB = infoWithoutAB.nodes;
        
        // Calculate statistics
        result.movesMatch = (result.moveWithAB == result.moveWithoutAB);
        result.scoresMatch = (result.scoreWithAB == result.scoreWithoutAB);
        if (result.nodesWithoutAB > 0) {
            result.nodeReduction = 100.0 * (1.0 - static_cast<double>(result.nodesWithAB) / 
                                           static_cast<double>(result.nodesWithoutAB));
        } else {
            result.nodeReduction = 0.0;
        }
        
        return result;
    }
};

// Test that alpha-beta returns the same best move as full minimax
TEST_F(AlphaBetaValidationTest, SameBestMove) {
    std::vector<std::string> testPositions = {
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",  // Starting position
        "r1bqkbnr/pppp1ppp/2n5/4p3/4P3/5N2/PPPP1PPP/RNBQKB1R w KQkq - 4 4",  // Italian Game
        "rnbqkb1r/pp1ppppp/5n2/2p5/4P3/5N2/PPPP1PPP/RNBQKB1R w KQkq c6 0 4",  // Sicilian
        "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1", // Kiwipete
        "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1",  // Endgame position
        "r2q1rk1/ppp2ppp/2n1bn2/2bpp3/3PP3/2N2N2/PPP1BPPP/R1BQK2R w KQ - 0 8"  // Complex middlegame
    };
    
    for (const auto& fen : testPositions) {
        auto result = validatePosition(fen, 4);  // Test at depth 4
        
        EXPECT_TRUE(result.movesMatch) 
            << "Position: " << fen << "\n"
            << "Move with AB: " << SafeMoveExecutor::moveToString(result.moveWithAB) << "\n"
            << "Move without AB: " << SafeMoveExecutor::moveToString(result.moveWithoutAB);
    }
}

// Test that alpha-beta returns the same score as full minimax
TEST_F(AlphaBetaValidationTest, SameScore) {
    std::vector<std::string> testPositions = {
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
        "r1bqkbnr/pppp1ppp/2n5/4p3/4P3/5N2/PPPP1PPP/RNBQKB1R w KQkq - 4 4",
        "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1",
        "4k3/8/8/8/8/8/4P3/4K3 w - - 0 1"  // Simple endgame
    };
    
    for (const auto& fen : testPositions) {
        auto result = validatePosition(fen, 4);
        
        EXPECT_EQ(result.scoreWithAB.value(), result.scoreWithoutAB.value())
            << "Position: " << fen << "\n"
            << "Score with AB: " << result.scoreWithAB.to_cp() << " cp\n"
            << "Score without AB: " << result.scoreWithoutAB.to_cp() << " cp";
    }
}

// Test that alpha-beta achieves significant node reduction
TEST_F(AlphaBetaValidationTest, NodeReduction) {
    std::vector<std::pair<std::string, double>> testCases = {
        {"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", 50.0},  // Expect >50% reduction
        {"r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1", 60.0},  // Complex, expect >60%
        {"8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1", 40.0}  // Endgame, expect >40%
    };
    
    for (const auto& [fen, minReduction] : testCases) {
        auto result = validatePosition(fen, 4);
        
        EXPECT_GT(result.nodeReduction, minReduction)
            << "Position: " << fen << "\n"
            << "Nodes with AB: " << result.nodesWithAB << "\n"
            << "Nodes without AB: " << result.nodesWithoutAB << "\n"
            << "Reduction: " << result.nodeReduction << "%";
        
        // Also verify correctness
        EXPECT_TRUE(result.movesMatch) << "Moves should match";
        EXPECT_TRUE(result.scoresMatch) << "Scores should match";
    }
}

// Test edge cases - positions where pruning might fail
TEST_F(AlphaBetaValidationTest, EdgeCases) {
    // Test stalemate position
    {
        std::string fen = "7k/8/7K/8/8/8/8/1R6 b - - 0 1";  // Black in stalemate
        auto result = validatePosition(fen, 2);
        EXPECT_TRUE(result.scoresMatch);
        EXPECT_EQ(result.scoreWithAB.value(), 0);  // Should be draw
    }
    
    // Test immediate checkmate
    {
        std::string fen = "7k/8/5K2/8/8/8/8/1R6 w - - 0 1";  // White can mate in 1
        auto result = validatePosition(fen, 2);
        EXPECT_TRUE(result.scoresMatch);
        EXPECT_TRUE(result.scoreWithAB.is_mate_score());
    }
    
    // Test position with only one legal move
    {
        std::string fen = "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q2/PPPBBPPP/R3K2R b KQkq - 0 1";
        Board board;
        board.fromFEN(fen);
        Move move = parseMove("e7h4", board);
        Board::UndoInfo undo;
        board.makeMove(move, undo);  // Put king in check
        auto result = validatePosition(board.toFEN(), 3);
        EXPECT_TRUE(result.movesMatch);
        EXPECT_TRUE(result.scoresMatch);
    }
}

// Test move ordering efficiency
TEST_F(AlphaBetaValidationTest, MoveOrderingEfficiency) {
    std::string fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
    Board board;
    board.fromFEN(fen);
    
    SearchInfo searchInfo;
    searchInfo.clear();
    searchInfo.setRootHistorySize(board.gameHistorySize());
    SearchData info;
    negamax(board, 5, 0, eval::Score::minus_infinity(), eval::Score::infinity(), searchInfo, info);
    
    double efficiency = info.moveOrderingEfficiency();
    
    // With basic move ordering (captures/promotions first), expect >50% efficiency
    EXPECT_GT(efficiency, 50.0) 
        << "Move ordering efficiency: " << efficiency << "%\n"
        << "Beta cutoffs: " << info.betaCutoffs << "\n"
        << "First-move cutoffs: " << info.betaCutoffsFirst;
    
    // Also check effective branching factor
    double ebf = info.effectiveBranchingFactor();
    EXPECT_LT(ebf, 10.0) << "Effective branching factor: " << ebf;
    EXPECT_GT(ebf, 2.0) << "Effective branching factor: " << ebf;
}

// Test that pruning works correctly at different depths
TEST_F(AlphaBetaValidationTest, VariableDepth) {
    std::string fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
    
    for (int depth = 1; depth <= 5; ++depth) {
        auto result = validatePosition(fen, depth);
        
        EXPECT_TRUE(result.movesMatch) 
            << "Depth " << depth << ": moves don't match";
        EXPECT_TRUE(result.scoresMatch) 
            << "Depth " << depth << ": scores don't match";
        
        // Node reduction should increase with depth
        if (depth >= 3) {
            EXPECT_GT(result.nodeReduction, 30.0)
                << "Depth " << depth << ": insufficient node reduction";
        }
        
        std::cout << "Depth " << depth 
                  << ": Nodes with AB = " << result.nodesWithAB
                  << ", Nodes without = " << result.nodesWithoutAB
                  << ", Reduction = " << result.nodeReduction << "%\n";
    }
}

// Performance benchmark comparing with and without alpha-beta
TEST_F(AlphaBetaValidationTest, PerformanceBenchmark) {
    std::string fen = "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1";
    
    // Time search WITH alpha-beta
    Board board1;
    board1.fromFEN(fen);
    SearchInfo searchInfo1;
    searchInfo1.clear();
    searchInfo1.setRootHistorySize(board1.gameHistorySize());
    SearchData info1;
    auto start1 = std::chrono::steady_clock::now();
    negamax(board1, 5, 0, eval::Score::minus_infinity(), eval::Score::infinity(), searchInfo1, info1);
    auto end1 = std::chrono::steady_clock::now();
    auto timeWithAB = std::chrono::duration_cast<std::chrono::milliseconds>(end1 - start1).count();
    
    // Time search WITHOUT alpha-beta
    Board board2;
    board2.fromFEN(fen);
    SearchData info2;
    auto start2 = std::chrono::steady_clock::now();
    negamaxNoPruning(board2, 5, 0, info2);
    auto end2 = std::chrono::steady_clock::now();
    auto timeWithoutAB = std::chrono::duration_cast<std::chrono::milliseconds>(end2 - start2).count();
    
    double speedup = static_cast<double>(timeWithoutAB) / std::max(static_cast<decltype(timeWithAB)>(1), timeWithAB);
    
    std::cout << "\nPerformance Comparison (depth 5):\n"
              << "With Alpha-Beta: " << timeWithAB << " ms, " << info1.nodes << " nodes\n"
              << "Without Alpha-Beta: " << timeWithoutAB << " ms, " << info2.nodes << " nodes\n"
              << "Speedup: " << speedup << "x\n"
              << "Node reduction: " << (100.0 * (1.0 - static_cast<double>(info1.nodes) / info2.nodes)) << "%\n";
    
    // Expect significant speedup
    EXPECT_GT(speedup, 2.0) << "Alpha-beta should provide at least 2x speedup";
}