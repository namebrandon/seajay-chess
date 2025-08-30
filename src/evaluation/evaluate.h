#pragma once

#include "types.h"

namespace seajay {
class Board;
}

namespace seajay::eval {

// Phase 2.5.a: Lazy evaluation configuration
struct LazyEvalConfig {
    bool enabled = false;           // Master switch for lazy eval
    int threshold = 700;            // Material advantage threshold in centipawns
    bool staged = true;              // Use staged approach vs simple binary
    bool phaseAdjust = true;         // Adjust thresholds by game phase
    
    // Thread-safe singleton accessor
    static LazyEvalConfig& getInstance() {
        static LazyEvalConfig instance;
        return instance;
    }
    
private:
    LazyEvalConfig() = default;
};

Score evaluate(const Board& board);

// Phase 3: Detailed evaluation breakdown for UCI eval command
struct EvalBreakdown {
    Score material;
    Score pst;
    Score passedPawns;
    Score isolatedPawns;
    Score doubledPawns;
    Score backwardPawns;
    Score pawnIslands;
    Score bishopPair;
    Score mobility;
    Score kingSafety;
    Score rookFiles;
    Score knightOutposts;
    Score total;
};

EvalBreakdown evaluateDetailed(const Board& board);

#ifdef DEBUG
bool verifyMaterialIncremental(const Board& board);
#endif

} // namespace seajay::eval