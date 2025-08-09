#include <iostream>
#include <vector>
#include <utility>
#include <string>
#include "src/core/board.h"

using namespace seajay;

int main() {
    std::cout << "=== Isolating the exact issue ===" << std::endl;
    
    // Build up the position piece by piece
    std::vector<std::pair<std::string, std::string>> positions = {
        // Start simple
        {"8/8/8/8/8/8/8/8 w - - 0 1", "Empty board"},
        {"rnbqkbnr/8/8/8/8/8/8/RNBQKBNR w - - 0 1", "Just back ranks"},
        {"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w - - 0 1", "Starting position"},
        
        // Now modify towards problem position
        {"rnbqkbnr/ppp1pppp/8/8/8/8/PPPPPPPP/RNBQKBNR w - - 0 1", "Black d-pawn moved"},
        {"rnbqkbnr/ppp1pppp/8/8/8/8/PPP1PPPP/RNBQKBNR w - - 0 1", "White d-pawn also moved"},
        {"rnbqkbnr/ppp1pppp/8/3p4/8/8/PPP1PPPP/RNBQKBNR w - - 0 1", "Black d5"},
        {"rnbqkbnr/ppp1pppp/8/3p4/3P4/8/PPP1PPPP/RNBQKBNR w - - 0 1", "White d4 (target without c5/e5)"},
        {"rnbqkbnr/ppp1pppp/8/2p5/3P4/8/PPP1PPPP/RNBQKBNR w - - 0 1", "Add black c5"},
        {"rnbqkbnr/ppp1pppp/8/4p3/3P4/8/PPP1PPPP/RNBQKBNR w - - 0 1", "Add black e5 instead"},
        {"rnbqkbnr/ppp1pppp/8/2p1p3/3P4/8/PPP1PPPP/RNBQKBNR w - - 0 1", "TARGET: Add both c5 and e5"},
    };
    
    for (const auto& [fen, desc] : positions) {
        Board board;
        std::cout << "\n" << desc << ":" << std::endl;
        std::cout << "FEN: " << fen << std::endl;
        
        auto result = board.parseFEN(fen);
        if (result.hasValue()) {
            std::cout << "  ✓ SUCCESS" << std::endl;
        } else {
            std::cout << "  ✗ FAILED: " << result.error().message << std::endl;
            
            // If it failed, try to understand why
            if (result.error().error == FenError::PositionValidationFailed) {
                // Test individual validators
                Board testBoard;
                testBoard.fromFEN(fen);  // Use legacy method which doesn't validate
                
                std::cout << "    Debug checks on partially parsed board:" << std::endl;
                std::cout << "      Piece counts: " << (testBoard.validatePieceCounts() ? "OK" : "FAIL") << std::endl;
                std::cout << "      Kings: " << (testBoard.validateKings() ? "OK" : "FAIL") << std::endl;
                std::cout << "      En passant: " << (testBoard.validateEnPassant() ? "OK" : "FAIL") << std::endl;
                std::cout << "      Castling: " << (testBoard.validateCastlingRights() ? "OK" : "FAIL") << std::endl;
            }
        }
    }
    
    return 0;
}