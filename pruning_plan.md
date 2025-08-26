# SeaJay Pruning Optimization Plan

**Branch:** feature/analysis/20250826-pruning-aggressiveness  
**Created:** August 26, 2025  
**Status:** IMPLEMENTATION READY  
**Parent Analysis:** pruning_aggressiveness_analysis.md  

## Executive Summary

SeaJay exhibits overly aggressive pruning resulting in:
- **20% fewer nodes** searched compared to peer engines
- **171 tactical blunders** in 29 games (vs <50 expected)
- **Missing standard techniques** used by all modern engines

This plan provides a systematic approach to fix pruning issues through conservative adjustments and implementation of missing techniques.

## Phase Summary Table

| Phase | Sub-phases | Changes | SPRT per Sub-phase | Total Time |
|-------|------------|---------|-------------------|------------|
| **Phase 1** | 5 | Conservative parameter adjustments | Yes - Each tested | 8-10 hours |
| 1.1 | - | Null move margin 120→90cp | `[-3, 3]` | 1-2 hours |
| 1.2 | - | Null move margin 90→70cp | `[-2, 3]` | 1-2 hours |
| 1.3 | - | SEE default → conservative | `[-2, 3]` | 1-2 hours |
| 1.4 | - | Null verification depth≥12 | `[-2, 2]` | 1-2 hours |
| 1.5 | - | Null verification depth≥10 | `[-1, 3]` | 1-2 hours |
| **Phase 2** | 5 | Futility pruning implementation | Yes - Each tested | 10-12 hours |
| 2.1 | - | Basic futility (conservative) | `[0, 5]` | 2-3 hours |
| 2.2 | - | Tune margins | `[0, 3]` | 2 hours |
| 2.3 | - | Extend to depth 6 | `[0, 3]` | 2 hours |
| 2.4 | - | Final tuning to Laser | `[0, 5]` | 2 hours |
| 2.5 | - | UCI options | No SPRT | 1 hour |
| **Phase 3** | 3 | Move count pruning | Yes - Each tested | 6-8 hours |
| **Phase 4** | 2 | Razoring | Yes - Each tested | 4 hours |

**Critical Rule:** STOP and wait for SPRT results after EACH sub-phase before proceeding.

## Critical Requirements

### OpenBench Compatibility
**EVERY commit MUST include bench count in exact format:**
```bash
# Get bench count after building
echo "bench" | ./seajay | grep "Benchmark complete" | awk '{print $4}'

# Commit message format
git commit -m "feat: [description]

bench 19191913"  # EXACT format required
```

### Testing Protocol
1. **Local validation** before pushing
2. **SPRT testing** for each phase
3. **Tactical suite** verification
4. **NO merging to main** without human approval

### CRITICAL: Branch Management
**IMPORTANT:** This branch (`feature/analysis/20250826-pruning-aggressiveness`) should NEVER be merged directly to main.
- **Parent Branch:** This is a feature analysis branch created from prior work
- **Merge Target:** Parent feature branch or as directed by human
- **NOT main:** All merges must go through proper feature branch hierarchy

---

## PHASE 1: Conservative Parameter Adjustments [HIGH PRIORITY]

**Goal:** Reduce tactical blindness by making existing pruning less aggressive  
**Expected Impact:** -5 to 0 Elo short term, but 50%+ reduction in blunders  
**Time Estimate:** 8-10 hours (including SPRT validation for each sub-phase)

**CRITICAL:** Each sub-phase MUST be independently tested with SPRT before proceeding to the next.

---

### Phase 1.1: Null Move Static Margin - First Reduction
**Change:** Reduce from 120cp to 90cp (intermediate step)  
**Reference:** Laser uses 70cp, this is 25% reduction toward target

#### Implementation:
```cpp
// File: src/search/negamax.cpp, line ~351
// Current:
eval::Score margin = eval::Score(limits.nullMoveStaticMargin * depth);  // 120

// Change to:
eval::Score margin = eval::Score(90 * depth);  // Reduced from 120
```

#### Local Validation:
```bash
# Tactical suite before change
echo "position fen 2r3k1/1q1nbppp/r3p3/3pP3/p1pP4/P1Q2N2/1PRN1PPP/2R3K1 b - - 0 23" | ./bin/seajay
# Record nodes and best move

# Apply change, rebuild
./build.sh

# Same position after change
echo "position fen 2r3k1/1q1nbppp/r3p3/3pP3/p1pP4/P1Q2N2/1PRN1PPP/2R3K1 b - - 0 23" | ./bin/seajay
# Should see MORE nodes (less aggressive pruning)
```

