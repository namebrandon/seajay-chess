#include <iostream>
#include <iomanip>
#include "src/core/board.h"
#include "src/core/bitboard.h"

using namespace seajay;

void testFEN(const std::string& fen, const std::string& desc) {
    Board board;
    std::cout << "\n" << desc << ":" << std::endl;
    std::cout << "FEN: " << fen << std::endl;
    
    auto result = board.parseFEN(fen);
    if (result.hasValue()) {
        std::cout << "  ✓ SUCCESS" << std::endl;
        
        // Verify kings are found
        Bitboard wk = board.pieces(WHITE_KING);
        Bitboard bk = board.pieces(BLACK_KING);
        if (wk) std::cout << "    White king at: " << squareToString(lsb(wk)) << std::endl;
        if (bk) std::cout << "    Black king at: " << squareToString(lsb(bk)) << std::endl;
    } else {
        std::cout << "  ✗ FAILED: " << result.error().message << std::endl;
    }
}

int main() {
    std::cout << "=== Final hypothesis testing ===" << std::endl;
    
    // These should work
    testFEN("rnbqkbnr/ppp1pppp/8/3p4/3P4/8/PPP1PPPP/RNBQKBNR w - - 0 1", 
            "d5 d4 position (works)");
    
    testFEN("rnbqkbnr/ppp1pppp/8/2pp4/3P4/8/PPP1PPPP/RNBQKBNR w - - 0 1", 
            "c5 d5 d4 with adjacent pawns (testing)");
    
    testFEN("rnbqkbnr/ppp1pppp/8/2p1p3/3P4/8/PPP1PPPP/RNBQKBNR w - - 0 1", 
            "c5 e5 d4 THE PROBLEM (fails)");
    
    // Test if it's specific to rank 5
    testFEN("rnbqkbnr/ppp1pppp/2p1p3/8/3P4/8/PPP1PPPP/RNBQKBNR w - - 0 1", 
            "c6 e6 d4 (same pattern on rank 6)");
    
    testFEN("rnbqkbnr/ppp1pppp/8/8/2P1P3/8/PPP1PPPP/RNBQKBNR w - - 0 1", 
            "C4 E4 white pawns (same pattern)");
    
    // Try the exact same pattern "2p1p3" in different contexts
    testFEN("8/8/8/2p1p3/8/8/8/8 w - - 0 1", 
            "Just the problem pattern alone (no kings!)");
    
    testFEN("4k3/8/8/2p1p3/8/8/8/4K3 w - - 0 1", 
            "Problem pattern with kings only");
    
    // What if we manually build it differently?
    testFEN("rnbqkbnr/ppp1pppp/8/2P1P3/3p4/8/PPP1PPPP/RNBQKBNR w - - 0 1", 
            "Swapped colors for the pattern");
    
    return 0;
}