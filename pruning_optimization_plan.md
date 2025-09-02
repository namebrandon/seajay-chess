# Pruning Optimization Plan

Author: SeaJay Development Team  
Date: 2025-09-02  
Branch: feature/20250902-pruning-optimization  
Status: Active Development

## Executive Summary

SeaJay currently searches 10-20x more nodes than comparable engines at the same depth. Telemetry analysis reveals critical gaps in our pruning implementation:
- Futility pruning stops at depth 4 (should extend to 7-8)
- No reverse futility pruning (static null move) - *[CORRECTED: Was already implemented]*
- No delta pruning in quiescence - *[CORRECTED: Was already implemented]*
- Potentially sub-optimal aspiration windows causing re-searches

This plan outlines a systematic approach to reduce node explosion while maintaining tactical strength.

## Implementation Results (2025-09-02)

### Phase 1: Critical Fixes - COMPLETED
**Commits:** 
- cbaba15: Phase 1 implementation (futility extension, reverse futility, delta pruning)
- dac0030: B1 fix (PV-first-legal)
- 9d4009f: Added diagnostic telemetry for cutoff position tracking

**Test Results:**
- **Baseline:** 332,267 nodes at depth 10 (starting position)
- **After Phase 1:** 332,267 nodes (0% reduction on balanced positions)
- **SPRT Result:** Neutral (as expected for correctness fixes)
- **Key Finding:** Pruning techniques exist but are too conservative

### Root TT Optimization Attempt - REVERTED
**Commits (Problematic):**
- c61adff: Enable root TT store/probe, defer move gen - **CAUSED -238 ELO REGRESSION**
  - Bug: Allowed TT cutoffs before move generation, breaking checkmate/stalemate detection
  - Bug: Allowed early returns at root, breaking iterative deepening

**Fix Commits:**
- 5cc9049: Disallow hard TT cutoffs at ply 0; maintain valid search window
- ee04682: Add root draw check; add fallback to ensure bestMove always set
- **Status:** Fixes appear correct but need SPRT validation

### Hash Size Investigation - COMPLETED
**SPRT Test:** Hash=8MB vs Hash=64MB
- **Result:** -0.41 ± 10.21 ELO (essentially neutral)
- **Conclusion:** Hash size is NOT the primary bottleneck
- **Decision:** Skip TT size changes, focus on actual search optimizations

### Diagnostic Analysis - COMPLETED

**Node Explosion Root Causes Identified:**
1. **TT Performance:** 35.8% hit rate (low but not catastrophic for 16MB)
   - TT move cutoffs: 23.3% (should be 30-40%)
   - Default Hash size too small (16MB vs typical 64-128MB)
   
2. **Move Ordering Breakdown:**
   - Killers: 47.0% (good!)
   - TT moves: 23.3% (low)
   - First captures: 26.6% (reasonable)
   - Quiet moves: 2.3% (normal - quiets rarely cause cutoffs)

3. **Pruning Limitations:**
   - Futility concentrated at depths 1-3 (confirmed by telemetry)
   - Heavy gating prevents deeper pruning
   - Margins too conservative at deeper depths

4. **Search Efficiency:**
   - PVS re-search: 5.3% (acceptable, slightly high)
   - Null move cutoff: 38.5% (acceptable)

### Updated Understanding

The 6-20x node explosion is **multi-factorial**, not a single issue:
- Under-pruning at deeper depths due to restrictive gating
- Insufficient LMR on late quiets
- TT underutilization (but not broken)
- Missing advanced pruning techniques (razoring, probcut)

## Current Telemetry Capabilities

### New SearchData Statistics (B0 Implementation)

#### Pruning Breakdown (`info.pruneBreakdown`)
```cpp
fut_b=[d1-3, d4-6, d7-9, d10+]  // Futility pruning by depth buckets
mcp_b=[d1-3, d4-6, d7-9, d10+]  // Move count pruning by depth buckets
```
- Tracks where pruning is actually happening
- Current data shows fut_b=[306346,14039,0,0] - zero pruning at depth 7+!

#### Aspiration Window Telemetry (`info.aspiration`)
```cpp
asp: att=<attempts> low=<failLow> high=<failHigh>
```
- `attempts`: Total re-search attempts across all iterations
- `failLow/failHigh`: Iterations with aspiration failures
- High values indicate windows too tight

#### Other Key Metrics
```cpp
null: att=<attempts> cut=<cutoffs> cut%=<percentage>  // Null move effectiveness
prune: fut=<total> mcp=<total>                        // Total prunes
illegal: first=<count> total=<count>                  // Pseudo-legal move validation
```

