#include "../src/core/board.h"
#include "../src/core/types.h"
#include <iostream>

using namespace seajay;

int main() {
    Board board;
    
    // Position after d7-d5 with white pawn on e4
    board.parseFEN("rnbqkbnr/ppp1pppp/8/3p4/4P3/8/PPPP1PPP/RNBQKBNR w KQkq d6 0 2");
    
    std::cout << "Testing en passant detection for position after d7-d5\n";
    std::cout << "======================================================\n\n";
    
    Square d6 = makeSquare(3, 5);  // file d=3, rank 6=5
    std::cout << "En passant square d6: file=" << (int)fileOf(d6) << " rank=" << (int)rankOf(d6) << "\n";
    
    // The en passant square is d6 (rank 6, file 3)
    // The black pawn that moved is on d5 (rank 5, file 3) 
    // White pawns that could capture would be on e5 or c5 (rank 5, files 4 and 2)
    
    // But wait... the white pawn is on e4, not e5!
    // After d7-d5, white pawn on e4 can capture en passant to d6
    // So we need to check rank 4 when en passant is on rank 6
    
    std::cout << "\nChecking for white pawns that can capture en passant:\n";
    
    // For en passant on rank 6 (black double-push), white pawns must be on rank 4
    Rank pawnRank = 3;  // rank 4 (0-indexed)
    std::cout << "Looking for white pawns on rank " << (pawnRank+1) << " (0-indexed: " << pawnRank << ")\n";
    
    // Check c4
    Square c4 = makeSquare(2, 3);
    Piece pc4 = board.pieceAt(c4);
    std::cout << "c4: " << (pc4 == WHITE_PAWN ? "White pawn" : pc4 == NO_PIECE ? "Empty" : "Other") << "\n";
    
    // Check e4
    Square e4 = makeSquare(4, 3);
    Piece pe4 = board.pieceAt(e4);
    std::cout << "e4: " << (pe4 == WHITE_PAWN ? "White pawn" : pe4 == NO_PIECE ? "Empty" : "Other") << "\n";
    
    if (pe4 == WHITE_PAWN) {
        std::cout << "\n✓ White pawn on e4 CAN capture en passant to d6!\n";
        std::cout << "The en passant square SHOULD be included in the hash.\n";
    } else {
        std::cout << "\n✗ No white pawn can capture en passant.\n";
    }
    
    return 0;
}