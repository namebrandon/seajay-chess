#include <iostream>
#include "src/core/board.h"

using namespace seajay;

int main() {
    Board board;
    
    // Clear the board first
    board.clear();
    
    // Manually parse just the board position part
    std::string fen = "rnbqkbnr/ppp1pppp/8/2p1p3/3P4/8/PPP1PPPP/RNBQKBNR";
    
    std::cout << "Parsing board position: " << fen << std::endl;
    
    auto result = board.parseBoardPosition(fen);
    if (result.hasValue()) {
        std::cout << "Board position parsed successfully" << std::endl;
    } else {
        std::cout << "Board position parse failed: " << result.error().message << std::endl;
        return 1;
    }
    
    std::cout << "\nBoard after parsing position only:" << std::endl;
    std::cout << board.toString() << std::endl;
    
    // Check kings
    std::cout << "\nChecking kings:" << std::endl;
    Bitboard whiteKing = board.pieces(WHITE, KING);
    Bitboard blackKing = board.pieces(BLACK, KING);
    
    std::cout << "White king bitboard: 0x" << std::hex << whiteKing << std::dec << std::endl;
    std::cout << "Black king bitboard: 0x" << std::hex << blackKing << std::dec << std::endl;
    
    if (whiteKing) {
        Square wk = lsb(whiteKing);
        std::cout << "White king at: " << squareToString(wk) << std::endl;
    }
    
    if (blackKing) {
        Square bk = lsb(blackKing);
        std::cout << "Black king at: " << squareToString(bk) << std::endl;
    }
    
    std::cout << "\nValidation results:" << std::endl;
    std::cout << "Kings valid: " << (board.validateKings() ? "YES" : "NO") << std::endl;
    
    return 0;
}