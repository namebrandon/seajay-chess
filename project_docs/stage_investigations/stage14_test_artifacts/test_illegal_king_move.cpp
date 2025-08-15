#include <iostream>
#include <string>
#include <sstream>
#include "src/core/board.h"
#include "src/core/move_generation.h"
#include "src/core/types.h"

using namespace seajay;

int main() {
    Board board;
    board.setStartingPosition();
    
    // The moves leading to the illegal position
    std::string moves[] = {
        "d2d4", "g8f6", "b1c3", "e7e6", "g1f3", "h7h6", "e2e4", "f8b4", "e4e5", "f6d5",
        "c1d2", "e8g8", "c3d5", "b4d2", "e1d2", "e6d5", "d2c1", "d7d6", "c1b1", "f7f6",
        "e5d6", "c7d6", "c2c4", "c8f5", "f1d3", "f5d3", "d1d3", "d5c4", "d3c4", "g8h8",
        "c4e6", "d8b6", "d4d5", "b6f2", "h1e1", "b8d7", "e6d7", "a8c8", "d7a4", "b7b5",
        "a4b3", "a7a5", "a2a3", "f2g2", "e1g1", "g2e2", "f3d4", "e2e4", "d4c2", "a5a4",
        "b3b5", "e4c2", "b1a2", "c8b8", "a1c1", "c2h2", "b5b4", "b8b4", "a3b4", "h2d2",
        "c1d1", "d2b4", "g1e1", "f8b8", "d1b1", "b4d2", "a2a1", "d2d5", "e1d1", "d5a5",
        "d1d3", "a5e5", "d3a3", "e5d4", "a3c3", "b8b3", "b1c1", "d6d5", "c3b3", "a4b3",
        "c1e1", "d4a4", "a1b1", "a4a2", "b1c1", "a2a1", "c1d2", "a1b2", "d2e3", "b2c3",
        "e3e2", "f6f5", "e1f1", "b3b2", "f1e1", "b2b1q", "e1b1", "c2b1", "e3d2", "d5d4",
        "d2e2", "d4d3", "e2e3", "b1c2", "e3f3", "d3d2", "f3e2", "d2d1q"
    };
    
    std::cout << "Testing illegal king move bug reproduction\n";
    std::cout << "===========================================\n\n";
    
    // Apply all moves
    int moveCount = 0;
    for (const std::string& moveStr : moves) {
        // Parse move manually
        if (moveStr.length() < 4) {
            std::cout << "ERROR: Invalid move format " << moveStr << "\n";
            break;
        }
        
        Square from = makeSquare(File(moveStr[0] - 'a'), Rank(moveStr[1] - '1'));
        Square to = makeSquare(File(moveStr[2] - 'a'), Rank(moveStr[3] - '1'));
        
        // Generate legal moves and find matching one
        MoveList legalMoves;
        MoveGenerator::generateLegalMoves(board, legalMoves);
        
        Move move = NO_MOVE;
        for (const Move& m : legalMoves) {
            if (::seajay::from(m) == from && ::seajay::to(m) == to) {
                move = m;
                break;
            }
        }
        
        if (move == NO_MOVE) {
            std::cout << "ERROR: Invalid/illegal move " << moveStr << " at move " << (moveCount + 1) << "\n";
            break;
        }
        
        Board::UndoInfo undo;
        board.makeMove(move, undo);
        moveCount++;
    }
    
    std::cout << "Applied " << moveCount << " moves\n\n";
    
    // Display the position
    std::cout << "Final position:\n";
    std::cout << board.toString() << "\n";
    std::cout << "FEN: " << board.toFEN() << "\n\n";
    
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
    
    // Find checking pieces - manually since getCheckers might be private
    if (whiteInCheck) {
        std::cout << "\nChecking pieces:\n";
        for (Square sq = A1; sq <= H8; sq = Square(sq + 1)) {
            Piece piece = board.pieceAt(sq);
            if (piece != NO_PIECE && colorOf(piece) == BLACK) {
                // Check if this piece attacks the white king
                PieceType type = typeOf(piece);
                
                if (type == QUEEN) {
                    std::cout << "  - Queen at " << squareToString(sq) << "\n";
                } else if (type == ROOK) {
                    if (fileOf(sq) == fileOf(whiteKing) || rankOf(sq) == rankOf(whiteKing)) {
                        std::cout << "  - Rook at " << squareToString(sq) << " (potential check)\n";
                    }
                }
            }
        }
    }
    
    // Generate legal moves for White
    std::cout << "\nGenerating legal moves for White:\n";
    MoveList legalMoves;
    MoveGenerator::generateLegalMoves(board, legalMoves);
    
    std::cout << "Number of legal moves: " << legalMoves.size() << "\n";
    
    // Show all legal king moves
    std::cout << "\nLegal King moves:\n";
    for (const Move& move : legalMoves) {
        if (from(move) == whiteKing) {
            Square to = ::seajay::to(move);
            std::cout << "  " << squareToString(from(move)) << squareToString(to) << " ";
            
            // Check if destination is safe
            bool toAttacked = MoveGenerator::isSquareAttacked(board, to, BLACK);
            if (toAttacked) {
                std::cout << " WARNING: Destination " << squareToString(to) << " is attacked!";
            }
            std::cout << "\n";
        }
    }
    
    // Now specifically check f2 and f3
    std::cout << "\nChecking specific squares:\n";
    Square f2 = makeSquare(File(5), Rank(1));  // FILE_F = 5, RANK_2 = 1
    Square f3 = makeSquare(File(5), Rank(2));  // FILE_F = 5, RANK_3 = 2
    
    std::cout << "f2 attacked by Black: " << (MoveGenerator::isSquareAttacked(board, f2, BLACK) ? "YES" : "NO") << "\n";
    std::cout << "f3 attacked by Black: " << (MoveGenerator::isSquareAttacked(board, f3, BLACK) ? "YES" : "NO") << "\n";
    
    // Check what pieces attack f2 and f3
    std::cout << "\nPieces that could attack f2:\n";
    // Check each black piece type
    Bitboard blackQueens = board.pieces(BLACK, QUEEN);
    int queenCount = 0;
    while (blackQueens) {
        Square queen = popLsb(blackQueens);
        std::cout << "  - Queen at " << squareToString(queen);
        // Check if on same rank/file/diagonal as f2
        if (rankOf(queen) == rankOf(f2) || fileOf(queen) == fileOf(f2) ||
            std::abs(rankOf(queen) - rankOf(f2)) == std::abs(fileOf(queen) - fileOf(f2))) {
            std::cout << " (can potentially attack f2)";
        }
        std::cout << "\n";
        queenCount++;
    }
    std::cout << "Total black queens: " << queenCount << "\n";
    
    std::cout << "\nPieces that could attack f3:\n";
    blackQueens = board.pieces(BLACK, QUEEN);
    while (blackQueens) {
        Square queen = popLsb(blackQueens);
        std::cout << "  - Queen at " << squareToString(queen);
        // Check if on same rank/file/diagonal as f3
        if (rankOf(queen) == rankOf(f3) || fileOf(queen) == fileOf(f3) ||
            std::abs(rankOf(queen) - rankOf(f3)) == std::abs(fileOf(queen) - fileOf(f3))) {
            std::cout << " (can potentially attack f3)";
        }
        std::cout << "\n";
    }
    
    // Try to find e2f2 and e2f3 in legal moves
    std::cout << "\nSearching for problematic moves:\n";
    bool foundE2F2 = false;
    bool foundE2F3 = false;
    
    for (const Move& move : legalMoves) {
        if (from(move) == whiteKing && ::seajay::to(move) == f2) {
            foundE2F2 = true;
            std::cout << "ERROR: Found illegal move e2f2 in legal moves list!\n";
        }
        if (from(move) == whiteKing && ::seajay::to(move) == f3) {
            foundE2F3 = true;
            std::cout << "ERROR: Found illegal move e2f3 in legal moves list!\n";
        }
    }
    
    if (!foundE2F2 && !foundE2F3) {
        std::cout << "Good: Neither e2f2 nor e2f3 are in the legal moves list\n";
    }
    
    return 0;
}