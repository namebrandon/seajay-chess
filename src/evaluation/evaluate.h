#pragma once

#include "types.h"

namespace seajay {
class Board;
}

namespace seajay::eval {

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