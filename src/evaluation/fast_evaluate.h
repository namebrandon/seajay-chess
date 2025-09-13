#pragma once

#include "../core/board.h"
#include "types.h"

namespace seajay {

namespace eval {

// Fast evaluation function for qsearch and pruning decisions
// Phase 3A: Currently just calls full evaluate() - no behavior change
Score fastEvaluate(const Board& board);

// Debug counters (compiled out in Release builds)
#ifndef NDEBUG
struct FastEvalStats {
    uint64_t fastEvalCalls = 0;
    uint64_t fastEvalUsedInStandPat = 0;
    uint64_t fastEvalUsedInPruning = 0;
    
    void reset() {
        fastEvalCalls = 0;
        fastEvalUsedInStandPat = 0;
        fastEvalUsedInPruning = 0;
    }
};

// Thread-local stats to avoid contention
extern thread_local FastEvalStats g_fastEvalStats;
#endif

} // namespace eval

} // namespace seajay