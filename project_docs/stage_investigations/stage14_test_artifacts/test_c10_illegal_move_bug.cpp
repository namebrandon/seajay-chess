#include <iostream>
#include <string>
#include <vector>
#include "src/core/board.h"
#include "src/core/move_generation.h"
#include "src/core/types.h"

using namespace seajay;

// Parse a UCI-style move string like "e2e4" or "b1q"
Move parseMove(const Board& board, const std::string& moveStr) {
    if (moveStr.length() < 4) return NO_MOVE;
    
    Square from = makeSquare(File(moveStr[0] - 'a'), Rank(moveStr[1] - '1'));
    Square to = makeSquare(File(moveStr[2] - 'a'), Rank(moveStr[3] - '1'));
    
    // Generate legal moves and find matching one
    MoveList legalMoves;
    MoveGenerator::generateLegalMoves(board, legalMoves);
    
    for (const Move& m : legalMoves) {
        if (::seajay::from(m) == from && ::seajay::to(m) == to) {
            // For promotions, check the promotion piece
            if (moveStr.length() == 5) {
                char promo = moveStr[4];
                // Check if this is a promotion move and matches the requested piece
                if (isPromotion(m)) {
                    PieceType promotedPiece = promotionType(m);
                    if ((promo == 'q' && promotedPiece != QUEEN) ||
                        (promo == 'r' && promotedPiece != ROOK) ||
                        (promo == 'b' && promotedPiece != BISHOP) ||
                        (promo == 'n' && promotedPiece != KNIGHT)) {
                        continue;  // Wrong promotion type
                    }
                }
            }
            return m;
        }
    }
    
    return NO_MOVE;
}

int main() {
    std::cout << "===========================================\n";
    std::cout << "Testing C10 Illegal King Move Bug\n";
    std::cout << "===========================================\n\n";
    
    // The exact move sequence from the bug report
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
    
    // Set up the starting position
    Board board;
    board.setStartingPosition();
    
    // Apply all moves
    int moveNum = 0;
    bool error = false;
    
    for (const std::string& moveStr : moves) {
        moveNum++;
        
        Move move = parseMove(board, moveStr);
        if (move == NO_MOVE) {
            std::cout << "ERROR at move " << moveNum << ": " << moveStr << " is not legal!\n";
            std::cout << "Position before failed move:\n";
            std::cout << board.toString() << "\n";
            std::cout << "FEN: " << board.toFEN() << "\n\n";
            error = true;
            break;
        }
        
        Board::UndoInfo undo;
        board.makeMove(move, undo);
        
        // Show progress for key moves
        if (moveNum == 97 || moveNum == 98 || moveNum == 108) {
            std::cout << "After move " << moveNum << " (" << moveStr << "):\n";
            std::cout << board.toString() << "\n";
            std::cout << "FEN: " << board.toFEN() << "\n\n";
        }
    }
    
    if (!error) {
        std::cout << "Successfully applied all " << moveNum << " moves!\n\n";
        std::cout << "Final position:\n";
        std::cout << board.toString() << "\n";
        std::cout << "FEN: " << board.toFEN() << "\n\n";
    }
    
    // Now test the critical position after 108 moves
    // The position should be: 7k/6p1/7p/3p1p2/8/8/3qK3/3q4 w - - 0 55
    
    std::cout << "===========================================\n";
    std::cout << "Testing Critical Position (White to Move)\n";
    std::cout << "===========================================\n\n";
    
    // Generate legal moves
    MoveList legalMoves;
    MoveGenerator::generateLegalMoves(board, legalMoves);
    
    std::cout << "Number of legal moves: " << legalMoves.size() << "\n";
    
    // Find the white king
    Square whiteKing = board.kingSquare(WHITE);
    std::cout << "White King position: " << squareToString(whiteKing) << "\n\n";
    
    // List all legal king moves
    std::cout << "Legal King moves:\n";
    int kingMoveCount = 0;
    for (const Move& move : legalMoves) {
        if (from(move) == whiteKing) {
            Square to = ::seajay::to(move);
            std::cout << "  " << squareToString(whiteKing) << squareToString(to);
            
            // Check if destination is attacked
            bool attacked = MoveGenerator::isSquareAttacked(board, to, BLACK);
            if (attacked) {
                std::cout << " (WARNING: destination attacked!)";
            }
            std::cout << "\n";
            kingMoveCount++;
        }
    }
    std::cout << "Total king moves: " << kingMoveCount << "\n\n";
    
    // Specifically check e2f2 and e2f3
    Square f2 = makeSquare(File(5), Rank(1));  // f2
    Square f3 = makeSquare(File(5), Rank(2));  // f3
    
    std::cout << "Checking problematic squares:\n";
    std::cout << "  f2 attacked by Black: " << (MoveGenerator::isSquareAttacked(board, f2, BLACK) ? "YES" : "NO") << "\n";
    std::cout << "  f3 attacked by Black: " << (MoveGenerator::isSquareAttacked(board, f3, BLACK) ? "YES" : "NO") << "\n\n";
    
    // Check if e2f2 or e2f3 are in the legal moves list
    bool foundE2F2 = false;
    bool foundE2F3 = false;
    
    for (const Move& move : legalMoves) {
        if (from(move) == whiteKing) {
            if (::seajay::to(move) == f2) {
                foundE2F2 = true;
                std::cout << "ERROR: Found illegal move e2f2 in legal moves!\n";
            }
            if (::seajay::to(move) == f3) {
                foundE2F3 = true;
                std::cout << "ERROR: Found illegal move e2f3 in legal moves!\n";
            }
        }
    }
    
    if (!foundE2F2 && !foundE2F3) {
        std::cout << "✓ GOOD: Neither e2f2 nor e2f3 are in the legal moves list\n";
    } else {
        std::cout << "\n⚠️ BUG CONFIRMED: Illegal king moves found in legal moves list!\n";
    }
    
    // Compare with Stockfish expected position
    std::cout << "\n===========================================\n";
    std::cout << "Expected Position Analysis\n";
    std::cout << "===========================================\n";
    std::cout << "After 108 moves, the position should be:\n";
    std::cout << "FEN: 7k/6p1/7p/3p1p2/8/8/3qK3/3q4 w - - 0 55\n";
    std::cout << "\nStockfish confirms:\n";
    std::cout << "  - White king on e2\n";
    std::cout << "  - Black queens on d1 and d2\n";
    std::cout << "  - e2f2 should be LEGAL (f2 not attacked)\n";
    std::cout << "  - e2f3 should be ILLEGAL (f3 attacked by queen on d2)\n";
    
    return 0;
}