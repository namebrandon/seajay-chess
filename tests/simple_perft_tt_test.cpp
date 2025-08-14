/**
 * Simple test to validate perft with TT
 */

#include "src/core/board.h"
#include "src/core/perft.h"
#include "src/core/transposition_table.h"
#include <iostream>
#include <cassert>

using namespace seajay;

int main() {
    std::cout << "Testing Perft with Transposition Table...\n";
    
    // Create a small TT
    TranspositionTable tt(4);  // 4MB
    
    // Test position
    Board board;
    board.fromFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    
    // Test depth 4
    std::cout << "Testing startpos depth 4...\n";
    uint64_t expected = 197281;
    
    // Run without TT
    std::cout << "  Without TT: ";
    uint64_t noTT = Perft::perft(board, 4);
    std::cout << noTT << " nodes\n";
    assert(noTT == expected);
    
    // Run with TT (cold)
    tt.clear();
    tt.resetStats();
    std::cout << "  With TT (cold): ";
    uint64_t withTT = Perft::perftWithTT(board, 4, tt);
    std::cout << withTT << " nodes\n";
    assert(withTT == expected);
    
    // Print TT stats
    auto& stats = tt.stats();
    std::cout << "  TT Stats: " << stats.hits.load() << " hits / "
              << stats.probes.load() << " probes (" 
              << stats.hitRate() << "%)\n";
    
    // Run with TT (warm)
    std::cout << "  With TT (warm): ";
    uint64_t warmTT = Perft::perftWithTT(board, 4, tt);
    std::cout << warmTT << " nodes\n";
    assert(warmTT == expected);
    
    std::cout << "  TT Stats: " << stats.hits.load() << " hits / "
              << stats.probes.load() << " probes (" 
              << stats.hitRate() << "%)\n";
    
    // Test encoding/decoding
    std::cout << "\nTesting encoding/decoding...\n";
    assert(Perft::encodeNodeCount(100) == 100);
    assert(Perft::decodeNodeCount(100) == 100);
    assert(Perft::encodeNodeCount(32767) == 32767);
    assert(Perft::decodeNodeCount(32767) == 32767);
    assert(Perft::encodeNodeCount(1000000) == -1);  // Too large
    
    std::cout << "\n✓ All tests passed!\n";
    
    // Test collision rate
    std::cout << "\nCollision rate: " << 
              (100.0 * stats.collisions.load() / stats.stores.load()) << "%\n";
    
    return 0;
}