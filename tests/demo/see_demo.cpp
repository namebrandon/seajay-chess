/**
 * SeaJay Chess Engine - Stage 15: Static Exchange Evaluation
 * Demo program showing SEE in action
 */

#include "core/board.h"
#include "core/see.h"
#include <iostream>
#include <iomanip>
#include <string>

using namespace seajay;

void showSEE(Board& board, const std::string& moveStr, const std::string& description) {
    // Parse the move (simple format like "e4d5")
    if (moveStr.length() < 4) {
        std::cout << "Invalid move format\n";
        return;
    }
    
    Square from = static_cast<Square>((moveStr[1] - '1') * 8 + (moveStr[0] - 'a'));
    Square to = static_cast<Square>((moveStr[3] - '1') * 8 + (moveStr[2] - 'a'));
    
    // Check if it's a capture
    bool isCapture = (board.pieceAt(to) != NO_PIECE);
    Move move = makeMove(from, to, isCapture ? CAPTURE : 0);
    
    // Special moves
    if (board.pieceAt(from) == WHITE_PAWN || board.pieceAt(from) == BLACK_PAWN) {
        // Check for en passant
        if (board.enPassantSquare() != NO_SQUARE && to == board.enPassantSquare()) {
            move = makeEnPassantMove(from, to);
        }
        // Check for promotion
        else if ((board.pieceAt(from) == WHITE_PAWN && rankOf(to) == 7) ||
                 (board.pieceAt(from) == BLACK_PAWN && rankOf(to) == 0)) {
            // Default to queen promotion
            move = makePromotionMove(from, to, QUEEN);
        }
    }
    
    SEEValue value = see(board, move);
    
    std::cout << std::setw(8) << moveStr << " | "
              << std::setw(6) << value << " | "
              << description << "\n";
}

int main() {
    std::cout << "=== SeaJay SEE Demo ===\n\n";
    
    Board board;
    
    // Test 1: Simple pawn trade
    std::cout << "Test 1: Simple exchanges\n";
    std::cout << "------------------------------------\n";
    std::cout << "   Move  | Value | Description\n";
    std::cout << "------------------------------------\n";
    
    board.fromFEN("rnbqkbnr/ppp1pppp/8/3p4/4P3/8/PPPP1PPP/RNBQKBNR w KQkq - 0 2");
    showSEE(board, "e4d5", "PxP equal trade");
    
    board.fromFEN("r1bqkbnr/pppp1ppp/2n5/4p3/2B1P3/5N2/PPPP1PPP/RNBQK2R w KQkq - 3 3");
    showSEE(board, "f3e5", "NxP (defended)");
    
    board.fromFEN("rnbqkb1r/pppp1ppp/5n2/4p3/2B1P3/5N2/PPPP1PPP/RNBQK2R w KQkq - 3 3");
    showSEE(board, "c4f7", "BxP check (good)");
    
    // Test 2: Bad captures
    std::cout << "\nTest 2: Bad captures\n";
    std::cout << "------------------------------------\n";
    std::cout << "   Move  | Value | Description\n";
    std::cout << "------------------------------------\n";
    
    board.clear();
    board.setPiece(D1, WHITE_QUEEN);
    board.setPiece(D5, BLACK_PAWN);
    board.setPiece(C6, BLACK_KNIGHT);
    board.setPiece(E6, BLACK_BISHOP);
    board.setSideToMove(WHITE);
    showSEE(board, "d1d5", "QxP (defended 2x)");
    
    board.clear();
    board.setPiece(E1, WHITE_ROOK);
    board.setPiece(E5, BLACK_BISHOP);
    board.setPiece(D6, BLACK_PAWN);
    board.setPiece(F6, BLACK_PAWN);
    board.setSideToMove(WHITE);
    showSEE(board, "e1e5", "RxB (defended by pawns)");
    
    // Test 3: Complex multi-piece exchanges
    std::cout << "\nTest 3: Multi-piece exchanges\n";
    std::cout << "------------------------------------\n";
    std::cout << "   Move  | Value | Description\n";
    std::cout << "------------------------------------\n";
    
    board.clear();
    board.setPiece(E4, WHITE_PAWN);
    board.setPiece(C1, WHITE_BISHOP);
    board.setPiece(B1, WHITE_KNIGHT);
    board.setPiece(A1, WHITE_ROOK);
    board.setPiece(D5, BLACK_PAWN);
    board.setPiece(C6, BLACK_KNIGHT);
    board.setPiece(F8, BLACK_BISHOP);
    board.setPiece(A8, BLACK_ROOK);
    board.setSideToMove(WHITE);
    showSEE(board, "e4d5", "Complex sequence");
    
    // Test 4: King participation
    std::cout << "\nTest 4: King participation\n";
    std::cout << "------------------------------------\n";
    std::cout << "   Move  | Value | Description\n";
    std::cout << "------------------------------------\n";
    
    board.clear();
    board.setPiece(E4, WHITE_PAWN);
    board.setPiece(D5, BLACK_PAWN);
    board.setPiece(E6, BLACK_KING);
    board.setSideToMove(WHITE);
    showSEE(board, "e4d5", "PxP, king recaptures");
    
    board.clear();
    board.setPiece(E4, WHITE_KING);
    board.setPiece(D5, BLACK_PAWN);
    board.setPiece(D8, BLACK_ROOK);
    board.setSideToMove(WHITE);
    showSEE(board, "e4d5", "KxP (rook can't capture king)");
    
    // Test 5: Special moves
    std::cout << "\nTest 5: Special moves\n";
    std::cout << "------------------------------------\n";
    std::cout << "   Move  | Value | Description\n";
    std::cout << "------------------------------------\n";
    
    // En passant
    board.fromFEN("rnbqkbnr/1ppppppp/8/pP6/8/8/P1PPPPPP/RNBQKBNR w KQkq a6 0 2");
    showSEE(board, "b5a6", "En passant capture");
    
    // Promotion
    board.clear();
    board.setPiece(B7, WHITE_PAWN);
    board.setPiece(A8, BLACK_ROOK);
    board.setPiece(C8, BLACK_BISHOP);
    board.setSideToMove(WHITE);
    showSEE(board, "b7b8", "Promotion (defended)");
    showSEE(board, "b7a8", "PxR with promotion");
    showSEE(board, "b7c8", "PxB with promotion");
    
    std::cout << "\n=== SEE Demo Complete ===\n";
    std::cout << "SEE helps with:\n";
    std::cout << "- Move ordering (good captures first)\n";
    std::cout << "- Pruning bad captures in quiescence search\n";
    std::cout << "- Evaluation of tactical sequences\n";
    
    return 0;
}