/**
 * Test for Phase 3B - Replace Attack Generation
 * 
 * Verifies that perft results match exactly when using magic bitboards
 * vs ray-based attack generation.
 */

#include "../src/core/board.h"
#include "../src/core/move_generation.h"
#ifdef USE_MAGIC_BITBOARDS
#include "../src/core/magic_bitboards.h"
#endif
#include <iostream>
#include <chrono>

using namespace seajay;

// Simple perft implementation for testing
uint64_t perft(Board& board, int depth) {
    if (depth == 0) return 1;
    
    MoveList moves;
    MoveGenerator::generateLegalMoves(board, moves);
    
    if (depth == 1) return moves.size();
    
    uint64_t nodes = 0;
    for (const Move& move : moves) {
        Board::UndoInfo undo;
        board.makeMove(move, undo);
        nodes += perft(board, depth - 1);
        board.unmakeMove(move, undo);
    }
    
    return nodes;
}

struct TestPosition {
    const char* fen;
    const char* description;
    uint64_t perft4;
};

const TestPosition testPositions[] = {
    {"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", "Starting position", 197281},
    {"r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1", "Kiwipete", 4085603},
    {"8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1", "Position 3", 43238},
    {"r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1", "Position 4 (b)", 422333},
    {"rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8", "Position 5", 2103487},
    {"r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10", "Position 6", 3894594}
};

bool runPerftTest(const char* fen, uint64_t expected, int depth) {
    Board board;
    if (!board.fromFEN(fen)) {
        std::cerr << "Failed to parse FEN: " << fen << std::endl;
        return false;
    }
    
    auto start = std::chrono::high_resolution_clock::now();
    uint64_t result = perft(board, depth);
    auto elapsed = std::chrono::high_resolution_clock::now() - start;
    
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed);
    
    if (result != expected) {
        std::cerr << "MISMATCH: Expected " << expected << ", got " << result << std::endl;
        return false;
    }
    
    std::cout << "✓ " << result << " nodes in " << ms.count() << "ms" << std::endl;
    return true;
}

int main() {
    std::cout << "Phase 3B: Replace Attack Generation - Perft Validation\n";
    std::cout << "======================================================\n\n";
    
#ifdef USE_MAGIC_BITBOARDS
    std::cout << "Using: MAGIC BITBOARDS\n\n";
    
    // Initialize magic bitboards
    magic::initMagics();
    if (!magic::areMagicsInitialized()) {
        std::cerr << "ERROR: Failed to initialize magic bitboards!\n";
        return 1;
    }
#else
    std::cout << "Using: RAY-BASED ATTACKS\n\n";
#endif
    
    bool allPassed = true;
    
    // Run perft(4) tests
    std::cout << "Running perft(4) validation:\n";
    std::cout << "----------------------------\n";
    
    for (const auto& pos : testPositions) {
        std::cout << pos.description << ": ";
        if (!runPerftTest(pos.fen, pos.perft4, 4)) {
            allPassed = false;
            std::cerr << "Failed on: " << pos.description << std::endl;
        }
    }
    
    // Test perft(1-3) on starting position for quick validation
    std::cout << "\nQuick validation on starting position:\n";
    std::cout << "--------------------------------------\n";
    
    const char* startFen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
    std::cout << "Perft(1): ";
    if (!runPerftTest(startFen, 20, 1)) allPassed = false;
    
    std::cout << "Perft(2): ";
    if (!runPerftTest(startFen, 400, 2)) allPassed = false;
    
    std::cout << "Perft(3): ";
    if (!runPerftTest(startFen, 8902, 3)) allPassed = false;
    
    if (allPassed) {
        std::cout << "\n✅ Phase 3B COMPLETE: All perft tests passed with magic bitboards\n";
        std::cout << "Gate: No change in move generation\n";
    } else {
        std::cerr << "\n❌ Phase 3B FAILED: Perft results do not match\n";
        return 1;
    }
    
    return 0;
}