#### Commit and Push:
```bash
BENCH=$(echo "bench" | ./bin/seajay | grep "Benchmark complete" | sed 's/.*complete: \([0-9]*\) nodes.*/\1/')
git add -A
git commit -m "fix: Reduce null move static margin from 120cp to 90cp

First step toward less aggressive null move pruning. Laser uses 70cp,
this reduces our margin by 25% as an intermediate step.

Expected: Small Elo regression but fewer tactical misses.

bench $BENCH"
git push origin feature/analysis/20250826-pruning-aggressiveness
```

#### SPRT Validation:
**OpenBench Test Configuration:**
- Dev Branch: `feature/analysis/20250826-pruning-aggressiveness`
- Base Branch: Previous commit on same branch (NOT main)
- Time Control: `10+0.1`
- SPRT Bounds: `[-3, 3]` (allowing small regression)
- Book: `UHO_4060_v2.epd`

**STOP - Wait for SPRT completion before proceeding to Phase 1.2**

---

### Phase 1.2: [REMOVED - Will be handled via SPSA tuning in Phase 1.6]
**Note:** The final margin value will be determined through SPSA tuning after other fixes are in place.
The NullMoveStaticMargin UCI parameter (range 50-300) allows for automated tuning.

**PROCEED DIRECTLY TO Phase 1.2a**

---

### Phase 1.2a: Extend Static Null Move Pruning Depth
**Change:** Increase static null move pruning from depth <= 3 to depth <= 6  
**Reference:** Laser uses depth <= 6, most modern engines use 4-7 range  
**Note:** This is a MORE IMPACTFUL change than margin adjustment

#### Implementation:
```cpp
// File: src/search/negamax.cpp, line ~330
// Current:
if (!isPvNode && depth <= 3 && depth > 0 && !weAreInCheck && std::abs(beta.value()) < MATE_BOUND - MAX_PLY) {

// Change to:
if (!isPvNode && depth <= 6 && depth > 0 && !weAreInCheck && std::abs(beta.value()) < MATE_BOUND - MAX_PLY) {
```

#### Local Validation:
```bash
# Test at depths 4-6 where new pruning would trigger
echo "position fen rnbqkbnr/pppp1ppp/8/4p3/4P3/5N2/PPPP1PPP/RNBQKB1R b KQkq - 1 2" | ./bin/seajay
# Should see reduced nodes at depths 4-6
```

#### Commit:
```bash
BENCH=$(echo "bench" | ./bin/seajay | grep "Benchmark complete" | sed 's/.*complete: \([0-9]*\) nodes.*/\1/')
git add -A
git commit -m "feat: Extend static null move pruning to depth <= 6

Increase static null move (reverse futility) pruning depth limit from 3 to 6,
matching Laser and other modern engines. This allows pruning at mid-depths
when position is significantly above beta.

bench $BENCH"
git push
```

#### SPRT Validation:
- SPRT Bounds: `[0, 5]` (should be positive - more pruning opportunities)
- Base: Phase 1.2 commit
- Expected: 5-10% node reduction with minimal tactical impact

**STOP - Wait for SPRT completion before proceeding to Phase 1.2b**

---

### Phase 1.2b: Fix Static Null Move Material Balance Check
**Change:** Remove or invert the material balance pre-check that prevents pruning  
**Reference:** See `/workspace/static_null_comparison.md` for detailed analysis  
**Issue:** Current check only allows pruning when NOT behind, but should trigger when AHEAD

#### Current Problem:
```cpp
// Line 346: This logic seems backwards!
if (board.material().balance(board.sideToMove()).value() - beta.value() > -200) {
    // Only evaluates if material balance - beta > -200
    // This PREVENTS pruning when we're ahead!
}
```

#### Implementation Option A - Remove the check entirely:
```cpp
// File: src/search/negamax.cpp, lines 343-358
// Remove the material balance check completely
if (staticEval == eval::Score::zero()) {
    staticEval = board.evaluate();
    searchInfo.setStaticEval(ply, staticEval);
}

// Margin based on depth (tunable)
eval::Score margin = eval::Score(limits.nullMoveStaticMargin * depth);

if (staticEval - margin >= beta) {
    info.nullMoveStats.staticCutoffs++;
    return staticEval - margin;
}
```

#### Implementation Option B - Invert the logic:
```cpp
// Only prune when we're materially ahead or equal
if (board.material().balance(board.sideToMove()).value() >= 0) {
    // Now we prune when ahead, which makes sense
}
```

#### Local Validation:
```bash
# Test position where we're materially ahead
echo "position fen r1bqkb1r/pppp1ppp/2n2n2/1B2p3/4P3/5N2/PPPP1PPP/RNBQK2R w KQkq - 4 4
go depth 8" | ./bin/seajay 2>/dev/null | grep "info depth 8"
# Should see fewer nodes with the fix
```

