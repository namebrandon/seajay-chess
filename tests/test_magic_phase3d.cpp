/**
 * Test for Phase 3D - Edge Case Testing with Magic Bitboards
 * 
 * Tests critical edge cases identified in stage10_magic_validation_harness.md
 * to ensure magic bitboards handle all special positions correctly.
 */

#include "../src/core/board.h"
#include "../src/core/move_generation.h"
#ifdef USE_MAGIC_BITBOARDS
#include "../src/core/magic_bitboards.h"
#endif
#include <iostream>
#include <iomanip>
#include <vector>
#include <string>

using namespace seajay;

struct EdgeCaseTest {
    const char* name;
    const char* fen;
    const char* description;
    int depth;
    uint64_t expected;
};

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

// Test specific attack generation for a position
bool testAttackGeneration(const char* fen, Square sq, Bitboard expectedRook, Bitboard expectedBishop) {
    Board board;
    if (!board.fromFEN(fen)) {
        std::cerr << "Failed to parse FEN\n";
        return false;
    }
    
    Bitboard occupied = board.occupied();
    
    // Test rook attacks if expected
    if (expectedRook != 0) {
        Bitboard rookAttacks = magicRookAttacks(sq, occupied);
        Bitboard rayRookAttacks = ::seajay::rookAttacks(sq, occupied);
        
        if (rookAttacks != rayRookAttacks) {
            std::cerr << "Rook attacks mismatch at " << sq << "\n";
            std::cerr << "Magic: 0x" << std::hex << rookAttacks << "\n";
            std::cerr << "Ray:   0x" << std::hex << rayRookAttacks << "\n";
            return false;
        }
        
        if (rookAttacks != expectedRook) {
            std::cerr << "Rook attacks don't match expected at " << sq << "\n";
            std::cerr << "Got:      0x" << std::hex << rookAttacks << "\n";
            std::cerr << "Expected: 0x" << std::hex << expectedRook << "\n";
            return false;
        }
    }
    
    // Test bishop attacks if expected
    if (expectedBishop != 0) {
        Bitboard bishopAttacks = magicBishopAttacks(sq, occupied);
        Bitboard rayBishopAttacks = ::seajay::bishopAttacks(sq, occupied);
        
        if (bishopAttacks != rayBishopAttacks) {
            std::cerr << "Bishop attacks mismatch at " << sq << "\n";
            std::cerr << "Magic: 0x" << std::hex << bishopAttacks << "\n";
            std::cerr << "Ray:   0x" << std::hex << rayBishopAttacks << "\n";
            return false;
        }
        
        if (bishopAttacks != expectedBishop) {
            std::cerr << "Bishop attacks don't match expected at " << sq << "\n";
            std::cerr << "Got:      0x" << std::hex << bishopAttacks << "\n";
            std::cerr << "Expected: 0x" << std::hex << expectedBishop << "\n";
            return false;
        }
    }
    
    return true;
}

