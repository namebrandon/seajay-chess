#include "../src/core/board.h"
#include "../src/core/types.h"
#include <iostream>

using namespace seajay;

int main() {
    Board board;
    
    // Position after d7-d5 with white pawn on e4
    board.parseFEN("rnbqkbnr/ppp1pppp/8/3p4/4P3/8/PPPP1PPP/RNBQKBNR w KQkq d6 0 2");
    
    std::cout << "Board after d7-d5:\n";
    std::cout << board.toString() << "\n";
    
    std::cout << "En passant square: ";
    if (board.enPassantSquare() != NO_SQUARE) {
        std::cout << "d6 (square " << (int)board.enPassantSquare() << ")\n";
    } else {
        std::cout << "None\n";
    }
    
    std::cout << "Side to move: " << (board.sideToMove() == WHITE ? "White" : "Black") << "\n";
    
    // Check what's on e4
    Square e4 = makeSquare(4, 3);  // file e=4, rank 4=3
    Piece p = board.pieceAt(e4);
    std::cout << "Piece on e4: ";
    if (p == WHITE_PAWN) std::cout << "White pawn\n";
    else if (p == NO_PIECE) std::cout << "Empty\n";
    else std::cout << "Other piece (" << (int)p << ")\n";
    
    // Check what's on d5
    Square d5 = makeSquare(3, 4);  // file d=3, rank 5=4
    p = board.pieceAt(d5);
    std::cout << "Piece on d5: ";
    if (p == BLACK_PAWN) std::cout << "Black pawn\n";
    else if (p == NO_PIECE) std::cout << "Empty\n";
    else std::cout << "Other piece (" << (int)p << ")\n";
    
    // Check d6 square details
    Square d6 = makeSquare(3, 5);  // file d=3, rank 6=5
    std::cout << "d6 square number: " << (int)d6 << "\n";
    std::cout << "d6 file: " << (int)fileOf(d6) << " rank: " << (int)rankOf(d6) << "\n";
    
    return 0;
}