#### Commit:
```bash
BENCH=$(echo "bench" | ./bin/seajay | grep "Benchmark complete" | sed 's/.*complete: \([0-9]*\) nodes.*/\1/')
git add -A
git commit -m "fix: Remove restrictive material balance check in static null move

The material balance pre-check was preventing static null move pruning
in positions where we're ahead. Static null move should trigger when
we can afford to pass, not when we're behind.

Reference: static_null_comparison.md shows Publius and Laser don't use
this check.

bench $BENCH"
git push
```

#### SPRT Validation:
- SPRT Bounds: `[0, 5]` (should enable more pruning)
- Base: Phase 1.2a commit
- Expected: 5-10% node reduction when ahead materially

**STOP - Wait for SPRT completion before proceeding to Phase 1.2c**

---

### Phase 1.2c: Simplify Static Null Move Implementation
**Change:** Remove complex caching logic and simplify to match Publius/Laser style  
**Reference:** See `/workspace/static_null_comparison.md` - Publius uses simplest approach  
**Goal:** Reduce overhead and make pruning more predictable

#### Current Complex Implementation:
```cpp
// Lines 335-358: Complex caching and multiple checks
if (ply > 0) {
    int cachedEval = searchInfo.getStackEntry(ply).staticEval;
    if (cachedEval != 0) {
        staticEval = eval::Score(cachedEval);
    }
}
// ... more complex logic
```

#### New Simplified Implementation (Publius-style):
```cpp
// File: src/search/negamax.cpp, replace lines 330-359
// Simple static null move pruning (matches Publius/Laser style)
if (!isPvNode && depth <= 6 && depth > 0 && !weAreInCheck 
    && std::abs(beta.value()) < MATE_BOUND - MAX_PLY
    && board.nonPawnMaterial(board.sideToMove()) > ZUGZWANG_THRESHOLD) {
    
    eval::Score staticEval = board.evaluate();
    eval::Score margin = eval::Score(limits.nullMoveStaticMargin * depth);
    
    if (staticEval - margin >= beta) {
        info.nullMoveStats.staticCutoffs++;
        return staticEval - margin;
    }
}
```

#### Benefits:
1. **Cleaner code** - easier to understand and maintain
2. **Less overhead** - no complex caching logic
3. **More predictable** - always evaluates when conditions met
4. **Matches successful engines** - Publius gained 14 Elo with simple version

#### Local Validation:
```bash
# Compare node counts and time
echo "position startpos
go depth 12" | ./bin/seajay 2>/dev/null | grep "info depth 12"
# Should see similar nodes but potentially faster nps
```

#### Commit:
```bash
BENCH=$(echo "bench" | ./bin/seajay | grep "Benchmark complete" | sed 's/.*complete: \([0-9]*\) nodes.*/\1/')
git add -A
git commit -m "refactor: Simplify static null move implementation

Remove complex caching logic and simplify to match Publius/Laser style.
This reduces overhead and makes the pruning more predictable.

Publius gained 14 Elo with this simple approach.

bench $BENCH"
git push
```

#### SPRT Validation:
- SPRT Bounds: `[-2, 3]` (refactor, should be neutral)
- Base: Phase 1.2b commit
- Expected: Neutral Elo, potentially better nps

**STOP - Wait for SPRT completion before proceeding to Phase 1.3**

---

### Phase 1.3: SEE Pruning Conservative Default
**Change:** Switch default SEE pruning from "off" to "conservative"  
**Reference:** Conservative uses -100cp threshold vs aggressive -75cp

#### Implementation:
```cpp
// File: src/uci/uci.cpp, line ~90
// Change default from "off" to "conservative"
std::cout << "option name SEEPruning type combo default conservative var off var conservative var aggressive" << std::endl;

// File: src/uci/uci.cpp, line ~744 (constructor or initialization)
m_seePruning = "conservative";  // was "off"
```

#### Local Validation:
```bash
# Test position with hanging piece
echo "position fen rnbqkb1r/pppp1ppp/5n2/4p3/4P3/5N2/PPPP1PPP/RNBQKB1R w KQkq - 4 4" | ./bin/seajay
# Should still find good tactics but prune bad captures
```

#### Commit:
```bash
BENCH=$(echo "bench" | ./bin/seajay | grep "Benchmark complete" | sed 's/.*complete: \([0-9]*\) nodes.*/\1/')
git add -A
git commit -m "fix: Change SEE pruning default to conservative mode

Switch from no SEE pruning to conservative mode (-100cp threshold).
This prevents pruning of potentially good tactical captures while
still eliminating clearly bad exchanges.

bench $BENCH"
git push
```

#### SPRT Validation:
- SPRT Bounds: `[-2, 3]` (should be neutral to positive)
- Base: Phase 1.2 commit
- Expected: Small improvement in tactical positions

**STOP - Wait for SPRT completion before proceeding to Phase 1.4**

---

### Phase 1.4: Null Move Verification Search - Depth 12+
**Change:** Add verification search at depth >= 12 (conservative start)  
**Reference:** Laser verifies at depth >= 10, we start more conservative