## Available Analysis Tools

### 1. `/workspace/tools/analyze_position.sh`
**Purpose:** Deep analysis of specific positions with detailed telemetry  
**Key Features:**
- Runs fixed-depth searches with SearchStats enabled
- Compares node counts across different configurations
- Captures pruning breakdown telemetry
- Supports custom FEN positions

**Usage for Pruning Analysis:**
```bash
# Analyze with telemetry enabled
./tools/analyze_position.sh --depth 10 --fen "r1bqkbnr/pppp1ppp/2n5/4p3/4P3/5N2/PPPP1PPP/RNBQKB1R w KQkq - 0 1"

# Compare before/after changes
./tools/analyze_position.sh --compare-branches main feature/pruning
```

### 2. `/workspace/tools/eval_compare.sh`
**Purpose:** Evaluation breakdown comparison across reference positions  
**Key Features:**
- Tests three reference FENs by default
- Shows king safety and evaluation components
- Can be customized with --fen1, --fen2, --fen3

**Usage for Pruning Context:**
```bash
# Run with default positions
./tools/eval_compare.sh

# Custom endgame position to test pruning impact
./tools/eval_compare.sh --fen1 "8/5p2/2R2Pk1/5r1p/5P1P/5KP1/8/8 b - - 26 82"
```

### 3. `/workspace/tools/ks_sanity.sh`
**Purpose:** King safety evaluation testing  
**Note:** Less relevant for pruning but useful for ensuring eval stability

## Telemetry Analysis Strategy

### Phase 0: Baseline Measurement
Run telemetry sweep on representative positions:

```bash
# Create test suite
cat > pruning_test.epd << EOF
rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1
r1bqkbnr/pppp1ppp/2n5/4p3/4P3/5N2/PPPP1PPP/RNBQKB1R w KQkq - 0 1
r1b1k2r/pp4pp/3Bpp2/3p4/6q1/8/PQ3PPP/1R2R1K1 w kq - 2 17
8/5p2/2R2Pk1/5r1p/5P1P/5KP1/8/8 b - - 26 82
EOF

# Run analysis with telemetry
for fen in $(cat pruning_test.epd); do
    echo "=== Position: $fen ==="
    echo -e "setoption name SearchStats value true\nposition fen $fen\ngo depth 10\nquit" | ./bin/seajay
done > pruning_baseline.log
```

### Telemetry Interpretation Guide

#### Pattern 1: Futility Under-utilization
```
fut_b=[296508,13203,0,0]  // All pruning at depth ≤6, none at 7+
```
**Action:** Extend futilityMaxDepth from 4 to 7-8

#### Pattern 2: Excessive Aspiration Re-searches
```
asp: att=5 low=3 high=2  // Many re-searches at single depth
```
**Action:** Widen initial window or adjust growth mode

#### Pattern 3: Poor Null Move Efficiency
```
null: att=1000 cut=200 cut%=20.0  // Low cutoff rate
```
**Action:** Adjust nullMoveStaticMargin or R value

## Implementation Status

### Phase 1: Critical Fixes - COMPLETED (2025-09-02)
**Goal:** Address the most glaring pruning gaps
**Status:** Implemented and tested

#### 1.1 Extend Futility Pruning Depth - IMPLEMENTED
**Changes Made:**
- Extended `futilityMaxDepth` from 4 to 7
- Implemented capped margin growth: `futilityMargin = futilityBase * min(depth, 4)`
- Margins: depth 1: 150, 2: 300, 3: 450, 4-7: 600 (capped)

**Actual Impact:** Minimal (<1% node reduction)
**Issue Identified:** Linear scaling made margins too aggressive at deeper depths. Even with capping, the restrictive conditions (non-PV, not in check, moveCount > 1, quiet moves only) severely limit applicability at depths 5-7.

#### 1.2 Enhanced Reverse Futility Pruning - IMPLEMENTED
**Changes Made:**
- Extended depth from 6 to 8
- Implemented progressive margin scaling:
  - Depths 1-3: 85 * depth (aggressive at shallow depths)
  - Depths 4-8: 255 + (depth-3) * 42.5 (conservative growth)
- Margins: d1: 85, d2: 170, d3: 255, d4: 320, d5: 375, d6: 420, d7: 455, d8: 480

**Actual Impact:** Already existed, enhanced for better coverage
**Note:** Static null move pruning was already implemented, we improved the margin formula and extended the depth range.

