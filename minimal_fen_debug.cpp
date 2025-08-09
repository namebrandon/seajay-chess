#include <iostream>
#include "src/core/board.h"

using namespace seajay;

int main() {
    std::cout << "=== Testing FEN parsing issue ===" << std::endl;
    
    // Test 1: Simple position with kings only
    {
        Board board;
        std::string fen = "4k3/8/8/8/8/8/8/4K3 w - - 0 1";
        std::cout << "\nTest 1: Kings only position" << std::endl;
        std::cout << "FEN: " << fen << std::endl;
        
        auto result = board.parseFEN(fen);
        if (result.hasValue()) {
            std::cout << "SUCCESS" << std::endl;
            std::cout << board.toString() << std::endl;
        } else {
            std::cout << "FAILED: " << result.error().message << std::endl;
        }
    }
    
    // Test 2: Starting position
    {
        Board board;
        std::string fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
        std::cout << "\nTest 2: Starting position" << std::endl;
        std::cout << "FEN: " << fen << std::endl;
        
        auto result = board.parseFEN(fen);
        if (result.hasValue()) {
            std::cout << "SUCCESS" << std::endl;
            // Don't print whole board to save space
        } else {
            std::cout << "FAILED: " << result.error().message << std::endl;
        }
    }
    
    // Test 3: The problematic position
    {
        Board board;
        std::string fen = "rnbqkbnr/ppp1pppp/8/2p1p3/3P4/8/PPP1PPPP/RNBQKBNR b KQkq - 0 1";
        std::cout << "\nTest 3: Problematic position" << std::endl;
        std::cout << "FEN: " << fen << std::endl;
        
        auto result = board.parseFEN(fen);
        if (result.hasValue()) {
            std::cout << "SUCCESS" << std::endl;
        } else {
            std::cout << "FAILED: " << result.error().message << std::endl;
            
            // Let's try to understand which part is failing
            // Try just the board position without other fields
            Board testBoard;
            std::string simplefen = "rnbqkbnr/ppp1pppp/8/2p1p3/3P4/8/PPP1PPPP/RNBQKBNR w - - 0 1";
            std::cout << "\nTrying simpler version (white to move, no castling):" << std::endl;
            auto result2 = testBoard.parseFEN(simplefen);
            if (result2.hasValue()) {
                std::cout << "  This works!" << std::endl;
            } else {
                std::cout << "  Still fails: " << result2.error().message << std::endl;
            }
        }
    }
    
    // Test 4: Check if it's the "ppp1pppp" that's the issue
    {
        Board board;
        std::string fen = "rnbqkbnr/ppp2ppp/8/4p3/4P3/8/PPP2PPP/RNBQKBNR w KQkq - 0 1";
        std::cout << "\nTest 4: Position after 1.e4 e5 (simpler pawn structure)" << std::endl;
        std::cout << "FEN: " << fen << std::endl;
        
        auto result = board.parseFEN(fen);
        if (result.hasValue()) {
            std::cout << "SUCCESS" << std::endl;
        } else {
            std::cout << "FAILED: " << result.error().message << std::endl;
        }
    }
    
    return 0;
}