#pragma once

#include "../core/board.h"
#include "types.h"
#include <cstdint>
#include <algorithm>

namespace seajay {

namespace eval {

// Fast evaluation function for qsearch and pruning decisions
// Phase 3A: Currently just calls full evaluate() - no behavior change
Score fastEvaluate(const Board& board);

// Phase 3C.0: Fast material + PST evaluation for shadow parity checking
// Returns material balance + PST score with phase interpolation
// O(1) operation using board's incremental values
Score fastEvaluateMatPST(const Board& board);

// Debug counters (compiled out in Release builds)
#ifndef NDEBUG

// Histogram for tracking fast vs reference eval differences
struct alignas(64) ParityHistogram {
    static constexpr int BUCKET_SIZE = 8;  // 8 centipawn buckets
    static constexpr int NUM_BUCKETS = 17; // -64 to +64 in 8cp buckets
    
    uint64_t buckets[NUM_BUCKETS] = {};
    uint64_t totalSamples = 0;
    uint64_t nonZeroDiffCount = 0;
    int32_t maxAbsDiff = 0;
    
    void record(int32_t diff) {
        totalSamples++;
        if (diff != 0) {
            nonZeroDiffCount++;
            maxAbsDiff = std::max(maxAbsDiff, std::abs(diff));
        }
        
        // Map diff to bucket index (-64 to +64 -> 0 to 16)
        int bucketIdx = std::clamp((diff + 64) / BUCKET_SIZE, 0, NUM_BUCKETS - 1);
        buckets[bucketIdx]++;
    }
    
    void reset() {
        for (int i = 0; i < NUM_BUCKETS; i++) {
            buckets[i] = 0;
        }
        totalSamples = 0;
        nonZeroDiffCount = 0;
        maxAbsDiff = 0;
    }
};

struct alignas(64) FastEvalStats {
    uint64_t fastEvalCalls = 0;
    uint64_t fastEvalUsedInStandPat = 0;
    uint64_t fastEvalUsedInPruning = 0;
    
    // Phase 3E.1: Counter for fast eval used in futility at depth 1
    uint64_t fastFutilityDepth1Used = 0;
    
    // Phase 3C.0: Parity checking stats
    ParityHistogram parityHist;
    
    // Phase 3E.0: Shadow audit counters for pruning decisions
    // Track where fast_eval would flip pruning decisions vs full eval
    struct PruningAudit {
        // Reverse futility (static null move) by depth [1..8]
        uint64_t staticNullAttempts[9] = {};  // Index 0 unused
        uint64_t staticNullWouldFlip[9] = {};
        
        // Razoring by depth [1..2]
        uint64_t razorAttempts[3] = {};  // Index 0 unused
        uint64_t razorWouldFlip[3] = {};
        
        // Futility pruning by effective depth [1..6]
        uint64_t futilityAttempts[7] = {};  // Index 0 unused
        uint64_t futilityWouldFlip[7] = {};
        
        // Phase 3F.0: Null-move static margin check by depth [1..12]
        uint64_t nullMoveStaticAttempts[13] = {};  // Index 0 unused
        uint64_t nullMoveStaticWouldFlip[13] = {};
        
        void reset() {
            for (int i = 0; i < 9; i++) {
                staticNullAttempts[i] = 0;
                staticNullWouldFlip[i] = 0;
            }
            for (int i = 0; i < 3; i++) {
                razorAttempts[i] = 0;
                razorWouldFlip[i] = 0;
            }
            for (int i = 0; i < 7; i++) {
                futilityAttempts[i] = 0;
                futilityWouldFlip[i] = 0;
            }
            for (int i = 0; i < 13; i++) {
                nullMoveStaticAttempts[i] = 0;
                nullMoveStaticWouldFlip[i] = 0;
            }
        }
    } pruningAudit;
    
    void reset() {
        fastEvalCalls = 0;
        fastEvalUsedInStandPat = 0;
        fastEvalUsedInPruning = 0;
        fastFutilityDepth1Used = 0;
        parityHist.reset();
        pruningAudit.reset();
    }
};

// Thread-local stats to avoid contention
extern thread_local FastEvalStats g_fastEvalStats;
#endif

} // namespace eval

} // namespace seajay