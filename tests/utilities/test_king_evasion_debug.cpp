#include <iostream>
#include <string>
#include "../src/core/board.h"
#include "../src/core/move_generation.h"
#include "../src/core/move_list.h"

using namespace seajay;

void testKingEvasion() {
    // Test case 1: Black king in check from rook on e-file
    std::string fen = "4k3/8/8/8/8/8/8/3KR3 b - - 0 1";
    Board board;
    board.fromFEN(fen);
    
    std::cout << "Position: Black king in check from rook" << std::endl;
    std::cout << board.toString() << std::endl;
    
    // Check if we're in check
    bool inCheck = MoveGenerator::inCheck(board);
    std::cout << "In check: " << (inCheck ? "YES" : "NO") << std::endl;
    
    // Generate all legal moves  
    MoveList moves;
    MoveGenerator::generateLegalMoves(board, moves);
    
    std::cout << "All legal moves (" << moves.size() << "):" << std::endl;
    for (size_t i = 0; i < moves.size(); ++i) {
        Move move = moves[i];
        Square from = moveFrom(move);
        Square to = moveTo(move);
        std::cout << "  " << squareToString(from) << " -> " << squareToString(to) << std::endl;
    }
    
    // Now specifically test if d8 and f8 are attacked
    std::cout << "\nSquare attack tests:" << std::endl;
    std::cout << "d8 attacked by WHITE: " << (MoveGenerator::isSquareAttacked(board, D8, WHITE) ? "YES" : "NO") << std::endl;
    std::cout << "f8 attacked by WHITE: " << (MoveGenerator::isSquareAttacked(board, F8, WHITE) ? "YES" : "NO") << std::endl;
    std::cout << "d7 attacked by WHITE: " << (MoveGenerator::isSquareAttacked(board, D7, WHITE) ? "YES" : "NO") << std::endl;
    std::cout << "f7 attacked by WHITE: " << (MoveGenerator::isSquareAttacked(board, F7, WHITE) ? "YES" : "NO") << std::endl;
    
    // Test the moves manually
    std::cout << "\nManual move validation:" << std::endl;
    Move e8d8 = makeMove(E8, D8, NORMAL);
    Move e8f8 = makeMove(E8, F8, NORMAL);
    Move e8d7 = makeMove(E8, D7, NORMAL);
    Move e8f7 = makeMove(E8, F7, NORMAL);
    
    std::cout << "e8-d8 leaves king in check: " << (MoveGenerator::leavesKingInCheck(board, e8d8) ? "YES" : "NO") << std::endl;
    std::cout << "e8-f8 leaves king in check: " << (MoveGenerator::leavesKingInCheck(board, e8f8) ? "YES" : "NO") << std::endl;
    std::cout << "e8-d7 leaves king in check: " << (MoveGenerator::leavesKingInCheck(board, e8d7) ? "YES" : "NO") << std::endl;
    std::cout << "e8-f7 leaves king in check: " << (MoveGenerator::leavesKingInCheck(board, e8f7) ? "YES" : "NO") << std::endl;
}

int main() {
    testKingEvasion();
    return 0;
}