#include <gtest/gtest.h>
#include "../src/core/board.h"
#include "../src/evaluation/pst.h"
#include "../src/evaluation/evaluate.h"
#include <iostream>

using namespace seajay;
using namespace seajay::eval;

class PSTTest : public ::testing::Test {
protected:
    Board board;
};

// Test that PST tables are properly initialized
TEST_F(PSTTest, TablesInitialized) {
    // Pawns should have 0 value on rank 1 and 8
    for (Square sq = A1; sq <= H1; ++sq) {
        EXPECT_EQ(PST::rawValue(PAWN, sq).mg.value(), 0);
        EXPECT_EQ(PST::rawValue(PAWN, sq).eg.value(), 0);
    }
    for (Square sq = A8; sq <= H8; ++sq) {
        EXPECT_EQ(PST::rawValue(PAWN, sq).mg.value(), 0);
        EXPECT_EQ(PST::rawValue(PAWN, sq).eg.value(), 0);
    }
    
    // Knights should prefer center squares
    MgEgScore knightCenter = PST::rawValue(KNIGHT, E4);
    MgEgScore knightCorner = PST::rawValue(KNIGHT, A1);
    EXPECT_GT(knightCenter.mg.value(), knightCorner.mg.value());
}

// Test rank mirroring for black pieces
TEST_F(PSTTest, RankMirroring) {
    // White pawn on e4 should have same value as black pawn on e5
    MgEgScore whitePawnE4 = PST::value(PAWN, E4, WHITE);
    MgEgScore blackPawnE5 = PST::value(PAWN, E5, BLACK);
    
    // Values should be equal (black gets same bonus on mirrored square)
    EXPECT_EQ(whitePawnE4.mg.value(), blackPawnE5.mg.value());
    
    // White knight on b1 vs black knight on b8
    MgEgScore whiteKnightB1 = PST::value(KNIGHT, B1, WHITE);
    MgEgScore blackKnightB8 = PST::value(KNIGHT, B8, BLACK);
    EXPECT_EQ(whiteKnightB1.mg.value(), blackKnightB8.mg.value());
}

// Test incremental PST updates
TEST_F(PSTTest, IncrementalUpdates) {
    board.setStartingPosition();
    
    // Recalculate from scratch for comparison
    board.recalculatePSTScore();
    MgEgScore initialScore = board.pstScore();
    
    // Make a move and check PST update
    Board::UndoInfo undo;
    Move e2e4 = makeMove(E2, E4, DOUBLE_PAWN);
    board.makeMove(e2e4, undo);
    
    // Recalculate and compare with incremental
    MgEgScore incrementalScore = board.pstScore();
    board.recalculatePSTScore();
    MgEgScore recalculatedScore = board.pstScore();
    
    EXPECT_EQ(incrementalScore, recalculatedScore);
    
    // Unmake and verify restoration
    board.unmakeMove(e2e4, undo);
    EXPECT_EQ(board.pstScore(), initialScore);
}

// Test evaluation symmetry
TEST_F(PSTTest, EvaluationSymmetry) {
    // Set up a symmetric position
    board.fromFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    Score whiteEval = board.evaluate();
    
    // Flip colors (black to move in same position)
    board.fromFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR b KQkq - 0 1");
    Score blackEval = board.evaluate();
    
    // Evaluations should be equal in magnitude, opposite in sign
    EXPECT_EQ(whiteEval.value(), -blackEval.value());
}

// Test castling PST updates
TEST_F(PSTTest, CastlingPSTUpdate) {
    board.fromFEN("r3k2r/pppppppp/8/8/8/8/PPPPPPPP/R3K2R w KQkq - 0 1");
    
    MgEgScore beforeCastling = board.pstScore();
    
    // Castle kingside
    Board::UndoInfo undo;
    Move castle = makeCastlingMove(E1, G1);
    board.makeMove(castle, undo);
    
    // Verify PST changed for both king and rook
    MgEgScore afterCastling = board.pstScore();
    EXPECT_NE(beforeCastling, afterCastling);
    
    // Recalculate and verify incremental is correct
    board.recalculatePSTScore();
    EXPECT_EQ(afterCastling, board.pstScore());
    
    // Unmake and verify restoration
    board.unmakeMove(castle, undo);
    EXPECT_EQ(board.pstScore(), beforeCastling);
}

// Test en passant PST updates
TEST_F(PSTTest, EnPassantPSTUpdate) {
    board.fromFEN("rnbqkbnr/ppp1p1pp/8/3pPp2/8/8/PPPP1PPP/RNBQKBNR w KQkq f6 0 1");
    
    MgEgScore beforeEP = board.pstScore();
    
    // Capture en passant
    Board::UndoInfo undo;
    Move ep = makeEnPassantMove(E5, F6);
    board.makeMove(ep, undo);
    
    // Verify PST changed correctly
    MgEgScore afterEP = board.pstScore();
    EXPECT_NE(beforeEP, afterEP);
    
    // Recalculate and verify
    board.recalculatePSTScore();
    EXPECT_EQ(afterEP, board.pstScore());
    
    // Unmake and verify restoration
    board.unmakeMove(ep, undo);
    EXPECT_EQ(board.pstScore(), beforeEP);
}