#### 1.3 Enhanced Delta Pruning in Quiescence - IMPLEMENTED
**Changes Made:**
- **Coarse filter:** More aggressive pre-check
  - General: Queen value (975cp) margin
  - Endgame: 600cp margin (rook + minor)
- **Per-move pruning:** Tighter margins for small captures
  - Minor pieces or less: deltaMargin / 2
  - Major pieces: standard deltaMargin

**Actual Impact:** Delta pruning was already implemented, we made it more aggressive
**Note:** The engine already had sophisticated delta pruning with phase-aware margins (200/100/50cp). We enhanced the coarse filter and made per-move pruning more aggressive for minor captures.

## Lessons Learned

1. **Deferred Move Generation is Dangerous**
   - Must ensure moves are generated before any checkmate/stalemate check
   - Never allow TT cutoffs before verifying legal moves exist
   - Root position requires special handling - never return early

2. **Hash Size Has Limited Impact**
   - Increasing from 8MB to 64MB gave neutral result
   - The 6-20x node explosion has deeper causes than TT capacity

3. **Existing Pruning is Too Conservative**
   - Futility, reverse futility, and delta pruning all exist
   - Problem is restrictive gating and conservative margins
   - Need to make more nodes eligible for pruning

## Current Status (2025-09-02 End of Day)

- **Branch:** feature/20250902-pruning-optimization
- **Latest Commit:** ee04682 (root TT fixes, awaiting SPRT validation)
- **Node Count:** Still ~332k at depth 10 (6-20x too high)
- **Next Focus:** Actual search optimizations, not infrastructure

## Prioritized Next Steps (Updated)

### Priority 1: ~~Quick TT Improvements~~ (SKIP - tested, minimal impact)
1. ~~Increase default Hash size~~ - TESTED: Neutral result
2. **Fix qsearch TT move ordering** - Still worth trying but lower priority
   - Don't let queen promotions override TT move
   - Move TT move to absolute front in qsearch
   - Expected impact: Increase TT cutoff share from 23% to 30%+

### Priority 1 (NEW): Razoring - Simple & Proven
**Start Here** - Low risk, well-understood technique
1. **Razoring (depths 1-2)**
   - Drop to qsearch when `staticEval + margin < alpha` at shallow depths
   - Conservative margins (300cp at d=1, 500cp at d=2)
   - Expected impact: 5-10% node reduction

### Priority 2: Effective Depth Pruning
1. **Use (depth - reduction) for futility decisions**
   - Apply futility at effective depth for LMR moves
   - Makes futility work near leaves where it's most useful
   - Expected impact: 15-25% node reduction on late moves

2. **Extended futility with victim value**
   - Formula: `staticEval + bestVictimValue + margin < alpha`
   - Enables deeper futility when no capture can save position
   - Expected impact: 10-15% additional pruning at depths 4-7

### Priority 3: LMR Calibration
1. **More aggressive late quiet reductions**
   - Increase reduction for moves 15+ with bad history
   - Add history-based reduction adjustments
   - Monitor re-search rate to avoid inflation

### Priority 4: Advanced Techniques (Later)
1. **Probcut (depths 5+)**
   - Shallow search predicts fail-high → cut
   - Only for non-PV nodes with good eval
   - Expected impact: 5-10% at deeper depths

2. **Singular extensions** (opposite of reductions)
   - Extend singular moves that are much better than alternatives
   - Helps with tactical accuracy

### Testing Strategy
- Each change needs SPRT validation
- Test incrementally, not as bundles (learned from Phase 1)
- Use telemetry to verify pruning is actually happening
- Monitor tactical suite to ensure no blindness

### Success Metrics
- Target: Reduce nodes from 332k to <100k at depth 10
- Maintain tactical strength (same bench positions solved)
- Keep PVS re-search rate under 8%
- Improve TT hit rate to 45-55%

### Phase 2: Parameter Tuning (DEFERRED)

#### 2.1 Move Count Pruning Enhancement (Now part of Priority 2)
- Extend MCP to depth 10 with adjusted limits
- Add UCI options for SPSA tuning:
```cpp
MoveCountLimit9, int, 48, 20, 90, 4, 0.002
MoveCountLimit10, int, 54, 25, 100, 5, 0.002
```

#### 2.2 Aspiration Window Tuning
Based on telemetry patterns:
- If `asp.att > 3` frequently at depth ≥8: widen initial window
- Add depth-dependent window sizing
- Consider different growth modes (linear vs exponential)

#### 2.3 Null Move Refinement
- Test R=3 base instead of R=2 for stronger reduction
- Add eval-based R adjustment
- Tune `nullMoveStaticMargin` via SPSA

