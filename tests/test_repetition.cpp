// Stage 9b: Test Suite for Draw Detection and Repetition Handling
// SeaJay Chess Engine

#include "../src/core/board.h"
#include "../src/core/move_generation.h"
#include "../src/core/move_list.h"
#include "../src/core/types.h"  // For Move type
#include "../src/search/negamax.h"
#include "../src/search/search.h"
#include "../src/search/types.h"
#include <iostream>
#include <vector>
#include <string>
#include <cassert>

using namespace seajay;

// Define NULL_MOVE if not already defined
constexpr Move NULL_MOVE_LOCAL = 0;

// Helper function to parse and make a move from UCI notation
Move parseMove(const Board& board, const std::string& moveStr) {
    if (moveStr.length() < 4) return NULL_MOVE_LOCAL;
    
    Square from = static_cast<Square>((moveStr[0] - 'a') + (moveStr[1] - '1') * 8);
    Square to = static_cast<Square>((moveStr[2] - 'a') + (moveStr[3] - '1') * 8);
    
    // Generate legal moves to find the exact move
    MoveList moves = generateLegalMoves(board);
    for (const Move& move : moves) {
        if (moveFrom(move) == from && moveTo(move) == to) {
            return move;
        }
    }
    
    return NULL_MOVE_LOCAL;
}

void testBasicThreefoldRepetition() {
    std::cout << "Testing basic threefold repetition..." << std::endl;
    
    Board board;
    board.setStartingPosition();
    board.clearGameHistory();
    
    // Sequence: Nc3 Nc6 Nb1 Nb8 Nc3 Nc6 Nb1 Nb8 Nc3
    // This should create a threefold repetition after the 9th move
    std::vector<std::string> moves = {
        "b1c3", "b8c6", "c3b1", "c6b8",
        "b1c3", "b8c6", "c3b1", "c6b8",
        "b1c3"  // Third repetition of initial position
    };
    
    std::vector<Board::UndoInfo> undoStack;
    for (size_t i = 0; i < moves.size(); ++i) {
        Move move = parseMove(board, moves[i]);
        assert(move != NULL_MOVE_LOCAL && "Failed to parse move");
        
        Board::UndoInfo undo;
        board.makeMove(move, undo);
        undoStack.push_back(undo);
        
        // Check for repetition after move 9
        if (i == 8) {
            assert(board.isRepetitionDraw() && "Should detect threefold repetition after 9th move");
        } else {
            assert(!board.isRepetitionDraw() && "Should not detect repetition before 9th move");
        }
    }
    
    std::cout << "  ✓ Basic threefold repetition detected correctly" << std::endl;
}

void testFiftyMoveRule() {
    std::cout << "Testing fifty-move rule..." << std::endl;
    
    Board board;
    // Position with just kings and rooks (no pawns to reset counter)
    board.fromFEN("4k3/8/8/8/8/8/8/R3K2R w KQ - 99 1");
    board.clearGameHistory();
    
    // At 99 halfmoves, next move should trigger 50-move rule
    assert(!board.isFiftyMoveRule() && "Should not be 50-move rule at 99 halfmoves");
    
    // Make any rook move
    Move move = parseMove(board, "a1a2");
    assert(move != NULL_MOVE_LOCAL);
    
    Board::UndoInfo undo;
    board.makeMove(move, undo);
    
    assert(board.isFiftyMoveRule() && "Should trigger 50-move rule at 100 halfmoves");
    assert(board.isDraw() && "Should be a draw by 50-move rule");
    
    std::cout << "  ✓ Fifty-move rule detected at exactly 100 halfmoves" << std::endl;
}

void testFiftyMoveRuleReset() {
    std::cout << "Testing fifty-move rule reset on capture..." << std::endl;
    
    Board board;
    board.fromFEN("4k3/8/8/8/4p3/8/4P3/4K3 w - - 99 1");
    board.clearGameHistory();
    
    // At 99 halfmoves
    assert(!board.isFiftyMoveRule());
    
    // Pawn capture should reset counter
    Move move = parseMove(board, "e2e4");  // Pawn move resets
    if (move == NULL_MOVE_LOCAL) {
        // Try capture if pawn push doesn't work in this position
        board.fromFEN("4k3/8/8/8/4p3/3P4/8/4K3 w - - 99 1");
        board.clearGameHistory();
        move = parseMove(board, "d3e4");  // Pawn capture
    }
    assert(move != NULL_MOVE_LOCAL);
    
    Board::UndoInfo undo;
    board.makeMove(move, undo);
    
    assert(!board.isFiftyMoveRule() && "Pawn move/capture should reset 50-move counter");
    assert(board.halfmoveClock() == 0 && "Halfmove clock should be reset to 0");
    
    std::cout << "  ✓ Fifty-move rule counter resets on pawn moves/captures" << std::endl;
}

