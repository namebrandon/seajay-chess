#include "src/core/board.h"
#include "src/core/move_generation.h"
#include "src/core/move_list.h"
#include "src/evaluation/evaluate.h"
#include <iostream>
#include <iomanip>
#include <sstream>

using namespace seajay;

// Simple move to string conversion
std::string moveToString(Move move) {
    std::stringstream ss;
    Square from = moveFrom(move);
    Square to = moveTo(move);
    
    ss << char('a' + (from % 8)) << char('1' + (from / 8))
       << char('a' + (to % 8)) << char('1' + (to / 8));
    
    if (isPromotion(move)) {
        PieceType pt = promotionType(move);
        char promoChar = pt == QUEEN ? 'q' : pt == ROOK ? 'r' : pt == BISHOP ? 'b' : 'n';
        ss << promoChar;
    }
    
    return ss.str();
}

// Parse a move from UCI string
Move parseMove(const std::string& moveStr, const Board& board) {
    if (moveStr.length() < 4) return NO_MOVE;
    
    int fromFile = moveStr[0] - 'a';
    int fromRank = moveStr[1] - '1';
    int toFile = moveStr[2] - 'a';
    int toRank = moveStr[3] - '1';
    
    Square from = static_cast<Square>(fromRank * 8 + fromFile);
    Square to = static_cast<Square>(toRank * 8 + toFile);
    
    // Generate legal moves to find the matching one
    MoveList moves;
    generateLegalMoves(const_cast<Board&>(board), moves);
    
    for (const Move& move : moves) {
        if (moveFrom(move) == from && moveTo(move) == to) {
            return move;
        }
    }
    
    return NO_MOVE;
}

void analyzePosition(Board& board, const std::string& description) {
    std::cout << "\n=== " << description << " ===\n";
    std::cout << "FEN: " << board.toFen() << "\n";
    std::cout << "Side to move: " << (board.sideToMove() == WHITE ? "White" : "Black") << "\n";
    
    // Get evaluation from current side's perspective
    eval::Score eval = eval::evaluate(board);
    std::cout << "Evaluation (from side-to-move perspective): " << eval.value() << "\n";
    
    // Generate all legal moves
    MoveList moves;
    generateLegalMoves(board, moves);
    std::cout << "Number of legal moves: " << moves.size() << "\n";
    
    // Show first 10 moves and their evaluations after making them
    std::cout << "\nFirst 10 moves and resulting evaluations:\n";
    int count = 0;
    for (const Move& move : moves) {
        if (count >= 10) break;
        
        UndoInfo undo;
        board.makeMove(move, undo);
        eval::Score evalAfter = eval::evaluate(board);
        board.unmakeMove(move, undo);
        
        // Note: evalAfter is from opponent's perspective after the move
        // So we negate it to get it from our perspective
        std::cout << std::setw(8) << moveToString(move) 
                  << " -> eval: " << std::setw(6) << (-evalAfter.value()) 
                  << " (diff: " << std::setw(5) << ((-evalAfter.value()) - eval.value()) << ")\n";
        count++;
    }
}

int main() {
    Board board;
    
    // Test 1: Starting position
    board.setStartingPosition();
    analyzePosition(board, "Starting Position - White to move");
    
    // Test 2: After 1.d4
    board.setStartingPosition();
    Move d4 = parseMove("d2d4", board);
    UndoInfo undo1;
    board.makeMove(d4, undo1);
    analyzePosition(board, "After 1.d4 - Black to move");
    
    // Test 3: After 1.d4 a6
    Move a6 = parseMove("a7a6", board);
    UndoInfo undo2;
    board.makeMove(a6, undo2);
    analyzePosition(board, "After 1.d4 a6 - White to move");
    
    // Test 4: Starting position - Black's perspective
    // Make a null move to switch sides
    board.setStartingPosition();
    board.setFromFen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR b KQkq - 0 1");
    analyzePosition(board, "Starting Position - Black to move (hypothetical)");
    
    // Test 5: Symmetric position
    board.setFromFen("rnbqkbnr/pppp1ppp/8/4p3/4P3/8/PPPP1PPP/RNBQKBNR w KQkq - 0 2");
    analyzePosition(board, "After 1.e4 e5 - White to move (symmetric)");
    
    // Test 6: Same position, Black to move
    board.setFromFen("rnbqkbnr/pppp1ppp/8/4p3/4P3/8/PPPP1PPP/RNBQKBNR b KQkq - 0 2");
    analyzePosition(board, "After 1.e4 e5 - Black to move (symmetric)");
    
    return 0;
}