int main() {
    std::cout << "Phase 3D: Edge Case Testing with Magic Bitboards\n";
    std::cout << "================================================\n\n";
    
#ifdef USE_MAGIC_BITBOARDS
    std::cout << "Using: MAGIC BITBOARDS\n\n";
    magic::initMagics();
    if (!magic::areMagicsInitialized()) {
        std::cerr << "ERROR: Failed to initialize magic bitboards!\n";
        return 1;
    }
#else
    std::cout << "ERROR: Must be compiled with USE_MAGIC_BITBOARDS\n";
    return 1;
#endif
    
    bool allPassed = true;
    
    // Critical edge cases from stage10_magic_validation_harness.md
    std::cout << "=== Critical Edge Case Tests ===\n\n";
    
    // Test 1: En Passant Phantom Blocker Bug
    std::cout << "Test 1: En Passant Phantom Blocker\n";
    std::cout << "Position where en passant capture might affect sliding attacks\n";
    {
        const char* fen = "8/8/8/2pPp3/8/8/8/R3K2R w KQ c6 0 1";
        Board board;
        board.fromFEN(fen);
        
        // Check that rook attacks from A1 are not affected by phantom blocker
        Bitboard occupied = board.occupied();
        Bitboard rookA1 = magicRookAttacks(A1, occupied);
        
        // Should see all of rank 1 except H1
        Bitboard expectedA1 = 0xFE; // Rank 1 minus A1
        
        if ((rookA1 & 0xFF) == expectedA1) {
            std::cout << "✅ A1 rook attacks correct (no phantom blocker)\n";
        } else {
            std::cout << "❌ A1 rook attacks incorrect\n";
            std::cout << "   Got:      0x" << std::hex << (rookA1 & 0xFF) << std::dec << "\n";
            std::cout << "   Expected: 0x" << std::hex << expectedA1 << std::dec << "\n";
            allPassed = false;
        }
    }
    std::cout << "\n";
    
    // Test 2: Promotion with Discovery Check
    std::cout << "Test 2: Promotion with Discovery Check\n";
    std::cout << "Pawn promotion that creates discovered check\n";
    {
        const char* fen = "r3k3/P7/8/8/8/8/8/R3K3 w Q - 0 1";
        Board board;
        board.fromFEN(fen);
        
        uint64_t result = perft(board, 2);
        uint64_t expected = 122;  // Verified with Stockfish
        
        if (result == expected) {
            std::cout << "✅ Perft(2) = " << result << " (correct)\n";
        } else {
            std::cout << "❌ Perft(2) = " << result << " (expected " << expected << ")\n";
            allPassed = false;
        }
    }
    std::cout << "\n";
    
    // Test 3: Symmetric Castling Position
    std::cout << "Test 3: Symmetric Castling Position\n";
    std::cout << "Both sides can castle, attacks must be symmetric\n";
    {
        const char* fen = "r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1";
        Board board;
        board.fromFEN(fen);
        
        // White rook on A1
        Bitboard whiteRookA1 = magicRookAttacks(A1, board.occupied());
        // Black rook on A8
        Bitboard blackRookA8 = magicRookAttacks(A8, board.occupied());
        
        // Should be symmetric (shifted by 56 squares)
        bool symmetric = (whiteRookA1 << 56) == (blackRookA8 & 0xFF00000000000000ULL);
        
        if (symmetric) {
            std::cout << "✅ Rook attacks are symmetric\n";
        } else {
            std::cout << "❌ Rook attacks are not symmetric\n";
            allPassed = false;
        }
        
        // Test perft to ensure castling works
        uint64_t result = perft(board, 3);
        uint64_t expected = 13744;  // Verified with Stockfish
        
        if (result == expected) {
            std::cout << "✅ Perft(3) = " << result << " (castling works)\n";
        } else {
            std::cout << "❌ Perft(3) = " << result << " (expected " << expected << ")\n";
            allPassed = false;
        }
    }
    std::cout << "\n";
    
    // Test 4: Maximum Blocker Density
    std::cout << "Test 4: Maximum Blocker Density\n";
    std::cout << "Board nearly full, complex blocking patterns\n";
    {
        const char* fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
        Board board;
        board.fromFEN(fen);
        
        // Test attacks from various squares with high density
        bool attacksCorrect = true;
        
        // Rook on A1 can see A2 (pawn) and B1 (knight)
        Bitboard rookA1 = magicRookAttacks(A1, board.occupied());
        Bitboard expectedA1 = squareBB(A2) | squareBB(B1);  // 0x102
        if (rookA1 != expectedA1) {
            std::cout << "❌ Rook A1 attacks incorrect in dense position\n";
            std::cout << "   Got:      0x" << std::hex << rookA1 << std::dec << "\n";
            std::cout << "   Expected: 0x" << std::hex << expectedA1 << std::dec << "\n";
            attacksCorrect = false;
        }
        
        // Bishop on C1 can see B2 and D2 (pawns)
        Bitboard bishopC1 = magicBishopAttacks(C1, board.occupied());
        Bitboard expectedC1 = squareBB(B2) | squareBB(D2);  // 0xa00
        if (bishopC1 != expectedC1) {
            std::cout << "❌ Bishop C1 attacks incorrect\n";
            std::cout << "   Got:      0x" << std::hex << bishopC1 << std::dec << "\n";
            std::cout << "   Expected: 0x" << std::hex << expectedC1 << std::dec << "\n";
            attacksCorrect = false;
        }
        
        if (attacksCorrect) {
            std::cout << "✅ Attacks correct in maximum density\n";
        } else {
            allPassed = false;
        }
    }
    std::cout << "\n";
    
    // Test 5: Corner and Edge Cases
    std::cout << "Test 5: Corner and Edge Cases\n";
    std::cout << "Pieces in corners with specific blockers\n";
    {
        // Corner rook with strategic blockers
        const char* fen = "7R/8/8/8/3p4/8/8/p7 w - - 0 1";
        Board board;
        board.fromFEN(fen);
        
        // Rook on H8
        Bitboard rookH8 = magicRookAttacks(H8, board.occupied());
        Bitboard expected = 0x7F80808080808080ULL;  // File H (except H8) and rank 8 (except H8)
        
        if (rookH8 == expected) {
            std::cout << "✅ Corner rook H8 attacks correct\n";
        } else {
            std::cout << "❌ Corner rook H8 attacks incorrect\n";
            std::cout << "   Got:      0x" << std::hex << rookH8 << std::dec << "\n";
            std::cout << "   Expected: 0x" << std::hex << expected << std::dec << "\n";
            allPassed = false;
        }
        
        // Test bishop in corner
        const char* fen2 = "B7/8/8/8/3p4/8/8/7b w - - 0 1";
        board.fromFEN(fen2);
        
        // Bishop on A8
        Bitboard bishopA8 = magicBishopAttacks(A8, board.occupied());
        Bitboard expectedBishop = 0x2040810204080ULL;  // Diagonal from A8 to H1 (stops at D4)
        
        if (bishopA8 == expectedBishop) {
            std::cout << "✅ Corner bishop A8 attacks correct\n";
        } else {
            std::cout << "❌ Corner bishop A8 attacks incorrect\n";
            allPassed = false;
        }
    }
    std::cout << "\n";
    
    // Test 6: Sliding Piece Chains
    std::cout << "Test 6: Sliding Piece Chains\n";
    std::cout << "Multiple sliding pieces on same ray\n";
    {
        const char* fen = "8/8/8/8/R2r4/8/8/8 w - - 0 1";
        Board board;
        board.fromFEN(fen);
        
        // White rook on A4 can see file A (all squares except A4) and rank 4 up to D4
        Bitboard rookA4 = magicRookAttacks(A4, board.occupied());
        Bitboard expectedA4 = 0x1010101FE010101ULL;  // File A except A4, rank 4 up to D4
        
        if (rookA4 == expectedA4) {
            std::cout << "✅ Rook chain attacks correct\n";
        } else {
            std::cout << "❌ Rook chain attacks incorrect\n";
            allPassed = false;
        }
    }
    std::cout << "\n";
    
    // Performance comparison
    std::cout << "=== Performance Comparison ===\n";
    {
        const char* fen = "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1";
        Board board;
        board.fromFEN(fen);
        Bitboard occupied = board.occupied();
        
        const int iterations = 10000000;
        
        // Benchmark magic bitboards
        auto start = std::chrono::high_resolution_clock::now();
        for (int i = 0; i < iterations; i++) {
            Square sq = static_cast<Square>(i & 63);
            volatile auto r = magicRookAttacks(sq, occupied);
            (void)r;
        }
        auto magicTime = std::chrono::high_resolution_clock::now() - start;
        
        // Benchmark ray-based
        start = std::chrono::high_resolution_clock::now();
        for (int i = 0; i < iterations; i++) {
            Square sq = static_cast<Square>(i & 63);
            volatile auto r = ::seajay::rookAttacks(sq, occupied);
            (void)r;
        }
        auto rayTime = std::chrono::high_resolution_clock::now() - start;
        
        auto magicNs = std::chrono::duration_cast<std::chrono::nanoseconds>(magicTime);
        auto rayNs = std::chrono::duration_cast<std::chrono::nanoseconds>(rayTime);
        
        std::cout << "Magic bitboards: " << magicNs.count() / iterations << " ns/call\n";
        std::cout << "Ray-based:       " << rayNs.count() / iterations << " ns/call\n";
        std::cout << "Speedup:         " << std::fixed << std::setprecision(1) 
                  << (double)rayNs.count() / magicNs.count() << "x\n";
    }
    
    std::cout << "\n" << std::string(50, '=') << "\n";
    if (allPassed) {
        std::cout << "✅ Phase 3D COMPLETE: All edge cases passed\n";
        std::cout << "Gate: No edge case failures with magic bitboards\n";
        return 0;
    } else {
        std::cout << "❌ Phase 3D FAILED: Some edge cases failed\n";
        return 1;
    }
}