#### Implementation:
```cpp
// File: src/search/negamax.cpp, after line ~406
if (nullScore >= beta) {
    info.nullMoveStats.cutoffs++;
    
    // Phase 1.4: Add verification search for very deep nodes
    if (depth >= 12) {  // Start conservative at depth 12
        // Verification search at reduced depth
        eval::Score verifyScore = negamax(
            board,
            depth - nullMoveReduction - 1,  // Even shallower
            ply,
            beta - eval::Score(1),
            beta,
            searchInfo,
            info,
            limits,
            tt,
            false
        );
        
        if (verifyScore < beta) {
            // Verification failed, don't trust null move
            info.nullMoveStats.verificationFails++;
            // Continue with normal search instead of returning
        } else {
            // Verification passed, null move cutoff is valid
            if (std::abs(nullScore.value()) < MATE_BOUND - MAX_PLY) {
                return nullScore;
            }
        }
    } else {
        // Shallow depth, trust null move without verification
        if (std::abs(nullScore.value()) < MATE_BOUND - MAX_PLY) {
            return nullScore;
        }
    }
}
```

#### Commit:
```bash
BENCH=$(echo "bench" | ./bin/seajay | grep "Benchmark complete" | sed 's/.*complete: \([0-9]*\) nodes.*/\1/')
git add -A
git commit -m "feat: Add null move verification search at depth >= 12

Implement verification search for deep null move pruning to prevent
tactical oversights in critical positions. Starting conservatively
at depth 12 (Laser uses 10).

bench $BENCH"
git push
```

#### SPRT Validation:
- SPRT Bounds: `[-2, 2]` (should be neutral)
- Base: Phase 1.3 commit
- Expected: Minimal impact at normal depths, safer at deep searches

**STOP - Wait for SPRT completion before proceeding to Phase 1.5**

---

### Phase 1.5: Null Move Verification - Extend to Depth 10+
**Change:** Lower verification threshold from 12 to 10 (match Laser)  
**Prerequisite:** Phase 1.4 must show no regression

#### Implementation:
```cpp
// File: src/search/negamax.cpp
// Change from:
if (depth >= 12) {  // Conservative

// To:
if (depth >= 10) {  // Match Laser's threshold
```

#### Commit:
```bash
BENCH=$(echo "bench" | ./bin/seajay | grep "Benchmark complete" | sed 's/.*complete: \([0-9]*\) nodes.*/\1/')
git add -A
git commit -m "feat: Lower null move verification threshold to depth >= 10

Extend verification search to depth 10+ positions, matching Laser's
implementation. This provides better tactical safety in mid-depth
searches.

bench $BENCH"
git push
```

#### SPRT Validation:
- SPRT Bounds: `[-1, 3]` (should be slightly positive)
- Base: Phase 1.4 commit
- Expected: Slightly fewer nodes but better tactical accuracy

**STOP - Wait for SPRT completion before proceeding to Phase 1.6**

---

### Phase 1.6: SPSA Tuning of NullMoveStaticMargin
**Change:** Use SPSA to find optimal null move static margin value  
**Prerequisite:** All other Phase 1 changes must be complete and tested  
**Note:** This replaces the manual Phase 1.2 (70cp adjustment)

#### Current State After Phase 1:
- Margin set to 90cp (reduced from 120cp)
- Depth extended to 6 (from 3)
- Material check fixed/removed
- Implementation simplified
- Verification search added

#### SPSA Configuration:
```
Parameter: NullMoveStaticMargin
Current Value: 90
Range: [50, 200]  # Conservative range for safety
Initial Step: 10
Games: 20000+
```

#### OpenBench SPSA Setup:
1. Ensure all Phase 1.1-1.5 changes are merged
2. Configure SPSA test:
   - Base: Current branch with margin=90
   - Parameter: `NullMoveStaticMargin`
   - Range: `[50, 200]`
   - Focus on tactical test positions if possible

#### Expected Outcome:
- **Laser uses 70cp** - we may converge near this
- **Publius uses 135cp** - but they have different conditions
- **Optimal for SeaJay:** Likely 60-90cp range given our safety features

#### Validation:
- Monitor tactical performance during tuning
- Ensure blunder rate doesn't increase
- Check node count changes
- Verify depth isn't negatively impacted

#### Final Implementation:
```bash
# After SPSA completes, update default value
# File: src/uci/uci.h, line 66
int m_nullMoveStaticMargin = [SPSA_RESULT];  // SPSA-tuned value

# File: src/search/types.h, line 73  
int nullMoveStaticMargin = [SPSA_RESULT];   // SPSA-tuned value

# Update UCI option default
# File: src/uci/uci.cpp, line 101
std::cout << "option name NullMoveStaticMargin type spin default [SPSA_RESULT] min 50 max 300" << std::endl;
```

