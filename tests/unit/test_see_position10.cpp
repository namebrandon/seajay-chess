#include "../../src/core/board.h"
#include "../../src/core/see.h"
#include <iostream>

using namespace seajay;

int main() {
    Board board;
    board.fromFEN("r3k2r/8/8/3q4/3Q4/8/8/R3K2R w - - 0 1");
    
    std::cout << board.toString() << "\n";
    
    std::cout << "Position analysis:\n";
    std::cout << "White Queen at d4\n";
    std::cout << "Black Queen at d5\n";
    std::cout << "White Rooks at a1 and h1\n";
    std::cout << "Black Rooks at a8 and h8\n";
    
    std::cout << "\nMove: Qd4xd5 (White to move)\n";
    std::cout << "Queen takes queen: +975\n";
    std::cout << "No piece can recapture (rooks not on d-file)\n";
    std::cout << "This wins the queen\n";
    
    Move capture = makeMove(D4, D5);
    SEEValue value = see(board, capture);
    std::cout << "\nSEE value: " << value << " (expected +975 - win a queen)\n";
    
    return 0;
}