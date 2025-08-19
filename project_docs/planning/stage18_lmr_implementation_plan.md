# SeaJay Chess Engine - Stage 18: Late Move Reductions (LMR) Implementation Plan

**Document Version:** 1.0  
**Date:** August 19, 2025  
**Stage:** Phase 3, Stage 18 (renumbered from Stage 21) - Late Move Reductions  
**Prerequisites Completed:** Yes - All Phase 1-2 stages, including SEE (Stage 15)  

## Executive Summary

Implement Late Move Reductions (LMR) to reduce search depth for moves late in move ordering, enabling significantly deeper searches. LMR is based on the observation that moves late in the move ordering are unlikely to be best, so we can search them to reduced depth initially. If they prove surprisingly good (beat alpha), we re-search at full depth. Expected gain: +50-100 ELO.

## Current State Analysis

### What Exists
- **Engine Strength:** ~2200 ELO (Stage 15 complete)
- **Search:** Negamax with alpha-beta pruning, iterative deepening
- **Move Ordering:** MVV-LVA for captures, quiet moves unordered
- **Search Depth:** Typically 6-8 plies
- **Performance:** ~1M NPS
- **Quiescence:** Active with conservative delta pruning (900cp)
- **Transposition Tables:** 128MB, always-replace strategy
- **SEE:** Implemented but disabled by default (overhead exceeds benefit)

### Branch Status
- Currently on: `feature/20250819-lmr`
- Base: main branch with Stage 15 complete

## Deferred Items Being Addressed

From `/workspace/project_docs/tracking/deferred_items_tracker.md`:
- Late Move Reductions was listed as a Phase 3 target (line 319)
- No specific prerequisites were blocking LMR implementation
- History heuristic was mentioned but not required for basic LMR

## Implementation Plan

### ⚠️ CRITICAL: Implementation Phase Structure

**MANDATORY REQUIREMENTS:**
1. **Phases must be GRANULAR** - Maximum 2-3 small changes per phase
2. **Each phase gets its own commit** with "bench <node-count>" in message
3. **OpenBench testing after EACH phase** - Human must perform tests
4. **Bug fixes BEFORE proceeding** - Never build on broken foundations
5. **Branch strategy per** `/workspace/project_docs/Git_Strategy_for_SeaJay.txt`

### Phase 1: LMR Data Structures and UCI Options (AWAITING OPENBENCH TEST)
**Status:** ⚠️ IMPLEMENTED - AWAITING OPENBENCH TEST
**Files modified:**
- `src/search/types.h` - Added LMRParams and LMRStats to SearchData
- `src/uci/uci.cpp` - Added 5 UCI options
- `src/uci/uci.h` - Added member variables
- `src/search/negamax.cpp` - Initialize params from limits

**Changes:**
1. Added LMRParams struct with enabled, minDepth, minMoveNumber, baseReduction, depthFactor
2. Added LMRStats struct for tracking reductions and re-searches
3. Added UCI options: LMREnabled, LMRMinDepth, LMRMinMoveNumber, LMRBaseReduction, LMRDepthFactor
4. Parse options and pass to search

**Testing:** Build and verify UCI options appear and parse correctly
**🛑 STOP POINT:** Human MUST run OpenBench test before proceeding to Phase 2

### Phase 2: Basic Reduction Formula (30 minutes)
**Files to create/modify:**
- `src/search/lmr.h` (new file) - ⚠️ PARTIALLY CREATED (declarations only)
- `src/search/lmr.cpp` (new file) - NOT STARTED
- `tests/test_lmr.cpp` (new file) - NOT STARTED

