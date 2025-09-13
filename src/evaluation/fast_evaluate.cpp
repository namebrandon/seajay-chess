#include "fast_evaluate.h"
#include "evaluate.h"

namespace seajay {

namespace eval {

#ifndef NDEBUG
thread_local FastEvalStats g_fastEvalStats;
#endif

Score fastEvaluate(const Board& board) {
#ifndef NDEBUG
    g_fastEvalStats.fastEvalCalls++;
#endif
    
    // Phase 3B: Material-only evaluation
    // Optional early-out for insufficient material (cheap check)
    if (board.isInsufficientMaterial()) {
        return Score::draw();
    }
    
    // Return material balance from side-to-move perspective
    return board.material().balance(board.sideToMove());
}

} // namespace eval

} // namespace seajay