#include <iostream>
#include <iomanip>
#include <chrono>
#include "core/board.h"
#include "core/move_generation.h"

using namespace seajay;

void testIsAttackedImplementation() {
    std::cout << "\n=== Testing isAttacked() Implementation ===\n";
    
    Board board;
    board.clear();
    
    // Test pawn attacks
    board.setPiece(E4, WHITE_PAWN);
    bool d5Attacked = board.isAttacked(D5, WHITE);
    bool f5Attacked = board.isAttacked(F5, WHITE);
    bool e5Attacked = board.isAttacked(E5, WHITE);
    
    std::cout << "White pawn on e4:" << std::endl;
    std::cout << "  - Attacks d5: " << (d5Attacked ? "YES" : "NO") << " (should be YES)" << std::endl;
    std::cout << "  - Attacks f5: " << (f5Attacked ? "YES" : "NO") << " (should be YES)" << std::endl;
    std::cout << "  - Attacks e5: " << (e5Attacked ? "YES" : "NO") << " (should be NO)" << std::endl;
    
    // Test knight attacks
    board.clear();
    board.setPiece(F3, WHITE_KNIGHT);
    bool e1Attacked = board.isAttacked(E1, WHITE);
    bool h4Attacked = board.isAttacked(H4, WHITE);
    bool f4Attacked = board.isAttacked(F4, WHITE);
    
    std::cout << "Knight on f3:" << std::endl;
    std::cout << "  - Attacks e1: " << (e1Attacked ? "YES" : "NO") << " (should be YES)" << std::endl;
    std::cout << "  - Attacks h4: " << (h4Attacked ? "YES" : "NO") << " (should be YES)" << std::endl;
    std::cout << "  - Attacks f4: " << (f4Attacked ? "YES" : "NO") << " (should be NO)" << std::endl;
    
    // Test sliding piece attacks
    board.clear();
    board.setPiece(D4, WHITE_BISHOP);
    bool c3Attacked = board.isAttacked(C3, WHITE);
    bool g7Attacked = board.isAttacked(G7, WHITE);
    bool d5Attacked2 = board.isAttacked(D5, WHITE);
    
    std::cout << "Bishop on d4:" << std::endl;
    std::cout << "  - Attacks c3: " << (c3Attacked ? "YES" : "NO") << " (should be YES)" << std::endl;
    std::cout << "  - Attacks g7: " << (g7Attacked ? "YES" : "NO") << " (should be YES)" << std::endl;
    std::cout << "  - Attacks d5: " << (d5Attacked2 ? "YES" : "NO") << " (should be NO)" << std::endl;
    
    std::cout << "\n✓ isAttacked() implementation test completed!\n";
}

void testMoveGeneration(const std::string& fen, const std::string& description) {
    std::cout << "\n=== " << description << " ===" << std::endl;
    std::cout << "FEN: " << fen << std::endl;
    
    Board board;
    auto result = board.parseFEN(fen);
    
    if (result.hasError()) {
        std::cout << "Error parsing FEN: " << result.error().message << std::endl;
        return;
    }
    
    std::cout << board.toString() << std::endl;
    
    // Generate pseudo-legal moves
    auto start = std::chrono::high_resolution_clock::now();
    MoveList pseudoLegalMoves;
    MoveGenerator::generatePseudoLegalMoves(board, pseudoLegalMoves);
    auto end = std::chrono::high_resolution_clock::now();
    
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    std::cout << "Pseudo-legal moves (" << pseudoLegalMoves.size() << "): " 
              << pseudoLegalMoves.toString() << std::endl;
    std::cout << "Generation time: " << duration.count() << " microseconds" << std::endl;
    
    // Test check detection
    bool inCheck = MoveGenerator::inCheck(board);
    std::cout << "In check: " << (inCheck ? "YES" : "NO") << std::endl;
    
    // Generate legal moves (this will be basic for Stage 3)
    start = std::chrono::high_resolution_clock::now();
    MoveList legalMoves;
    MoveGenerator::generateLegalMoves(board, legalMoves);
    end = std::chrono::high_resolution_clock::now();
    
    duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    std::cout << "Legal moves (" << legalMoves.size() << "): " 
              << legalMoves.toString() << std::endl;
    std::cout << "Legal generation time: " << duration.count() << " microseconds" << std::endl;
}

int main() {
    std::cout << "SeaJay Chess Engine - Move Generation Demo" << std::endl;
    std::cout << "Phase 1 Stage 3 - Basic Move Generation" << std::endl;
    
    // First test our isAttacked() implementation
    testIsAttackedImplementation();
    
    // Test various positions
    testMoveGeneration("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
                      "Starting Position");
    
    testMoveGeneration("rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq e3 0 1",
                      "After 1.e4");
    
    testMoveGeneration("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -",
                      "Kiwipete Position (Complex)");
    
    testMoveGeneration("8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - -",
                      "Endgame Position");
    
    testMoveGeneration("r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1",
                      "Position with Promotions");
    
    testMoveGeneration("rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8",
                      "Position with Checks");
    
    // Test individual piece movement
    testMoveGeneration("8/8/8/3N4/8/8/8/8 w - -", "Single Knight");
    testMoveGeneration("8/8/8/3B4/8/8/8/8 w - -", "Single Bishop");
    testMoveGeneration("8/8/8/3R4/8/8/8/8 w - -", "Single Rook");
    testMoveGeneration("8/8/8/3Q4/8/8/8/8 w - -", "Single Queen");
    testMoveGeneration("8/8/8/3K4/8/8/8/8 w - -", "Single King");
    
    std::cout << "\n=== Move Generation Demo Complete ===" << std::endl;
    
    return 0;
}