#### Commit:
```bash
BENCH=$(echo "bench" | ./bin/seajay | grep "Benchmark complete" | sed 's/.*complete: \([0-9]*\) nodes.*/\1/')
git add -A
git commit -m "tune: Set NullMoveStaticMargin to SPSA-optimized value

After SPSA tuning with [X] games, optimal value determined to be [Y]cp.
This balances tactical safety with pruning effectiveness.

Previous: 90cp (manually set)
Optimized: [Y]cp (SPSA result)
Comparison: Laser=70cp, Publius=135cp

bench $BENCH"
git push
```

**Phase 1 COMPLETE after SPSA tuning - Proceed to Phase 2**

---

## PHASE 2: Futility Pruning Implementation [CRITICAL]

**Goal:** Implement standard futility pruning (missing in SeaJay)  
**Expected Impact:** +10-15% node reduction, +5-10 Elo  
**Time Estimate:** 10-12 hours (including SPRT validation for each sub-phase)  
**Reference:** Laser formula: `staticEval <= alpha - 115 - 90*depth`

**CRITICAL:** This is a major missing feature. Each sub-phase must be carefully validated.

---

### Phase 2.1: Basic Futility Pruning - Conservative Start
**Change:** Add futility pruning with very conservative margins  
**Initial Formula:** `staticEval <= alpha - 150 - 60*depth` (more conservative than Laser)

#### Implementation:
```cpp
// File: src/search/negamax.cpp, after null move pruning (~line 420)

// Phase 2.1: Basic Futility Pruning (Conservative)
// Only for non-PV nodes at shallow depths
if (!isPvNode && depth <= 4 && depth > 0 && !weAreInCheck 
    && std::abs(alpha.value()) < MATE_BOUND - MAX_PLY
    && std::abs(beta.value()) < MATE_BOUND - MAX_PLY) {
    
    // Get static evaluation (use cached if available)
    eval::Score staticEval = eval::Score::zero();
    if (ply > 0) {
        int cachedEval = searchInfo.getStackEntry(ply).staticEval;
        if (cachedEval != 0) {
            staticEval = eval::Score(cachedEval);
        }
    }
    
    if (staticEval == eval::Score::zero()) {
        staticEval = board.evaluate();
        searchInfo.setStaticEval(ply, staticEval);
    }
    
    // Very conservative margin to start
    int futilityMargin = 150 + 60 * depth;  // More conservative than Laser
    
    if (staticEval + eval::Score(futilityMargin) <= alpha) {
        info.futilityPruned++;  // Add counter to SearchData
        return staticEval;  // Position is futile
    }
}
```

#### Adding Statistics Counter:
```cpp
// File: src/search/types.h or search_data.h
struct SearchData {
    // ... existing members
    uint64_t futilityPruned = 0;
    uint64_t extendedFutilityPruned = 0;
};
```

#### Commit:
```bash
BENCH=$(echo "bench" | ./bin/seajay | grep "Benchmark complete" | sed 's/.*complete: \([0-9]*\) nodes.*/\1/')
git add -A
git commit -m "feat: Add basic futility pruning with conservative margins

Implement futility pruning for depths 1-4 with formula:
staticEval <= alpha - 150 - 60*depth

This is more conservative than Laser (115 + 90*depth) to ensure
we don't miss tactics while implementing this new feature.

bench $BENCH"
git push
```

#### SPRT Validation:
- SPRT Bounds: `[0, 5]` (should be positive)
- Base: Phase 1.5 commit
- Expected: 5-10% node reduction, small Elo gain

**STOP - Wait for SPRT completion before proceeding to Phase 2.2**

---

### Phase 2.2: Futility Pruning - Tune Margins Closer to Laser
**Change:** Adjust margins from 150+60*depth to 130+75*depth  
**Prerequisite:** Phase 2.1 must show positive results

#### Implementation:
```cpp
// File: src/search/negamax.cpp
// Change from:
int futilityMargin = 150 + 60 * depth;  // Very conservative

// To:
int futilityMargin = 130 + 75 * depth;  // Moving toward Laser
```

#### Commit:
```bash
BENCH=$(echo "bench" | ./bin/seajay | grep "Benchmark complete" | sed 's/.*complete: \([0-9]*\) nodes.*/\1/')
git add -A
git commit -m "feat: Tune futility margins closer to optimal

Adjust futility pruning margins from 150+60*depth to 130+75*depth,
moving closer to Laser's proven formula while maintaining safety.

bench $BENCH"
git push
```

#### SPRT Validation:
- SPRT Bounds: `[0, 3]` (should be neutral to positive)
- Base: Phase 2.1 commit

**STOP - Wait for SPRT completion before proceeding to Phase 2.3**

---

### Phase 2.3: Futility Pruning - Extend to Depth 6
**Change:** Enable futility pruning up to depth 6 (from depth 4)