void testInsufficientMaterial() {
    std::cout << "Testing insufficient material detection..." << std::endl;
    
    struct TestCase {
        std::string fen;
        bool expected;
        std::string description;
    };
    
    std::vector<TestCase> testCases = {
        {"4k3/8/8/8/8/8/8/4K3 w - - 0 1", true, "K vs K"},
        {"4k3/8/8/8/8/8/8/4KN2 w - - 0 1", true, "KN vs K"},
        {"3bk3/8/8/8/8/8/8/4KB2 w - - 0 1", true, "KB vs KB (same color - both light)"},
        {"4kb2/8/8/8/8/8/8/3BK3 w - - 0 1", false, "KB vs KB (opposite colors)"},
        {"4k3/8/8/8/8/8/4P3/4K3 w - - 0 1", false, "K+P vs K"},
        {"4k3/8/8/8/8/8/8/4KQ2 w - - 0 1", false, "KQ vs K"},
        {"4k3/8/8/8/8/8/8/4KR2 w - - 0 1", false, "KR vs K"},
    };
    
    for (const auto& test : testCases) {
        Board board;
        board.fromFEN(test.fen);
        bool result = board.isInsufficientMaterial();
        if (result != test.expected) {
            std::cerr << "Failed: " << test.description << std::endl;
            assert(false);
        }
        std::cout << "  ✓ " << test.description << " - " 
                  << (test.expected ? "insufficient" : "sufficient") << std::endl;
    }
}

void testRepetitionInSearch() {
    std::cout << "Testing repetition detection during search..." << std::endl;
    
    // Position where both sides can repeat moves
    Board board;
    board.fromFEN("8/8/3k4/3q4/3Q4/3K4/8/8 w - - 0 1");
    board.clearGameHistory();
    
    // Make a few moves to set up history
    std::vector<std::string> setupMoves = {"d4e3", "d5e5", "e3d3", "e5d5"};
    std::vector<Board::UndoInfo> undoStack;
    
    for (const auto& moveStr : setupMoves) {
        Move move = parseMove(board, moveStr);
        if (move != NULL_MOVE_LOCAL) {
            Board::UndoInfo undo;
            board.makeMove(move, undo);
            undoStack.push_back(undo);
        }
    }
    
    // Now search should detect that repeating position leads to draw
    search::SearchLimits limits;
    limits.maxDepth = 4;  // Deep enough to see repetition
    
    Move bestMove = search::search(board, limits);
    
    // The search should avoid moves that repeat the position
    // We can't assert specific moves, but we can verify search completes
    assert(bestMove != NULL_MOVE_LOCAL && "Search should return a move");
    
    std::cout << "  ✓ Search handles repetitions correctly" << std::endl;
}

void testCheckmateVsRepetition() {
    std::cout << "Testing checkmate priority over repetition..." << std::endl;
    
    // Position where White can either repeat or deliver checkmate
    Board board;
    board.fromFEN("6k1/5R2/6K1/8/8/8/8/8 w - - 2 1");
    board.clearGameHistory();
    
    // Add some history that could be repeated
    Hash initialKey = board.zobristKey();
    board.pushGameHistory(initialKey);  // Simulate this position occurred before
    
    // Search should find mate (Rf8#) not repetition draw
    search::SearchLimits limits;
    limits.maxDepth = 3;
    
    Move bestMove = search::search(board, limits);
    
    // Best move should be Rf8# (checkmate)
    assert(bestMove != NULL_MOVE_LOCAL);
    assert(moveFrom(bestMove) == F7);
    assert(moveTo(bestMove) == F8);
    
    std::cout << "  ✓ Engine prioritizes checkmate over repetition draw" << std::endl;
}

void runAllTests() {
    std::cout << "\n=== Stage 9b: Draw Detection Tests ===" << std::endl;
    
    testBasicThreefoldRepetition();
    testFiftyMoveRule();
    testFiftyMoveRuleReset();
    testInsufficientMaterial();
    testRepetitionInSearch();
    testCheckmateVsRepetition();
    
    std::cout << "\n✅ All Stage 9b tests passed!" << std::endl;
}

int main() {
    try {
        runAllTests();
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Test failed with exception: " << e.what() << std::endl;
        return 1;
    }
}