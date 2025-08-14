/**
 * Simplified Perft Zobrist Validation
 * Quick check that zobrist hashes are maintained correctly through perft
 */

#include <iostream>
#include "../src/core/board.h"
#include "../src/core/move_generation.h"
#include "../src/core/move_list.h"

using namespace seajay;

// Simple perft with hash verification
uint64_t perftWithHashCheck(Board& board, int depth, int& errors) {
    if (depth == 0) return 1;
    
    MoveList moves;
    MoveGenerator::generateLegalMoves(board, moves);
    
    if (depth == 1) return moves.size();
    
    uint64_t nodes = 0;
    for (size_t i = 0; i < moves.size(); ++i) {
        Move move = moves[i];
        Board::UndoInfo undo;
        
        // Save hash before move
        uint64_t hashBefore = board.zobristKey();
        
        board.makeMove(move, undo);
        nodes += perftWithHashCheck(board, depth - 1, errors);
        board.unmakeMove(move, undo);
        
        // Verify hash is restored
        uint64_t hashAfter = board.zobristKey();
        if (hashBefore != hashAfter) {
            std::cerr << "Hash mismatch after unmake!\n";
            errors++;
        }
    }
    
    return nodes;
}

int main() {
    std::cout << "Quick Perft Zobrist Validation\n";
    std::cout << "==============================\n\n";
    
    // Test a few positions at shallow depth
    const char* positions[] = {
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
        "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1",
        "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1"
    };
    
    const char* names[] = {
        "Starting position",
        "Kiwipete",
        "Position 3"
    };
    
    for (int i = 0; i < 3; i++) {
        Board board;
        board.parseFEN(positions[i]);
        
        std::cout << names[i] << ":\n";
        
        // Validate current hash
        uint64_t currentHash = board.zobristKey();
        board.rebuildZobristKey();
        uint64_t rebuiltHash = board.zobristKey();
        
        if (currentHash == rebuiltHash) {
            std::cout << "  Initial hash valid: 0x" << std::hex << currentHash << std::dec << "\n";
        } else {
            std::cout << "  Initial hash INVALID!\n";
            std::cout << "  Current:  0x" << std::hex << currentHash << "\n";
            std::cout << "  Rebuilt:  0x" << rebuiltHash << std::dec << "\n";
        }
        
        // Run perft at depth 3
        int errors = 0;
        uint64_t nodes = perftWithHashCheck(board, 3, errors);
        
        std::cout << "  Perft(3): " << nodes << " nodes\n";
        if (errors > 0) {
            std::cout << "  ERRORS: " << errors << " hash mismatches\n";
        } else {
            std::cout << "  ✓ All hashes maintained correctly\n";
        }
        std::cout << "\n";
    }
    
    std::cout << "=== Quick Validation Complete ===\n";
    return 0;
}