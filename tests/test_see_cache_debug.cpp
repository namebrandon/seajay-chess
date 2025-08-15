#include <iostream>
#include <iomanip>
#include "../src/core/see.h"
#include "../src/core/board.h"
#include "../src/core/move_generation.h"
#include "../src/core/magic_bitboards.h"

using namespace seajay;

int main() {
    magic::initMagics();
    
    // Disable debug output for cleaner results
    g_seeCalculator.enableDebugOutput(false);
    g_seeCalculator.clearCache();
    g_seeCalculator.resetStatistics();
    
    Board board;
    board.fromFEN("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1");
    
    std::cout << "Board zobrist key: 0x" << std::hex << board.zobristKey() << std::dec << std::endl;
    
    MoveList moves;
    MoveGenerator::generateLegalMoves(board, moves);
    
    if (moves.size() >= 2) {
        Move move1 = moves[0];
        Move move2 = moves[1];
        
        std::cout << "\nTesting move 1: " << squareToString(moveFrom(move1)) 
                  << squareToString(moveTo(move1)) << std::endl;
        
        // Call SEE multiple times on same move
        SEEValue val1 = see(board, move1);
        SEEValue val2 = see(board, move1);
        SEEValue val3 = see(board, move1);
        
        std::cout << "SEE values: " << val1 << ", " << val2 << ", " << val3 << std::endl;
        
        const auto& stats = g_seeCalculator.statistics();
        std::cout << "After 3 calls to same move:" << std::endl;
        std::cout << "  Calls: " << stats.calls << std::endl;
        std::cout << "  Cache hits: " << stats.cacheHits << std::endl;
        std::cout << "  Cache misses: " << stats.cacheMisses << std::endl;
        std::cout << "  Hit rate: " << stats.hitRate() << "%" << std::endl;
        
        // Test different move
        std::cout << "\nTesting move 2: " << squareToString(moveFrom(move2)) 
                  << squareToString(moveTo(move2)) << std::endl;
        
        SEEValue val4 = see(board, move2);
        SEEValue val5 = see(board, move2);
        
        std::cout << "SEE values: " << val4 << ", " << val5 << std::endl;
        
        std::cout << "After 2 more calls to different move:" << std::endl;
        std::cout << "  Calls: " << stats.calls << std::endl;
        std::cout << "  Cache hits: " << stats.cacheHits << std::endl;
        std::cout << "  Cache misses: " << stats.cacheMisses << std::endl;
        std::cout << "  Hit rate: " << stats.hitRate() << "%" << std::endl;
        
        // Call first move again
        SEEValue val6 = see(board, move1);
        std::cout << "\nCalling move 1 again: " << val6 << std::endl;
        std::cout << "  Calls: " << stats.calls << std::endl;
        std::cout << "  Cache hits: " << stats.cacheHits << std::endl;
        std::cout << "  Hit rate: " << stats.hitRate() << "%" << std::endl;
    }
    
    return 0;
}