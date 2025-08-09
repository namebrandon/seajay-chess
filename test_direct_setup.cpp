#include <iostream>
#include "src/core/board.h"
#include "src/core/move_generation.h"

using namespace seajay;

int main() {
    std::cout << "Testing direct board setup without FEN..." << std::endl;
    
    Board board;
    board.clear();
    
    // Set up pieces manually instead of using FEN
    std::cout << "Setting up pieces manually..." << std::endl;
    
    // Set up white pieces
    board.setPiece(A1, WHITE_ROOK);
    board.setPiece(B1, WHITE_KNIGHT);
    board.setPiece(C1, WHITE_BISHOP);
    board.setPiece(D1, WHITE_QUEEN);
    board.setPiece(E1, WHITE_KING);
    board.setPiece(F1, WHITE_BISHOP);
    board.setPiece(G1, WHITE_KNIGHT);
    board.setPiece(H1, WHITE_ROOK);
    
    // Set up white pawns
    for (File f = 0; f < 8; ++f) {
        board.setPiece(makeSquare(f, 1), WHITE_PAWN);
    }
    
    // Set up black pieces
    board.setPiece(A8, BLACK_ROOK);
    board.setPiece(B8, BLACK_KNIGHT);
    board.setPiece(C8, BLACK_BISHOP);
    board.setPiece(D8, BLACK_QUEEN);
    board.setPiece(E8, BLACK_KING);
    board.setPiece(F8, BLACK_BISHOP);
    board.setPiece(G8, BLACK_KNIGHT);
    board.setPiece(H8, BLACK_ROOK);
    
    // Set up black pawns
    for (File f = 0; f < 8; ++f) {
        board.setPiece(makeSquare(f, 6), BLACK_PAWN);
    }
    
    // Set other game state
    board.setSideToMove(WHITE);
    board.setCastlingRights(ALL_CASTLING);
    board.setEnPassantSquare(NO_SQUARE);
    board.setHalfmoveClock(0);
    board.setFullmoveNumber(1);
    
    std::cout << "Board setup complete!" << std::endl;
    std::cout << board.toString() << std::endl;
    
    std::cout << "Testing move generation..." << std::endl;
    MoveList moves;
    MoveGenerator::generatePseudoLegalMoves(board, moves);
    
    std::cout << "Generated " << moves.size() << " pseudo-legal moves" << std::endl;
    
    return 0;
}