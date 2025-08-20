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

### Phase 1: LMR Data Structures and UCI Options (TESTED)
**Status:** ✅ TESTED - OpenBench Complete
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
**OpenBench Results:** 
- Test: https://openbench.seajay-chess.dev/test/9/
- Elo: -1.73 +- 4.80 (95%)
- Games: N=2010 W=499 L=509 D=1002
- Result: **Negligible** (as expected - no functional change, UCI infrastructure only)

### Phase 2: Basic Reduction Formula ✅ COMPLETE
**Files created/modified:**
- `src/search/lmr.h` - ✅ Completed with full declarations
- `src/search/lmr.cpp` - ✅ Created with robust implementation
- `tests/test_lmr.cpp` - ✅ Created comprehensive GoogleTest suite
- `tests/test_lmr_simple.cpp` - ✅ Created standalone test for validation
- `CMakeLists.txt` - ✅ Updated to include LMR source files

**Implementation Details:**
1. ✅ Created getLMRReduction() function with conservative linear formula
2. ✅ Implemented shouldReduceMove() for eligibility checking
3. ✅ Handles all edge cases properly:
   - Returns 0 for invalid inputs (negative depths, move number < 1)
   - Allows 0 reductions when conditions not met
   - Uses 1-based move counting for intuitive usage
   - Caps at depth-2 to ensure meaningful search
4. ✅ Comprehensive unit tests covering:
   - Basic formula calculation
   - Very late move extra reductions
   - Parameter configurations
   - Edge cases and boundary conditions
   - Eligibility criteria

**Formula Implementation:**
```cpp
// Basic linear formula with depth factor
reduction = baseReduction + (depth - minDepth) / depthFactor;

// Extra reduction for very late moves (>8)
if (moveNumber > 8) {
    reduction += (moveNumber - 8) / 4;
}

// Cap at depth-2 to leave meaningful search
reduction = std::min(reduction, std::max(1, depth - 2));
```

**Validation Results:**
- All unit tests pass
- Sample reduction values verified correct
- Conservative parameters (depthFactor=3 after fix, was incorrectly 100)

**Commit:** "Phase 2: Add LMR reduction formula with unit tests - bench [count]"
**🛑 STOP POINT:** Human MUST run OpenBench test before proceeding to Phase 3

### Phase 3: Integrate LMR into Negamax ✅ COMPLETE (WITH CRITICAL FIXES)
**Status:** ✅ FULLY TESTED - Multiple iterations and bug fixes applied
**Files modified:**
- `src/search/negamax.cpp` - ✅ Added reduction logic in move loop
- `src/uci/uci.cpp` - ✅ Fixed default to false, made parsing case-insensitive, corrected depth factor
- `src/uci/uci.h` - ✅ Fixed default to false, corrected depth factor
- `src/search/lmr.cpp` - ✅ Formula verified working

**Changes:**
1. ✅ Calculate reduction for each move based on conditions
2. ✅ Implement reduced search with null window (-(alpha+1), -alpha)
3. ✅ Add re-search logic when score > alpha

**Critical Bugs Found and Fixed:**
1. **Depth Factor Bug (CRITICAL)**: Default was 100 instead of 3
   - With integer division: (6-3)/100 = 0 (no reduction!)
   - Fixed to 3: (6-3)/3 = 1 (proper reduction)
   - Commit: aed24ad
2. **Boolean Parsing**: Made case-insensitive to accept "True", "TRUE", etc.
   - Now accepts: true/false, True/False, 1/0, yes/no, on/off
   - Commit: 3c631a9

**Implementation Notes:**
- Correctly checks if in check BEFORE making move (saves state)
- Uses proper negamax perspective (side-to-move scoring)
- Skips LMR at root (ply == 0) for stability
- 91% node reduction achieved in local testing
- Verified LMR is actually being called and reducing moves

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

**Testing History:**
1. **Initial Commit (8a51f88):** "Phase 3: Integrate LMR into negamax search"
   - Regression test (LMREnabled=false): -1.25 ± 5.11 ELO ✅
   - Performance test (LMREnabled=true): -2.21 ± 10.17 ELO ❌ (negligible, bug suspected)
   
2. **Fix Commit (3c631a9):** "Make LMR boolean parsing case-insensitive"
   - Addressed potential parsing issue with "True" vs "true"
   
3. **Critical Fix (aed24ad):** "Correct LMR depth factor from 100 to 3"
   - Fixed integer division bug that prevented any reduction
   - Performance test (LMREnabled=true): -10.65 ± 10.25 ELO ❌ (UNEXPECTED!)
   - Test: https://openbench.seajay-chess.dev/test/15/
   
**Local Testing Verification:**
- Node reduction: 91% at depth 6 (1,076,826 → 95,957 nodes)
- Debug output confirms LMR is being called and applying reductions
- Formula produces expected reduction values

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