#### Implementation:
```cpp
// File: src/search/negamax.cpp
// Change from:
if (!isPvNode && depth <= 4 && depth > 0 && !weAreInCheck

// To:
if (!isPvNode && depth <= 6 && depth > 0 && !weAreInCheck  // Extended to depth 6
```

#### Commit:
```bash
BENCH=$(echo "bench" | ./bin/seajay | grep "Benchmark complete" | sed 's/.*complete: \([0-9]*\) nodes.*/\1/')
git add -A
git commit -m "feat: Extend futility pruning to depth 6

Enable futility pruning for depths 1-6 (was 1-4), matching standard
engine practice. Margins remain at 130+75*depth.

bench $BENCH"
git push
```

#### SPRT Validation:
- SPRT Bounds: `[0, 3]` (should be positive)
- Base: Phase 2.2 commit
- Expected: Additional node reduction at mid-depths

**STOP - Wait for SPRT completion before proceeding to Phase 2.4**

---

### Phase 2.4: Futility Pruning - Final Tuning to Match Laser
**Change:** Adjust to Laser's proven formula: 115 + 90*depth  
**Prerequisite:** All previous futility phases must pass

#### Implementation:
```cpp
// File: src/search/negamax.cpp
// Final adjustment:
int futilityMargin = 115 + 90 * depth;  // Match Laser exactly
```

#### Commit:
```bash
BENCH=$(echo "bench" | ./bin/seajay | grep "Benchmark complete" | sed 's/.*complete: \([0-9]*\) nodes.*/\1/')
git add -A
git commit -m "feat: Final futility tuning to match Laser formula

Adjust futility margins to 115+90*depth, matching Laser's proven
formula. This completes the futility pruning implementation.

bench $BENCH"
git push
```

#### SPRT Validation:
- SPRT Bounds: `[0, 5]` (expect good improvement)
- Base: Phase 2.3 commit
- Expected: Optimal balance of pruning and accuracy

**STOP - Phase 2 Complete - Validate total improvement vs Phase 1.5**

---

### Phase 2.5: Add UCI Options for Future Tuning
**Change:** Expose futility parameters via UCI for SPSA tuning  
**Prerequisite:** Core futility implementation working

#### Implementation:
```cpp
// File: src/uci/uci.cpp
// Add futility options
std::cout << "option name FutilityPruning type check default true" << std::endl;
std::cout << "option name FutilityMarginBase type spin default 115 min 50 max 200" << std::endl;
std::cout << "option name FutilityMarginDepth type spin default 90 min 50 max 150" << std::endl;

// Add to SearchLimits structure
bool useFutility = true;
int futilityBase = 115;  // Laser's value
int futilityDepthMultiplier = 90;  // Laser's value
```

#### Commit:
```bash
BENCH=$(echo "bench" | ./bin/seajay | grep "Benchmark complete" | sed 's/.*complete: \([0-9]*\) nodes.*/\1/')
git add -A
git commit -m "feat: Add UCI options for futility pruning parameters

Expose futility pruning parameters via UCI for future SPSA tuning.
Default values match Laser's proven formula.

bench $BENCH"
git push
```

#### SPRT Validation:
- No SPRT needed (UCI options only)
- Verify options work via UCI interface

---

## Phase 2 Validation Suite

#### Test Positions:
```
// Quiet positions where futility should trigger
position fen r1bqkbnr/pppp1ppp/2n5/4p3/4P3/5N2/PPPP1PPP/RNBQKB1R w KQkq - 4 4
go depth 10

// Tactical positions where futility should NOT trigger
position fen r1b1kb1r/pp1n1ppp/2p1p3/8/3PP3/2N2N2/PPP2PPP/R1BQKB1R b KQkq - 0 7
go depth 10
```

---

## PHASE 3: Move Count Pruning [MEDIUM PRIORITY]

**Goal:** Implement late move pruning based on move count  
**Expected Impact:** +10-15% node reduction  
**Time Estimate:** 5-6 hours  
**Reference:** Laser uses depth/improvement-based tables

### Phase 3.1: Basic Move Count Framework

#### Implementation:
```cpp
// File: src/search/negamax.cpp, in move loop after moveCount++

// Phase 3.1: Move Count Pruning (Late Move Pruning)
if (!isPvNode && !weAreInCheck && depth <= 8 && moveCount > 1) {
    // Don't prune captures, promotions, or killers
    if (!isCapture(move) && !isPromotion(move) && !info.killers.isKiller(ply, move)) {
        
        // Depth-based move count limits
        static const int moveCountLimit[9] = {
            0,   // depth 0 (not used)
            3,   // depth 1
            6,   // depth 2  
            12,  // depth 3
            18,  // depth 4
            24,  // depth 5
            32,  // depth 6
            40,  // depth 7
            50   // depth 8
        };
        
        // Check if we're improving (compare to previous ply's eval)
        bool improving = false;
        if (ply >= 2) {
            int prevEval = searchInfo.getStackEntry(ply - 2).staticEval;
            int currEval = searchInfo.getStackEntry(ply).staticEval;
            if (prevEval != 0 && currEval != 0) {
                improving = (currEval > prevEval);
            }
        }
        
        // Adjust limit based on improvement
        int limit = moveCountLimit[depth];
        if (!improving) {
            limit = limit * 3 / 4;  // Reduce limit if not improving
        }
        
        if (moveCount > limit) {
            info.moveCountPruned++;
            continue;  // Skip this move
        }
    }
}
```