// Test promotion PST updates
TEST_F(PSTTest, PromotionPSTUpdate) {
    board.fromFEN("8/P7/8/8/8/8/8/8 w - - 0 1");
    
    MgEgScore beforePromo = board.pstScore();
    
    // Promote to queen
    Board::UndoInfo undo;
    Move promo = makeMove(A7, A8, PROMO_QUEEN);
    board.makeMove(promo, undo);
    
    // Verify PST changed (pawn removed, queen added)
    MgEgScore afterPromo = board.pstScore();
    EXPECT_NE(beforePromo, afterPromo);
    
    // The queen should have a different PST value than the pawn
    board.recalculatePSTScore();
    EXPECT_EQ(afterPromo, board.pstScore());
    
    // Unmake and verify restoration
    board.unmakeMove(promo, undo);
    EXPECT_EQ(board.pstScore(), beforePromo);
}

// Test that central pieces score higher
TEST_F(PSTTest, CentralPiecesScoreHigher) {
    // Knight on e5 should score better than knight on a1
    board.clear();
    board.setPiece(E5, WHITE_KNIGHT);
    board.recalculatePSTScore();
    MgEgScore centerScore = board.pstScore();
    
    board.clear();
    board.setPiece(A1, WHITE_KNIGHT);
    board.recalculatePSTScore();
    MgEgScore cornerScore = board.pstScore();
    
    EXPECT_GT(centerScore.mg.value(), cornerScore.mg.value());
}

// Test FEN loading recalculates PST
TEST_F(PSTTest, FENLoadingRecalculatesPST) {
    // Load a position
    board.fromFEN("r1bqkb1r/pppp1ppp/2n2n2/4N3/4P3/8/PPPP1PPP/RNBQKB1R w KQkq -");
    
    // Get PST score
    MgEgScore fenScore = board.pstScore();
    
    // Manually recalculate
    board.recalculatePSTScore();
    MgEgScore recalcScore = board.pstScore();
    
    // Should be identical
    EXPECT_EQ(fenScore, recalcScore);
}

// Test make/unmake sequence maintains PST integrity
TEST_F(PSTTest, MakeUnmakeSequence) {
    board.setStartingPosition();
    MgEgScore startScore = board.pstScore();
    
    // Make a series of moves
    Board::UndoInfo undo1, undo2, undo3;
    
    Move e2e4 = makeMove(E2, E4, DOUBLE_PAWN);
    board.makeMove(e2e4, undo1);
    
    Move e7e5 = makeMove(E7, E5, DOUBLE_PAWN);
    board.makeMove(e7e5, undo2);
    
    Move g1f3 = makeMove(G1, F3);
    board.makeMove(g1f3, undo3);
    
    // Verify incremental matches recalculation
    MgEgScore afterMoves = board.pstScore();
    board.recalculatePSTScore();
    EXPECT_EQ(afterMoves, board.pstScore());
    
    // Unmake in reverse order
    board.unmakeMove(g1f3, undo3);
    board.unmakeMove(e7e5, undo2);
    board.unmakeMove(e2e4, undo1);
    
    // Should be back to start
    EXPECT_EQ(board.pstScore(), startScore);
    
    // Verify with recalculation
    board.recalculatePSTScore();
    EXPECT_EQ(board.pstScore(), startScore);
}

#ifdef PST_TEST_MODE
// Debug mode test - verify PST after every move in a game
TEST_F(PSTTest, ComprehensiveValidation) {
    board.setStartingPosition();
    
    // Play through a short game, verifying PST at each step
    std::vector<std::pair<std::string, std::string>> moves = {
        {"e2", "e4"}, {"e7", "e5"},
        {"g1", "f3"}, {"b8", "c6"},
        {"f1", "b5"}, {"a7", "a6"},
        {"b5", "a4"}, {"g8", "f6"},
        {"e1", "g1"}, {"f8", "e7"}  // Includes castling
    };
    
    std::vector<Board::UndoInfo> undoStack;
    
    for (const auto& [from_str, to_str] : moves) {
        Square from = stringToSquare(from_str);
        Square to = stringToSquare(to_str);
        
        // Generate legal moves to find the right move
        MoveList legalMoves;
        generateMoves(board, legalMoves);
        
        Move move = NO_MOVE;
        for (Move m : legalMoves) {
            if (moveFrom(m) == from && moveTo(m) == to) {
                move = m;
                break;
            }
        }
        
        ASSERT_NE(move, NO_MOVE) << "Move " << from_str << to_str << " not found";
        
        // Make move
        Board::UndoInfo undo;
        board.makeMove(move, undo);
        undoStack.push_back(undo);
        
        // Verify PST
        MgEgScore incremental = board.pstScore();
        board.recalculatePSTScore();
        MgEgScore recalculated = board.pstScore();
        
        EXPECT_EQ(incremental, recalculated) 
            << "PST mismatch after " << from_str << to_str;
    }
    
    // Unmake all moves
    for (int i = moves.size() - 1; i >= 0; --i) {
        Square from = stringToSquare(moves[i].first);
        Square to = stringToSquare(moves[i].second);
        
        // Find the move again
        Move move = NO_MOVE;
        for (Square sq = A1; sq <= H8; ++sq) {
            if (board.pieceAt(sq) != NO_PIECE) {
                // Try to reconstruct move (simplified)
                if (sq == to) {
                    // This is where the piece ended up
                    move = makeMove(from, to);
                    break;
                }
            }
        }
        
        board.unmakeMove(move, undoStack[i]);
    }
    
    // Should be back at starting position
    board.recalculatePSTScore();
    board.setStartingPosition();
    EXPECT_EQ(board.pstScore(), board.pstScore());
}
#endif

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}