### Commit History:
1. **aa9166e:** "docs: Enforce OpenBench testing after EVERY phase + Phase 1 LMR UCI options"
2. **79f465b:** "Phase 2: Add LMR reduction formula with unit tests - bench 19191913"
3. **8a51f88:** "Phase 3: Integrate LMR into negamax search - bench 19191913"
4. **3c631a9:** "fix: Make LMR boolean parsing case-insensitive - bench 19191913"
5. **aed24ad:** "fix: CRITICAL - Correct LMR depth factor from 100 to 3 - bench 19191913"
6. **6d3777c:** "docs: Update Stage 18 - Phase 3 complete, awaiting LMREnabled=true test results"
7. **9bda88f:** "docs: Add Bug #014 - Critical illegal moves during gameplay"

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
- ✅ Unit tests pass for reduction formula
- ✅ Node reduction of 40-60% achieved (91% reduction observed!)
- ❌ SPRT shows +50 ELO or better (Currently -10 ELO)
- ❓ No tactical strength regression (Unclear, needs investigation)
- ❓ 15-30% re-search rate (Not measured yet - needs Phase 4)
- ✅ Clean, maintainable code

## Timeline Estimate

- Phase 1: ✅ COMPLETED
- Phase 2: ✅ COMPLETED
- Phase 3: 1 hour
- Phase 4: 45 minutes
- Phase 5: 30 minutes
- Testing buffer: 1 hour
- **Total remaining: ~3.5 hours**

## Current Status

**Phase 1:** ✅ TESTED via OpenBench - Result: -1.73 ± 4.80 (negligible, expected for UCI infrastructure)
**Phase 2:** ✅ TESTED via OpenBench - Result: +3.04 ± 5.14 (negligible, as expected)
**Phase 3:** ✅ COMPLETE - Initially showed -10 ELO loss due to lack of move ordering

### LMR Integration with Move Ordering (SUCCESS!)
**Branch:** `integration/lmr-with-move-ordering`
**Date:** August 20, 2025

After implementing Stages 19-20 (Killer Moves + History Heuristic):
- **Regression test (LMREnabled=false):** -2.08 ± 6.22 ELO ✅ (negligible overhead)
- **Performance test (LMREnabled=true):** +36.98 ± 9.67 ELO ✅ (SIGNIFICANT GAIN!)
- **Test results:** https://openbench.seajay-chess.dev/test/30/

**Current State:** ✅ LMR WORKING SUCCESSFULLY with proper move ordering

## Investigation Summary

### What We Know Works:
1. **LMR is functioning:** Debug output confirms reductions are being applied
2. **Node reduction is massive:** 91% fewer nodes searched (verified locally)
3. **Formula is correct:** Unit tests pass, manual calculations verified
4. **No crashes or errors:** Engine runs stably with LMR enabled

### What's Going Wrong:
Despite massive node reduction, we're LOSING 10 ELO with LMR enabled. This suggests:
1. **Over-reduction:** We may be reducing important moves that shouldn't be reduced
2. **Move ordering issues:** If good moves aren't ordered first, LMR reduces them incorrectly
3. **Re-search overhead:** Too many re-searches could negate the benefit
4. **Search instability:** Reduced searches may miss critical tactics

### Key Observations:
- OpenBench testing at 10+0.1 time control
- 8MB hash (sufficient for this time control)
- MVV-LVA move ordering is active (but only for captures)
- **Quiet moves are NOT ordered** - they remain in generation order
- TT move ordering is implemented

### Critical Issue RESOLVED:
**Move ordering was the key!** With Killer Moves and History Heuristic implemented, quiet moves are now properly ordered, allowing LMR to reduce genuinely unpromising moves rather than randomly ordered ones.

## LMR Tuning Opportunities

### Current Conservative Parameters:
```
LMREnabled = false (default)
LMRMinDepth = 3
LMRMinMoveNumber = 8    // Very conservative
LMRBaseReduction = 1
LMRDepthFactor = 3
```

### Proposed Tuning Tests:

#### Test 1: More Aggressive Move Count Threshold
```
LMRMinMoveNumber = 6  // Reduce after 6th move instead of 8th
```
Expected: +5-10 ELO by reducing more moves

#### Test 2: Even More Aggressive  
```
LMRMinMoveNumber = 4  // Standard for many engines
```
Expected: +10-15 ELO if move ordering is good enough

#### Test 3: Increased Base Reduction
```
LMRBaseReduction = 2  // Reduce by 2 plies minimum
LMRMinMoveNumber = 6
```
Expected: Higher risk/reward, could be +15 or -10 ELO

#### Test 4: Adjust Depth Factor
```
LMRDepthFactor = 2    // More aggressive scaling with depth
LMRMinMoveNumber = 6
```
Expected: Better at higher depths, +5-10 ELO

### Advanced Tuning (Future):
1. **Reduce bad captures:** With SEE integration, reduce captures with SEE < 0
2. **History-based LMR:** Less reduction for moves with good history scores
3. **Threat-based LMR:** Less reduction when opponent has threats
4. **PV node consideration:** Less aggressive LMR in PV nodes

## Next Steps

**Immediate Investigation Required:**
1. **Check re-search rate:** Add statistics output to see if we're re-searching too much
2. **Verify move ordering:** Confirm quiet moves are truly unordered
3. **Consider reducing parameters:** Make LMR less aggressive (higher minMoveNumber)
4. **Test at longer time control:** 10+0.1 may be too fast to see benefits

**Potential Solutions:**
1. **Phase 4:** Add history heuristic for quiet move ordering (critical!)
2. **Phase 5:** Tune parameters to be less aggressive
3. **Alternative:** Implement killer moves for better ordering
4. **Emergency:** Reduce minMoveNumber to 6 or 8 (only reduce very late moves)

---

**Remember:** The time spent in planning saves multiples in debugging. Each phase must be tested before proceeding to the next.