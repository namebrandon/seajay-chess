#include "../../src/core/see.h"
#include "../../src/core/board.h"
#include "../../src/core/move_generation.h"
#include "../../src/core/bitboard.h"
#include <iostream>
#include <string>

using namespace seajay;

// Mock access to internal SEE methods for debugging
class SEEDebugger {
public:
    static void debugPosition(const std::string& fen, Move move) {
        Board board;
        if (!board.fromFEN(fen)) {
            std::cerr << "Failed to parse FEN\n";
            return;
        }
        
        Square from = moveFrom(move);
        Square to = moveTo(move);
        
        std::cout << "=== SEE Debug ===\n";
        std::cout << "FEN: " << fen << "\n";
        std::cout << "Move: " << squareToString(from) << squareToString(to) << "\n\n";
        
        // Get initial state
        Piece movingPiece = board.pieceAt(from);
        Color stm = colorOf(movingPiece);
        
        // Simulate initial move
        Bitboard occupied = board.occupied();
        occupied ^= squareBB(from);  // Remove moving piece
        
        std::cout << "After " << squareToString(from) << " moves to " << squareToString(to) << ":\n";
        
        // Get all attackers to e5
        Bitboard allPieces = board.pieces(WHITE) | board.pieces(BLACK);
        
        // Check what can attack e5
        std::cout << "\nChecking attackers to " << squareToString(to) << ":\n";
        
        // Check for rooks
        Bitboard rooks = board.pieces(WHITE, ROOK) | board.pieces(BLACK, ROOK);
        std::cout << "Rooks on board: ";
        Bitboard rooksCopy = rooks;
        while (rooksCopy) {
            Square s = popLsb(rooksCopy);
            std::cout << squareToString(s) << " ";
        }
        std::cout << "\n";
        
        // Check if d8 rook can attack e5
        if (board.pieceAt(D8) != NO_PIECE) {
            std::cout << "Piece at d8: " << (char)board.pieceAt(D8) << "\n";
            
            // Check if there's a clear path from d8 to e5
            std::cout << "Checking if d8 can reach e5...\n";
            
            // After white rook moves from e1 to e5, is the path clear?
            Bitboard betweenD8E5 = between(D8, E5);
            std::cout << "Squares between d8 and e5: ";
            Bitboard betweenCopy = betweenD8E5;
            while (betweenCopy) {
                Square s = popLsb(betweenCopy);
                std::cout << squareToString(s) << " ";
            }
            std::cout << "\n";
            
            // Check occupancy
            std::cout << "Occupied squares after move: ";
            Bitboard occCopy = occupied & betweenD8E5;
            if (!occCopy) {
                std::cout << "(none - path is clear!)\n";
            } else {
                while (occCopy) {
                    Square s = popLsb(occCopy);
                    std::cout << squareToString(s) << " ";
                }
                std::cout << "\n";
            }
        }
        
        // Now run actual SEE
        std::cout << "\nActual SEE value: " << see(board, move) << "\n";
    }
};

int main() {
    std::cout << "Test: Rook takes pawn with x-ray\n";
    std::cout << "Position: White Re1 takes black pawn e5, black Rd8 can recapture\n";
    std::cout << "Expected: -400 (Rook for pawn is bad when rook recaptures)\n\n";
    
    SEEDebugger::debugPosition("1k1r4/1pp4p/p7/4p3/8/P5P1/1PP4P/2K1R3 w - - 0 1", makeMove(E1, E5));
    
    return 0;
}