#include "src/core/board.h"
#include "src/core/move_generation.h"
#include "src/core/types.h"
#include <iostream>
#include <string>

using namespace seajay;

int main() {
    std::cout << "Testing Legal Move Filtering" << std::endl;
    
    Board board;
    
    // Test 1: Starting position - king cannot move into attack
    board.setStartingPosition();
    std::cout << "\nTest 1: Starting position" << std::endl;
    
    MoveList moves = generateLegalMoves(board);
    std::cout << "Legal moves from starting position: " << moves.size() << std::endl;
    
    // Should be 20 (16 pawn moves + 4 knight moves)
    if (moves.size() == 20) {
        std::cout << "✓ Starting position legal move count correct" << std::endl;
    } else {
        std::cout << "✗ Expected 20 legal moves, got " << moves.size() << std::endl;
    }
    
    // Test 2: Position where king is in check
    std::cout << "\nTest 2: King in check position" << std::endl;
    const std::string checkFEN = "rnbqkbnr/pppp1ppp/8/4p3/6P1/5P2/PPPPP2P/RNBQKBNR b KQkq g3 0 2";
    
    if (board.fromFEN(checkFEN)) {
        if (inCheck(board)) {
            std::cout << "King is in check - testing evasions" << std::endl;
            MoveList checkMoves = generateLegalMoves(board);
            std::cout << "Legal moves in check: " << checkMoves.size() << std::endl;
            
            // All moves should be legal (no move should leave king in check)
            bool allMovesLegal = true;
            for (size_t i = 0; i < checkMoves.size(); ++i) {
                Move move = checkMoves[i];
                if (MoveGenerator::leavesKingInCheck(board, move)) {
                    allMovesLegal = false;
                    std::cout << "✗ Move leaves king in check: " << move << std::endl;
                }
            }
            
            if (allMovesLegal) {
                std::cout << "✓ All generated moves are legal" << std::endl;
            }
        }
    }
    
    // Test 3: Pinned piece position
    std::cout << "\nTest 3: Pinned piece position" << std::endl;
    const std::string pinnedFEN = "rnbqk2r/pppp1ppp/5n2/2b1p3/2B1P3/3P1N2/PPP2PPP/RNBQK2R w KQkq - 4 4";
    
    if (board.fromFEN(pinnedFEN)) {
        MoveList pinnedMoves = generateLegalMoves(board);
        std::cout << "Legal moves with pinned pieces: " << pinnedMoves.size() << std::endl;
        
        // Test if pinned piece detection is working
        Square f3 = F3;
        if (MoveGenerator::isPinned(board, f3, WHITE)) {
            std::cout << "✓ Knight on f3 is correctly detected as pinned" << std::endl;
        } else {
            std::cout << "✗ Knight on f3 should be pinned" << std::endl;
        }
    }
    
    // Test 4: En passant pin test
    std::cout << "\nTest 4: En passant pin test" << std::endl;
    const std::string epPinFEN = "8/8/8/2k5/3Pp3/8/8/4K2R w - e3 0 1";
    
    if (board.fromFEN(epPinFEN)) {
        MoveList epMoves = generateLegalMoves(board);
        std::cout << "Legal moves in en passant pin position: " << epMoves.size() << std::endl;
        
        // The d4 pawn should not be able to capture en passant as it would expose the king
        bool foundIllegalEP = false;
        for (size_t i = 0; i < epMoves.size(); ++i) {
            Move move = epMoves[i];
            if (moveFrom(move) == D4 && 
                moveTo(move) == E3 && 
                isEnPassant(move)) {
                foundIllegalEP = true;
                break;
            }
        }
        
        if (!foundIllegalEP) {
            std::cout << "✓ Illegal en passant move correctly filtered out" << std::endl;
        } else {
            std::cout << "✗ Illegal en passant move was not filtered" << std::endl;
        }
    }
    
    std::cout << "\nLegal move filtering test completed." << std::endl;
    return 0;
}
