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
    
    // Phase 3A: Simply call full evaluate() for now
    // Future phases will implement material + PST + pawn cache
    return evaluate(board);
}

} // namespace eval

} // namespace seajay