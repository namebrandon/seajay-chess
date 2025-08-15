#include "../../src/core/board.h"
#include "../../src/core/see.h"
#include <iostream>

using namespace seajay;

int main() {
    Board board;
    board.fromFEN("3r2k1/p2r1p1p/1p2p1p1/q4n2/3P4/PQ5P/1P1RNPP1/3R2K1 b - - 0 1");
    
    std::cout << board.toString() << "\n";
    
    std::cout << "Position analysis:\n";
    std::cout << "Black Knight at f5\n";
    std::cout << "White Pawn at d4\n";
    std::cout << "White Queen at b3\n";
    std::cout << "White Rook at d2\n";
    std::cout << "White Rook at d1\n";
    
    std::cout << "\nMove: Nf5xd4 (Black to move)\n";
    std::cout << "Knight takes pawn: +100\n";
    std::cout << "Queen can recapture: -325\n";
    std::cout << "Black has queen on a5 but can't reach d4\n";
    
    Move capture = makeMove(F5, D4);
    SEEValue value = see(board, capture);
    std::cout << "\nSEE value: " << value << "\n";
    std::cout << "Expected: Knight for pawn is bad when queen recaptures\n";
    
    return 0;
}