### Phase 3.2: History-Based Adjustment

#### Implementation:
```cpp
// Adjust move count limit based on history score
int historyScore = info.history.get(board.sideToMove(), moveFrom(move), moveTo(move));
int historyThreshold = 1000;  // Tune this value

// Good history moves get extra chances
if (historyScore > historyThreshold) {
    limit += 5;  // Allow 5 more moves if good history
}
```

### Phase 3.3: Countermove Consideration

```cpp
// Don't prune countermoves
Move counterMove = info.counterMoves.getCounterMove(prevMove);
if (move == counterMove) {
    continue;  // Don't prune this move
}
```

---

## PHASE 4: Razoring Implementation [LOW PRIORITY]

**Goal:** Prune at very shallow depths when far below alpha  
**Expected Impact:** +5% node reduction  
**Time Estimate:** 3-4 hours  
**Reference:** Laser uses 300cp margin at depth <= 2

### Phase 4.1: Basic Razoring

#### Implementation:
```cpp
// File: src/search/negamax.cpp, before move generation

// Phase 4.1: Razoring
if (!isPvNode && !weAreInCheck && depth <= 2) {
    // Get static eval
    eval::Score staticEval = board.evaluate();
    
    // Razoring margin
    int razorMargin = 300;  // Conservative start
    
    if (staticEval + eval::Score(razorMargin) < alpha) {
        // Drop into quiescence search
        eval::Score razorScore = quiescence(board, ply, alpha, beta, 
                                           searchInfo, info, limits, *tt, 0, false);
        if (razorScore <= alpha) {
            info.razoringCutoffs++;
            return razorScore;
        }
    }
}
```

---

## PHASE 5: Advanced Techniques [FUTURE]

These require more complex implementation:

### Phase 5.1: ProbCut
- Shallow search to predict deep failures
- Requires careful tuning

### Phase 5.2: History-Based Pruning
- Prune moves with consistently bad history
- Needs mature history tables

### Phase 5.3: SEE-Based Move Ordering
- Order all moves by SEE value
- Currently only used for captures

---

## Testing and Validation Protocol

### For Each Phase:

#### 1. Pre-Implementation Baseline:
```bash
# Record current performance
echo "position fen [standard test positions]" | ./seajay
# Save node counts, depth, time

# Run tactical suite
./run_tactical_suite.sh > baseline.txt
```

#### 2. Post-Implementation Testing:
```bash
# Same positions, compare metrics
echo "position fen [same positions]" | ./seajay
# Compare node counts (should change as expected)

# Tactical suite should maintain or improve
./run_tactical_suite.sh > after.txt
diff baseline.txt after.txt
```

#### 3. SPRT Testing Guidelines:

**CRITICAL:** All SPRT tests should use previous commit on THIS branch as base, NOT main branch.

| Phase | Expected Impact | SPRT Bounds | Time Control |
|-------|----------------|-------------|--------------|
| 1.1 Null Move | Small regression | `[-3, 3]` | 10+0.1 |
| 1.2 SEE Conservative | Neutral to positive | `[-2, 3]` | 10+0.1 |
| 1.3 Verification | Neutral | `[-2, 2]` | 10+0.1 |
| 2.x Futility | Positive | `[0, 5]` | 10+0.1 |
| 3.x Move Count | Positive | `[0, 5]` | 10+0.1 |
| 4.x Razoring | Small positive | `[0, 3]` | 10+0.1 |

### Tactical Test Positions:

```bash
# Create test file: tactical_suite.epd
cat > tactical_suite.epd << 'EOF'
# Positions where SeaJay currently fails
r1bqkb1r/pppp1ppp/2n2n2/1B2p3/4P3/5N2/PPPP1PPP/RNBQK2R w KQkq - bm Bxc6
r1bqkbnr/pppp1ppp/2n5/4p3/2B1P3/5N2/PPPP1PPP/RNBQK2R b KQkq - am Nf6
# Add more positions from the 171 blunders
EOF
```

---

## Statistics and Monitoring

### Add Pruning Counters:

