#include <iostream>
#include <string>
#include "../src/core/board.h"
#include "../src/core/move_generation.h"

using namespace seajay;

int main() {
    // Test case: Black king in check from rook
    std::string fen = "4k3/8/8/8/8/8/8/3KR3 b - - 0 1";
    Board board;
    board.fromFEN(fen);
    
    std::cout << "Original position:" << std::endl;
    std::cout << board.toString() << std::endl;
    
    // Make the move e8->d8
    Move e8d8 = makeMove(E8, D8, NORMAL);
    Board::UndoInfo undo;
    board.makeMove(e8d8, undo);
    
    std::cout << "\nAfter e8->d8:" << std::endl;
    std::cout << board.toString() << std::endl;
    
    // Check if d8 is attacked by white
    bool d8Attacked = MoveGenerator::isSquareAttacked(board, D8, WHITE);
    std::cout << "\nd8 attacked by WHITE: " << (d8Attacked ? "YES" : "NO") << std::endl;
    
    // Unmake and try e8->f8
    board.unmakeMove(e8d8, undo);
    
    Move e8f8 = makeMove(E8, F8, NORMAL);
    board.makeMove(e8f8, undo);
    
    std::cout << "\nAfter e8->f8:" << std::endl;
    std::cout << board.toString() << std::endl;
    
    bool f8Attacked = MoveGenerator::isSquareAttacked(board, F8, WHITE);
    std::cout << "\nf8 attacked by WHITE: " << (f8Attacked ? "YES" : "NO") << std::endl;
    
    return 0;
}