### Phase 3: Advanced Techniques (1 week)

#### 3.1 Razoring (Low Depths)
```cpp
// Very shallow positions far below alpha
if (depth <= 2 && !isPvNode && !weAreInCheck) {
    int razorMargin = 300 * depth;
    if (staticEval + razorMargin < alpha) {
        // Drop to quiescence
        return quiescence(...);
    }
}
```

#### 3.2 Probcut (High Depths)
```cpp
// Cut nodes where shallow search predicts fail-high
if (depth >= 5 && !isPvNode && abs(beta) < MATE_BOUND) {
    int probcutMargin = 100;
    // Shallow search with raised beta
    score = -negamax(depth - 4, -beta - probcutMargin, -beta);
    if (score >= beta + probcutMargin) {
        return score;
    }
}
```

#### 3.3 History-Based Pruning
- Use history scores more aggressively
- Prune moves with very negative history at shallow depths
- Consider continuation history for pruning decisions

## Testing Protocol

### A. Node Count Validation
For each change, measure node reduction:
```bash
# Before change
echo -e "position startpos\ngo depth 10" | ./bin/seajay | grep nodes

# After change  
echo -e "position startpos\ngo depth 10" | ./bin/seajay | grep nodes

# Target: 50-70% reduction while maintaining tactical strength
```

### B. Tactical Suite Testing
Ensure no tactical blindness:
```bash
# Run tactical test suite
./bin/seajay bench  # Should maintain same solution rate
```

### C. SPRT Testing
Each phase requires OpenBench SPRT validation:
- Phase 1: Test as bundle for synergy
- Phase 2: Individual parameter tuning
- Phase 3: Test each technique separately

## Success Metrics

### Primary Goals
- **Node Reduction:** 50-70% fewer nodes at depth 10
- **Search Efficiency:** Closer to 2-3x nodes vs comparable engines (not 10-20x)
- **ELO Gain:** +20-40 ELO from better time usage

### Telemetry Targets
- `fut_b`: Activity in buckets [0], [1], and [2] (not just [0])
- `asp.att`: Average < 2 re-searches per iteration
- `null cut%`: > 35% cutoff rate
- New pruning counters showing balanced activity

## Risk Mitigation

### Tactical Safety
- Each pruning technique has depth limits
- PV nodes treated conservatively  
- In-check positions never pruned
- Gradual margins to avoid blind spots

### Performance Monitoring
- Track time-to-depth on standard positions
- Monitor tactical suite solve rates
- Use telemetry to detect over-pruning

### Rollback Strategy
- Each technique behind UCI option
- Can disable individually if issues found
- Git commits for each phase allow easy reversion

## Implementation Order

1. **Day 1-2:** Phase 1 Critical Fixes
   - Extend futility depth
   - Add reverse futility
   - Add delta pruning
   - Measure node reduction

2. **Day 3-5:** Phase 2 Parameter Tuning
   - SPSA runs for MCP limits
   - Aspiration window adjustments
   - Null move parameter tuning

3. **Week 2:** Phase 3 Advanced Techniques
   - Implement razoring
   - Test probcut
   - Enhance history pruning

## Appendix: UCI Options for Testing

### New Options to Add
```cpp
// Futility Pruning
option name FutilityMaxDepth type spin default 7 min 0 max 10
option name ReverseFutilityMargin type spin default 85 min 50 max 150

// Delta Pruning  
option name DeltaPruning type check default true
option name DeltaMargin type spin default 975 min 500 max 1500

// Advanced Pruning
option name UseRazoring type check default false
option name UseProbcut type check default false
```

### Telemetry Control
```cpp
option name SearchStats type check default false  // Already exists
option name ShowPruningBreakdown type check default false  // Add for detailed view
```

## Next Session Action Plan

1. **Wait for SPRT results** on root TT fixes (commits 5cc9049, ee04682)
   - If passes: Good, move forward
   - If fails: Debug and fix the root handling

2. **Implement Razoring** (Priority 1)
   - Simple, proven technique
   - Low risk of tactical blindness
   - Quick to test via SPRT

3. **Focus on making existing pruning less conservative**
   - Loosen gating conditions
   - Use effective depth for LMR moves
   - Add victim value to futility

4. **Avoid infrastructure changes**
   - Hash size doesn't help
   - TT is working adequately
   - Focus on actual search logic

## Key Insight

The 6-20x node explosion is not from missing features but from **overly conservative pruning parameters and restrictive gating**. The solution is to make existing techniques more aggressive, not to add more techniques.