**Changes:**
1. Create getLMRReduction() function with conservative linear formula
2. Add unit tests for reduction calculation
3. Verify edge cases (don't reduce below depth 1, cap at depth-2)

**Formula:**
```cpp
reduction = baseReduction + (depth - minDepth) / depthFactor
// Additional reduction for very late moves (>8)
if (moveNumber > 8) reduction += (moveNumber - 8) / 4;
// Cap at depth-2 to leave at least 1 ply
return std::min(reduction, depth - 2);
```

**Commit:** "Phase 2: Add LMR reduction formula with unit tests - bench [count]"
**🛑 STOP POINT:** Human MUST run OpenBench test before proceeding to Phase 3 (even though no functional change)

### Phase 3: Integrate LMR into Negamax (1 hour)
**Files to modify:**
- `src/search/negamax.cpp` - Add reduction logic in move loop
- `CMakeLists.txt` - Add new LMR files

**Changes:**
1. Calculate reduction for each move based on conditions
2. Implement reduced search with null window
3. Add re-search logic when score > alpha

**Integration point (around line 361):**
```cpp
// Calculate LMR reduction
int reduction = 0;
if (depth >= info.lmrParams.minDepth && 
    moveCount >= info.lmrParams.minMoveNumber &&
    !isCapture(move) && 
    !inCheck(board)) {
    reduction = getLMRReduction(depth, moveCount, info.lmrParams);
}

// Search with reduction
eval::Score score;
if (reduction > 0) {
    info.lmrStats.totalReductions++;
    // Reduced search with null window
    score = -negamax(board, depth - 1 - reduction, ply + 1, 
                    -alpha - 1, -alpha, searchInfo, info, limits, tt);
    
    // Re-search if beats alpha
    if (score > alpha) {
        info.lmrStats.reSearches++;
        score = -negamax(board, depth - 1, ply + 1, 
                        -beta, -alpha, searchInfo, info, limits, tt);
    } else {
        info.lmrStats.successfulReductions++;
    }
} else {
    // Normal full-depth search
    score = -negamax(board, depth - 1, ply + 1, 
                    -beta, -alpha, searchInfo, info, limits, tt);
}
```

**Commit:** "Phase 3: Integrate LMR into negamax search - bench [count]"
**🛑 STOP POINT:** Human MUST run OpenBench test vs Phase 2 build before proceeding to Phase 4

### Phase 4: Add Statistics and Check Detection (45 minutes)
**Files to modify:**
- `src/search/negamax.cpp` - Track and output statistics
- `src/core/move_generation.h/cpp` - Add simple givesCheck helper if needed

**Changes:**
1. Output LMR statistics in UCI info
2. Add basic "gives direct check" detection (optional)
3. Refine conditions to avoid reducing checking moves

**Statistics output:**
```cpp
if (info.lmrStats.totalReductions > 0) {
    std::cout << "info string LMR: " 
              << info.lmrStats.totalReductions << " reductions, "
              << info.lmrStats.reSearches << " re-searches ("
              << info.lmrStats.reSearchRate() << "% re-search rate)"
              << std::endl;
}
```

**Commit:** "Phase 4: Add LMR statistics and refine conditions - bench [count]"
**🛑 STOP POINT:** Human MUST run OpenBench test vs Phase 3 build before proceeding to Phase 5

### Phase 5: Tuning and Optimization (30 minutes)
**Files to modify:**
- `src/search/lmr.cpp` - Optimize reduction formula
- `src/search/negamax.cpp` - Fine-tune conditions

**Changes:**
1. Reduce PV nodes less aggressively (if beta-alpha > 1)
2. Add endgame scaling (reduce less with low material)
3. Optimize parameters based on SPRT results

**Optimizations:**
```cpp
// PV node detection and adjustment
bool isPV = (beta - alpha > 1);
if (isPV && reduction > 0) {
    reduction = std::max(1, reduction - 1);
}

// Endgame adjustment
if (board.totalMaterial() < 2000) {  // ~2 rooks per side
    reduction = (reduction + 1) / 2;  // Halve reduction
}
```

**Commit:** "Phase 5: Optimize LMR with PV and endgame adjustments - bench [count]"
**🛑 STOP POINT:** Human MUST run extended SPRT test vs Phase 4 build before marking stage complete

## Git Branch Management

**MUST follow** `/workspace/project_docs/Git_Strategy_for_SeaJay.txt`:
- Feature branch: `feature/20250819-lmr` ✅ (already created)
- Each commit MUST include: `bench <node-count>`
- Get bench count: `echo "bench" | ./bin/seajay | grep "Benchmark complete" | awk '{print $4}'`
- Push after EVERY commit for OpenBench testing
- Create `ob/` reference branch if needed before major changes

## Build System Requirements

**CRITICAL:** 
- **ALWAYS use `./build.sh production` or `./build_production.sh`**
- **NEVER use ninja directly** - OpenBench requires make compatibility
- **NEVER use cmake directly** unless through build.sh
- The build system MUST produce a Makefile for OpenBench compatibility

## Technical Considerations

From cpp-pro review:
1. **Store parameters in SearchData member** - ✅ Implemented
2. **Use [[likely]]/[[unlikely]] attributes** - To be added in Phase 3
3. **Calculate move properties once** - Will implement in Phase 3
4. **Add comprehensive statistics** - ✅ Implemented structure
5. **Follow existing UCI pattern** - ✅ Followed

## Chess Engine Considerations

From chess-engine-expert review:
1. **Start with MVV-LVA only** - Yes, history heuristic deferred
2. **Conservative initial parameters** - minMoveNumber=4, minDepth=3
3. **Always use null window for reduced search** - Will implement
4. **Reduce PV nodes less** - Will add in Phase 5
5. **Expected 40-60% node reduction** - Will monitor
6. **15-30% re-search rate is healthy** - Will track

## Risk Mitigation

### Critical Risks and Mitigations:
1. **Reducing tactical moves** 
   - Mitigation: Strict !isCapture() check
   - Never reduce when in check
   
2. **Over-reduction causing blind spots**
   - Mitigation: Conservative parameters (minMoveNumber=4)
   - Cap reduction at depth-2
   
3. **Performance regression**
   - Mitigation: Track statistics
   - Compare node counts before/after
   
4. **Breaking existing features**
   - Mitigation: Granular phases
   - Test after each phase

## Validation Strategy

1. **Unit tests** for reduction formula (Phase 2)
2. **Node count comparison** - expect 40-60% reduction
3. **Best move stability** - same moves in 95%+ positions
4. **Tactical suite** (WAC) - maintain 95%+ solve rate
5. **SPRT testing** after Phase 3, 4, 5

### Test Positions:
```cpp
// Middlegame with many quiet moves
"r1bqkb1r/pppp1ppp/2n2n2/1B2p3/4P3/5N2/PPPP1PPP/RNBQK2R w KQkq - 4 4"

// Open tactical position
"r1bqk2r/pp2bppp/2n1pn2/3p4/2PP4/2N2NP1/PP2PPBP/R1BQK2R w KQkq - 0 7"

// Quiet endgame
"8/5pk1/p3p1p1/4P2p/1P3P1P/6P1/3K4/8 w - - 0 1"
```

## Items Being Deferred

1. **History heuristic** - Not critical for initial LMR
2. **Complex givesCheck() detection** - Expensive, minimal benefit
3. **Killer move detection** - Not implemented yet
4. **Logarithmic reduction formula** - Linear is sufficient for 2200 ELO

## Success Criteria

- ✅ UCI options functional and parsed correctly
- ⬜ Unit tests pass for reduction formula
- ⬜ Node reduction of 40-60% achieved
- ⬜ SPRT shows +50 ELO or better
- ⬜ No tactical strength regression
- ⬜ 15-30% re-search rate
- ⬜ Clean, maintainable code

## Timeline Estimate

- Phase 1: ✅ COMPLETED
- Phase 2: 30 minutes
- Phase 3: 1 hour
- Phase 4: 45 minutes
- Phase 5: 30 minutes
- Testing buffer: 1 hour
- **Total remaining: ~3.5 hours**

## Current Status

**⚠️ CRITICAL STATUS:**
- **Phase 1:** IMPLEMENTED but NOT TESTED via OpenBench - CANNOT PROCEED
- **Phase 2:** PARTIALLY STARTED (lmr.h created) - MUST STOP until Phase 1 tested
- **Current Block:** Awaiting human to run OpenBench test of Phase 1

## Next Steps

**🛑 IMMEDIATE STOP - HUMAN ACTION REQUIRED:**
1. Commit Phase 1 changes with bench count
2. Push to feature/20250819-lmr branch
3. Run OpenBench test vs main branch
4. Report results back before ANY further development

**DO NOT PROCEED WITH PHASE 2 UNTIL PHASE 1 IS TESTED**

---

**Remember:** The time spent in planning saves multiples in debugging. Each phase must be tested before proceeding to the next.