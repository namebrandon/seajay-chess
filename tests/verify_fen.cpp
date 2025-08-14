#include "../src/core/board.h"
#include <iostream>

using namespace seajay;

int main() {
    Board board;
    
    // Test the specific FEN
    board.parseFEN("rnbqkbnr/ppp1pppp/8/3p4/4P3/8/PPPP1PPP/RNBQKBNR w KQkq d6 0 2");
    
    std::cout << board.toString() << "\n";
    
    // Check ranks
    std::cout << "\nRank analysis (0-indexed):\n";
    std::cout << "Rank 4 (human 5): ";
    for (int f = 0; f < 8; f++) {
        Square sq = makeSquare(f, 4);
        Piece p = board.pieceAt(sq);
        if (p == BLACK_PAWN) std::cout << "p";
        else if (p == WHITE_PAWN) std::cout << "P";
        else std::cout << ".";
    }
    std::cout << "\n";
    
    std::cout << "Rank 3 (human 4): ";
    for (int f = 0; f < 8; f++) {
        Square sq = makeSquare(f, 3);
        Piece p = board.pieceAt(sq);
        if (p == BLACK_PAWN) std::cout << "p";
        else if (p == WHITE_PAWN) std::cout << "P";
        else std::cout << ".";
    }
    std::cout << "\n";
    
    std::cout << "\nWhite pawn is on e4 (rank 4 human, rank 3 0-indexed)\n";
    std::cout << "Black pawn is on d5 (rank 5 human, rank 4 0-indexed)\n";
    std::cout << "They are NOT on the same rank!\n";
    std::cout << "Therefore white CANNOT capture en passant.\n";
    
    return 0;
}