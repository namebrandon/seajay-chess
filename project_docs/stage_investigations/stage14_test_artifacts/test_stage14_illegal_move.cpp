#include <iostream>
#include <string>
#include <vector>
#include "src/core/board.h"
#include "src/core/move_generation.h"
#include "src/core/uci_notation.h"

using namespace seajay;

int main() {
    Board board;
    board.setStartingPosition();
    
    // The exact moves from the SPRT test that led to the illegal move
    std::vector<std::string> moves = {
        "d2d4", "g8f6", "b1c3", "e7e6", "g1f3", "h7h6", "e2e4", "f8b4", "e4e5", "f6d5",
        "c1d2", "e8g8", "c3d5", "b4d2", "e1d2", "e6d5", "d2c1", "d7d6", "c1b1", "f7f6",
        "e5d6", "c7d6", "c2c4", "c8f5", "f1d3", "f5d3", "d1d3", "d5c4", "d3c4", "g8h8",
        "c4e6", "d8b6", "d4d5", "b6f2", "h1e1", "b8d7", "e6d7", "a8c8", "d7a4", "b7b5",
        "a4b3", "a7a5", "a2a3", "f2g2", "e1g1", "g2e2", "f3d4", "e2e4", "d4c2", "a5a4",
        "b3b5", "e4c2", "b1a2", "c8b8", "a1c1", "c2h2", "b5b4", "b8b4", "a3b4", "h2d2",
        "c1d1", "d2b4", "g1e1", "f8b8", "d1b1", "b4d2", "a2a1", "d2d5", "e1d1", "d5a5",
        "d1d3", "a5e5", "d3a3", "e5d4", "a3c3", "b8b3", "b1c1", "d6d5", "c3b3", "a4b3",
        "c1e1", "d4a4", "a1b1", "a4a2", "b1c1", "a2a1", "c1d2", "a1b2", "d2e3", "b2c3",
        "e3e2", "f6f5", "e1f1", "b3b2", "f1e1", "b2b1q", "e1b1", "c3b1", "e3d2", "d5d4",
        "d2e2", "d4d3", "e2e3", "b1c2", "e3f3", "d3d2", "f3e2", "d2d1q"
    };
    
    std::cout << "Testing Stage 14 Illegal Move Bug from SPRT Test\n";
    std::cout << "================================================\n\n";
    
    // Apply all moves to reach the critical position
    int moveCount = 0;
    for (const std::string& moveStr : moves) {
        Move move = uciToMove(board, moveStr);
        if (move == NO_MOVE) {
            std::cout << "ERROR: Invalid move " << moveStr << " at move " << (moveCount + 1) << "\n";
            break;
        }
        
        Board::UndoInfo undo;
        board.makeMove(move, undo);
        moveCount++;
    }
    
    std::cout << "Applied " << moveCount << " moves (expecting 98 total)\n\n";
    
    // Display the critical position
    std::cout << "Final position after " << moveCount << " moves:\n";
    std::cout << board.toPrettyString() << "\n";
    std::cout << "FEN: " << board.toFen() << "\n\n";
    
    // Check position details
    Color sideToMove = board.sideToMove();
    std::cout << "Side to move: " << (sideToMove == WHITE ? "White" : "Black") << "\n";
    
    // Find kings
    Square whiteKing = board.kingSquare(WHITE);
    Square blackKing = board.kingSquare(BLACK);
    std::cout << "White King: " << squareToString(whiteKing) << "\n";
    std::cout << "Black King: " << squareToString(blackKing) << "\n\n";
    
    // Check if white is in check
    bool whiteInCheck = MoveGenerator::inCheck(board, WHITE);
    std::cout << "White in check: " << (whiteInCheck ? "YES" : "NO") << "\n";
    
    // Get checking pieces if in check
    if (whiteInCheck) {
        std::cout << "\nChecking pieces:\n";
        Bitboard checkers = MoveGenerator::getCheckers(board, whiteKing, BLACK);
        while (checkers) {
            Square checker = popLsb(checkers);
            Piece piece = board.pieceAt(checker);
            std::cout << "  - " << pieceToChar(piece) << " at " << squareToString(checker) << "\n";
        }
    }
    
    // Generate legal moves for White
    std::cout << "\nGenerating legal moves for White:\n";
    MoveList legalMoves;
    MoveGenerator::generateLegalMoves(board, legalMoves);
    
    std::cout << "Number of legal moves: " << legalMoves.size() << "\n";
    
    // Show all legal moves
    std::cout << "\nAll legal moves:\n";
    for (const Move& move : legalMoves) {
        std::cout << "  " << moveToUci(move) << "\n";
    }
    
    // Check specifically for the problematic moves e2f2 and e2f3
    std::cout << "\n=== CHECKING FOR ILLEGAL MOVES ===\n";
    
    // Check if e2 is the king position
    if (whiteKing != makeSquare(FILE_E, RANK_2)) {
        std::cout << "ERROR: White king is not on e2 as expected!\n";
        std::cout << "White king is on: " << squareToString(whiteKing) << "\n";
    } else {
        // Check for e2f2 in legal moves
        bool hasE2F2 = false;
        bool hasE2F3 = false;
        
        for (const Move& move : legalMoves) {
            if (from(move) == whiteKing) {
                Square to = ::seajay::to(move);
                if (to == makeSquare(FILE_F, RANK_2)) {
                    hasE2F2 = true;
                    std::cout << "WARNING: Found move e2f2 in legal moves!\n";
                } else if (to == makeSquare(FILE_F, RANK_3)) {
                    hasE2F3 = true;
                    std::cout << "WARNING: Found move e2f3 in legal moves!\n";
                }
            }
        }
        
        if (!hasE2F2 && !hasE2F3) {
            std::cout << "GOOD: Neither e2f2 nor e2f3 found in legal moves\n";
        } else {
            std::cout << "\n!!! BUG CONFIRMED !!!\n";
            std::cout << "SeaJay is generating illegal king moves!\n";
            
            // Check if those squares are attacked
            Square f2 = makeSquare(FILE_F, RANK_2);
            Square f3 = makeSquare(FILE_F, RANK_3);
            
            bool f2Attacked = MoveGenerator::isSquareAttacked(board, f2, BLACK);
            bool f3Attacked = MoveGenerator::isSquareAttacked(board, f3, BLACK);
            
            std::cout << "\nSquare attack status:\n";
            std::cout << "  f2 attacked by Black: " << (f2Attacked ? "YES" : "NO") << "\n";
            std::cout << "  f3 attacked by Black: " << (f3Attacked ? "YES" : "NO") << "\n";
            
            if ((hasE2F2 && f2Attacked) || (hasE2F3 && f3Attacked)) {
                std::cout << "\nCRITICAL: King is allowed to move into check!\n";
            }
        }
    }
    
    // Also print the FEN for easy testing with Stockfish
    std::cout << "\n=== FEN for Stockfish Verification ===\n";
    std::cout << board.toFen() << "\n";
    std::cout << "\nTo verify with Stockfish:\n";
    std::cout << "echo -e \"position fen " << board.toFen() << "\\ngo perft 1\\nquit\" | ./external/engines/stockfish/stockfish\n";
    
    return 0;
}