```cpp
// In SearchData struct
struct PruningStats {
    uint64_t futilityPruned = 0;
    uint64_t extendedFutilityPruned = 0;
    uint64_t moveCountPruned = 0;
    uint64_t razoringCutoffs = 0;
    uint64_t nullMoveVerificationFails = 0;
    
    void reset() {
        futilityPruned = 0;
        extendedFutilityPruned = 0;
        moveCountPruned = 0;
        razoringCutoffs = 0;
        nullMoveVerificationFails = 0;
    }
    
    void report() const {
        std::cerr << "Pruning Stats:\n";
        std::cerr << "  Futility: " << futilityPruned << "\n";
        std::cerr << "  Extended Futility: " << extendedFutilityPruned << "\n";
        std::cerr << "  Move Count: " << moveCountPruned << "\n";
        std::cerr << "  Razoring: " << razoringCutoffs << "\n";
        std::cerr << "  Null Verification Fails: " << nullMoveVerificationFails << "\n";
    }
};
```

### UCI Info String Updates:

```cpp
// Report pruning effectiveness periodically
if (depth >= 5) {
    std::cout << "info string pruning futility " << info.pruningStats.futilityPruned
              << " movecount " << info.pruningStats.moveCountPruned
              << " razor " << info.pruningStats.razoringCutoffs << std::endl;
}
```

---

## Risk Mitigation

### Potential Issues and Solutions:

1. **Over-pruning Tactics**
   - Solution: Conservative margins initially
   - Validate with tactical suite after each change
   - Keep safety conditions (check, mate scores)

2. **Performance Regression**
   - Solution: Incremental changes with SPRT
   - Profile before/after for bottlenecks
   - Ensure compile optimizations enabled

3. **Integration Conflicts**
   - Solution: Test each phase independently
   - Maintain clear phase boundaries
   - Document all dependencies

### Rollback Plan:

```bash
# If a phase fails badly
git checkout HEAD~1  # Revert last commit
git push --force-with-lease  # Update remote

# Or create recovery branch
git checkout -b recovery/pruning-phase-X
git revert [bad-commit]
```

---

## Success Criteria

### Primary Metrics:
1. **Tactical Blunders:** Reduce from 171 to <50 per 29 games
2. **Node Count:** Increase by 15-20% (match peer engines)
3. **Search Depth:** Maintain or improve average depth
4. **Elo:** No regression > 5 Elo

### Secondary Metrics:
1. **Tactical Puzzle Solve Rate:** >80% on standard suite
2. **Time to Depth:** Similar or better than baseline
3. **Move Ordering Efficiency:** Improve from 76.8% to >80%

---

## Implementation Schedule

### Week 1 (Immediate):
- Day 1: Phase 1.1-1.2 (Parameter adjustments)
- Day 2: Phase 1.3 (Verification search)
- Day 3-4: Phase 2.1-2.2 (Basic futility)
- Day 5: Testing and validation

### Week 2:
- Day 1-2: Phase 2.3-2.4 (Futility tuning)
- Day 3-4: Phase 3.1-3.2 (Move count pruning)
- Day 5: Phase 3.3 and testing

### Week 3:
- Day 1: Phase 4 (Razoring)
- Day 2-3: Integration testing
- Day 4-5: SPRT validation

---

## References

### Source Code:
- **Laser:** https://github.com/jeffreyan11/laser-chess-engine/blob/master/src/search.cpp
- **Stockfish:** https://github.com/official-stockfish/Stockfish/blob/master/src/search.cpp
- **Ethereal:** https://github.com/AndyGrant/Ethereal/blob/master/src/search.c

### Papers and Articles:
- "Null Move Pruning" - https://www.chessprogramming.org/Null_Move_Pruning
- "Futility Pruning" - https://www.chessprogramming.org/Futility_Pruning
- "Late Move Reductions" - https://www.chessprogramming.org/Late_Move_Reductions
- "Move Count Based Pruning" - https://www.chessprogramming.org/Movecount_Based_Pruning

### Testing Resources:
- Strategic Test Suite (STS) - https://www.chessprogramming.org/Strategic_Test_Suite
- Bratko-Kopec Test - https://www.chessprogramming.org/Bratko-Kopec_Test
- OpenBench Framework - https://github.com/AndyGrant/OpenBench

---

## Appendix A: Common Pitfalls

1. **Don't prune in check positions**
2. **Don't prune when close to mate scores**
3. **Always validate TT moves before using**
4. **Test with both tactical and positional suites**
5. **Profile after adding new pruning (overhead matters)**

## Appendix B: Debugging Commands

```bash
# Compare node counts
echo -e "position startpos\\ngo depth 10" | ./seajay | grep "nodes"

# Check pruning stats
echo -e "setoption name Debug value true\\nposition startpos\\ngo depth 10" | ./seajay

# Validate no crashes on extreme positions
echo -e "position fen 8/8/8/8/8/8/8/8 w - - 0 1\\ngo depth 10" | ./seajay
```

---

**Document Status:** Complete and ready for implementation  
**Next Action:** Begin Phase 1.1 - Reduce null move static margin