#include <iostream>
#include "src/core/board.h"

using namespace seajay;

int main() {
    // Try a simpler position first
    Board board;
    
    // First test: starting position
    std::cout << "Test 1: Starting position FEN" << std::endl;
    bool success = board.fromFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    std::cout << "Result: " << (success ? "SUCCESS" : "FAIL") << std::endl;
    if (success) {
        std::cout << board.toString() << std::endl;
    }
    
    // Second test: after 1.e4
    std::cout << "\nTest 2: After 1.e4" << std::endl;
    Board board2;
    success = board2.fromFEN("rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq e3 0 1");
    std::cout << "Result: " << (success ? "SUCCESS" : "FAIL") << std::endl;
    if (success) {
        std::cout << board2.toString() << std::endl;
    }
    
    // Third test: the problematic position
    std::cout << "\nTest 3: Problem position" << std::endl;
    Board board3;
    auto result = board3.parseFEN("rnbqkbnr/ppp1pppp/8/2p1p3/3P4/8/PPP1PPPP/RNBQKBNR b KQkq - 0 1");
    if (result.hasValue()) {
        std::cout << "Result: SUCCESS" << std::endl;
        std::cout << board3.toString() << std::endl;
    } else {
        std::cout << "Result: FAIL - " << result.error().message << std::endl;
        
        // Try to debug what's failing
        std::cout << "\nDebug info:" << std::endl;
        std::cout << "  Piece counts valid: " << (board3.validatePieceCounts() ? "YES" : "NO") << std::endl;
        std::cout << "  Kings valid: " << (board3.validateKings() ? "YES" : "NO") << std::endl;
        std::cout << "  En passant valid: " << (board3.validateEnPassant() ? "YES" : "NO") << std::endl;
        std::cout << "  Castling rights valid: " << (board3.validateCastlingRights() ? "YES" : "NO") << std::endl;
    }
    
    return 0;
}