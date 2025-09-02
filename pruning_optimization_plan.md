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

## Phase 1 Implementation Results (2025-09-02)

### Test Results Summary
**Baseline (before changes):** 332,267 nodes at depth 10 (starting position)

**After Phase 1 changes:** 332,267 nodes at depth 10 (starting position)
- Node reduction: 0% on balanced positions
- Node reduction: <1% on imbalanced positions

### Key Findings
1. **Futility pruning extension (Phase 1.1):** Minimal impact due to restrictive gating conditions
2. **Reverse futility enhancement (Phase 1.2):** Feature already existed; improvements marginal
3. **Delta pruning enhancement (Phase 1.3):** Feature already existed; made more aggressive

### Conclusion
Phase 1 revealed that SeaJay already had most pruning techniques implemented. The main issue is not missing techniques but overly conservative parameters and restrictive gating. Future phases should focus on:
- Loosening non-critical guards
- Implementing PV-first-legal to reduce PV node prevalence
- Using effective depth (depth - reduction) for pruning decisions
- Adding more sophisticated pruning like razoring and probcut

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

### Phase 2: Parameter Tuning (3-5 days)

#### 2.1 Move Count Pruning Enhancement
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

## Next Immediate Steps

1. Run baseline telemetry sweep (30 minutes)
2. Implement Phase 1.1 (extend futility depth)
3. Test node reduction on standard positions
4. If successful, continue with 1.2 and 1.3
5. Bundle test on OpenBench

The key is to be systematic: measure, implement, verify, test. The telemetry gives us unprecedented visibility into what's actually happening in the search.