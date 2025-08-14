#pragma once

#include "board.h"
#include "transposition_table.h"
#include <cstdint>
#include <string>
#include <map>

namespace seajay {

/**
 * Perft (Performance Test) Module
 * 
 * Stage 12: Transposition Table Integration
 * 
 * Provides move generation validation through exhaustive node counting.
 * Now includes TT caching for dramatic speedup on repeated calculations.
 */
class Perft {
public:
    struct Result {
        uint64_t nodes = 0;
        double timeSeconds = 0.0;
        
        double nps() const {
            return timeSeconds > 0 ? nodes / timeSeconds : 0;
        }
    };
    
    struct DivideResult {
        std::map<std::string, uint64_t> moveNodes;
        uint64_t totalNodes = 0;
    };
    
    // Basic perft without TT
    static uint64_t perft(Board& board, int depth);
    
    // Perft with TT caching
    static uint64_t perftWithTT(Board& board, int depth, TranspositionTable& tt);
    
    // Perft divide for debugging (shows nodes per root move)
    static DivideResult perftDivide(Board& board, int depth);
    static DivideResult perftDivideWithTT(Board& board, int depth, TranspositionTable& tt);
    
    // Run perft with timing
    static Result runPerft(Board& board, int depth, bool useTT = false, 
                           TranspositionTable* tt = nullptr);
    
    // Standard test positions
    struct TestPosition {
        std::string fen;
        std::string description;
        std::map<int, uint64_t> expectedNodes;  // depth -> node count
    };
    
    static const std::vector<TestPosition>& getStandardPositions();
    
    // Validation helpers
    static bool validatePosition(const std::string& fen, int depth, 
                                 uint64_t expectedNodes, bool useTT = false,
                                 TranspositionTable* tt = nullptr);
    
    static bool runStandardTests(int maxDepth = 5, bool useTT = false,
                                 TranspositionTable* tt = nullptr);
    
    // Helper to encode perft result in TT score field (public for testing)
    static int16_t encodeNodeCount(uint64_t nodes);
    static uint64_t decodeNodeCount(int16_t score);

private:
    // Internal perft implementation for TT version
    static uint64_t perftTTInternal(Board& board, int depth, TranspositionTable& tt);
};

} // namespace seajay