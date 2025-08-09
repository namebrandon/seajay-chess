#include <iostream>
#include <vector>
#include <utility>
#include <string>
#include "src/core/board.h"

using namespace seajay;

int main() {
    std::cout << "=== Testing specific pawn patterns ===" << std::endl;
    
    // Test different second rank pawn patterns
    std::vector<std::pair<std::string, std::string>> tests = {
        {"pppppppp", "8 pawns"},
        {"ppp1pppp", "7 pawns with gap at d7"},
        {"pp2pppp", "7 pawns with gap at c7-d7"},
        {"p3pppp", "6 pawns with larger gap"},
        {"1ppppppp", "7 pawns starting at b7"},
        {"ppppppp1", "7 pawns ending at g7"},
    };
    
    for (const auto& [pattern, desc] : tests) {
        Board board;
        std::string fen = "rnbqkbnr/" + pattern + "/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
        std::cout << "\nTest: " << desc << std::endl;
        std::cout << "Pattern: " << pattern << std::endl;
        std::cout << "Full FEN: " << fen << std::endl;
        
        auto result = board.parseFEN(fen);
        if (result.hasValue()) {
            std::cout << "  ✓ SUCCESS" << std::endl;
        } else {
            std::cout << "  ✗ FAILED: " << result.error().message << std::endl;
        }
    }
    
    // Now test with the exact problematic position components
    std::cout << "\n=== Testing exact problem position components ===" << std::endl;
    
    {
        Board board;
        // Just the piece positions, everything else default
        std::string fen = "rnbqkbnr/ppp1pppp/8/2p1p3/3P4/8/PPP1PPPP/RNBQKBNR w - - 0 1";
        std::cout << "\nWith white to move, no castling:" << std::endl;
        auto result = board.parseFEN(fen);
        std::cout << (result.hasValue() ? "  ✓ SUCCESS" : "  ✗ FAILED") << std::endl;
    }
    
    {
        Board board;
        // Add back black to move
        std::string fen = "rnbqkbnr/ppp1pppp/8/2p1p3/3P4/8/PPP1PPPP/RNBQKBNR b - - 0 1";
        std::cout << "\nWith black to move, no castling:" << std::endl;
        auto result = board.parseFEN(fen);
        std::cout << (result.hasValue() ? "  ✓ SUCCESS" : "  ✗ FAILED") << std::endl;
    }
    
    {
        Board board;
        // Add back castling rights
        std::string fen = "rnbqkbnr/ppp1pppp/8/2p1p3/3P4/8/PPP1PPPP/RNBQKBNR b KQkq - 0 1";
        std::cout << "\nWith black to move, with castling:" << std::endl;
        auto result = board.parseFEN(fen);
        std::cout << (result.hasValue() ? "  ✓ SUCCESS" : "  ✗ FAILED") << std::endl;
    }
    
    return 0;
}