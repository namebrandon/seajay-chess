#include <iostream>
#include "src/core/board.h"
#include "src/core/move_generation.h"
#include "src/core/move_list.h"

using namespace seajay;

int main() {
    Board board;
    board.fromFEN("rnbqkbnr/ppp1pppp/8/2p1p3/3P4/8/PPP1PPPP/RNBQKBNR b KQkq - 0 1");
    
    std::cout << board.toString() << std::endl;
    
    // Check what pieces are where
    std::cout << "c5: " << PIECE_CHARS[board.pieceAt(C5)] << std::endl;
    std::cout << "e5: " << PIECE_CHARS[board.pieceAt(E5)] << std::endl;
    std::cout << "d4: " << PIECE_CHARS[board.pieceAt(D4)] << std::endl;
    
    MoveList moves;
    MoveGenerator::generatePseudoLegalMoves(board, moves);
    
    std::cout << "Total moves: " << moves.size() << std::endl;
    
    // Show all moves from c5 and e5
    std::cout << "\nMoves from c5:" << std::endl;
    for (size_t i = 0; i < moves.size(); ++i) {
        Move m = moves[i];
        if (moveFrom(m) == C5) {
            std::cout << "  " << squareToString(moveFrom(m)) << squareToString(moveTo(m));
            if (isCapture(m)) std::cout << " CAPTURE";
            std::cout << std::endl;
        }
    }
    
    std::cout << "\nMoves from e5:" << std::endl;
    for (size_t i = 0; i < moves.size(); ++i) {
        Move m = moves[i];
        if (moveFrom(m) == E5) {
            std::cout << "  " << squareToString(moveFrom(m)) << squareToString(moveTo(m));
            if (isCapture(m)) std::cout << " CAPTURE";
            std::cout << std::endl;
        }
    }
    
    // Check all captures
    std::cout << "\nAll captures:" << std::endl;
    for (size_t i = 0; i < moves.size(); ++i) {
        Move m = moves[i];
        if (isCapture(m)) {
            std::cout << "  " << squareToString(moveFrom(m)) << squareToString(moveTo(m)) << std::endl;
        }
    }
    
    return 0;
}