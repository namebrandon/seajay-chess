# Transposition Table Remediation Plan

## Executive Summary
SeaJay's TT exhibits pathological behavior where hit rates **decrease** from 65% to 34% as search depth increases from 6 to 12. Root causes include missing TT stores on critical pruning paths, broken collision tracking, and overly conservative replacement policy in a single-slot direct-mapped architecture.

## Historical Context

### Current State (as of 2025-09-05)
- **Branch**: `bugfix/nodexp/20250905-tt-optimization`
- **Parent Issue**: Node explosion (5-10x vs comparable engines)
- **TT Performance**: 38% hit rate at depth 10 (vs 65% expected)
- **Architecture**: Single-slot direct-mapped table, 16-byte entries
- **Key Problems**:
  1. Hit rate decreases with depth (opposite of normal)
  2. Zero collisions reported even at 120% hashfull
  3. Missing TT stores on high-frequency pruning paths
  4. Conservative replacement (requires depth > entry.depth+2 for collisions)

### Previous Investigation Results
- **Hash size testing**: Hit rate plateaus at 38% regardless of size (1MB-1GB)
- **Position variance**: Starting position 65% hit rate, tactical middlegame 38%
- **Node reduction achieved**: 2% with improved replacement strategy
- **Collision detection**: Fixed logic but still reports 0 (probe-side not tracked)

### Key Code Locations
- TT Implementation: `src/core/transposition_table.cpp/h`
- Search Integration: `src/search/negamax.cpp`
- Quiescence: `src/search/quiescence.cpp`
- Statistics: `src/search/types.h` (SearchData struct)

## Phase 1: Diagnostic Baseline [VALIDATE PROBLEMS]

### Step 1.1: Add Probe-Side Collision Tracking
**Purpose**: Validate that collisions are actually occurring

1. Modify `TranspositionTable::probe()` to track:
   - Probe attempts
   - Empty slots encountered
   - Non-empty key mismatches (actual collisions)
   - Successful key matches

2. Add counters to TTStats structure:
   ```cpp
   uint64_t probeEmpties = 0;
   uint64_t probeMismatches = 0;  // This is the real collision metric
   ```

3. **Test**: Run with 1MB hash on tactical position, verify probeMismatches > 0

4. **Commit**: "fix: Add probe-side collision tracking for accurate metrics - bench [X]"
5. **Push**: `git push origin bugfix/nodexp/20250905-tt-optimization`

### Step 1.2: Instrument Missing Store Locations
**Purpose**: Quantify impact of missing TT stores

1. Add counters to SearchData:
   ```cpp
   uint64_t nullMoveCutoffs = 0;      // Already exists
   uint64_t nullMoveNoStore = 0;      // NEW: cutoffs without TT store
   uint64_t staticNullReturns = 0;    // NEW: static null early returns
   uint64_t staticNullNoStore = 0;    // NEW: returns without TT store
   ```

2. Instrument null-move and static-null paths (don't add stores yet, just count)

3. **Test**: Run depth 10 search, record ratio of no-store events

4. **Commit**: "feat: Add instrumentation for missing TT store paths - bench [X]"
5. **Push**: `git push origin bugfix/nodexp/20250905-tt-optimization`

### Step 1.3: A/B Testing Baseline
**Purpose**: Establish control measurements

1. Create `test_tt_baseline.sh` script that records:
   - Hit rate by depth (6, 8, 10, 12)
   - Node counts at each depth
   - Collision rates (now properly tracked)
   - Missing store event counts

2. Run baseline 3 times, verify consistent results

3. **Document**: Save baseline metrics in `tt_baseline_metrics.txt`

**GATE**: Review baseline metrics. Confirm:
- [ ] Collisions now detected (probeMismatches > 0)
- [ ] Hit rate decreases with depth
- [ ] Significant null-move/static-null events without stores

## Phase 2: Add Missing TT Stores [HIGH IMPACT FIX]

### Step 2.1: Add TT Store for Null-Move Cutoffs
**Purpose**: Cache null-move pruning decisions

1. In `negamax.cpp`, find null-move cutoff paths:
   - After `if (nullScore >= beta)` block
   - In shallow trust branch (no verification)

2. Add TT store with:
   - Bound: LOWER (fail-high)
   - Move: NO_MOVE
   - Score: beta (with mate adjustment)
   - Depth: current depth

3. **Test**: Run A/B comparison
   - A: Without null-move TT stores
   - B: With null-move TT stores
   - Expect: Higher hit rate, lower node count at depth 10+

4. **Commit**: "feat: Add TT stores for null-move cutoffs - bench [X]"
5. **Push**: `git push origin bugfix/nodexp/20250905-tt-optimization`

### Step 2.2: Add TT Store for Static Null (Reverse Futility)
**Purpose**: Cache static evaluation cutoffs

1. In `negamax.cpp`, find static null early return:
   - Where `return staticEval - margin`

2. Add TT store with:
   - Bound: UPPER (fail-low)
   - Move: NO_MOVE  
   - Score: staticEval - margin
   - Depth: current depth

3. **Test**: Run A/B comparison
   - Measure incremental improvement over 2.1

4. **Commit**: "feat: Add TT stores for static null pruning - bench [X]"
5. **Push**: `git push origin bugfix/nodexp/20250905-tt-optimization`

### Step 2.3: Comprehensive A/B Testing
**Purpose**: Validate cumulative impact

1. Run full comparison:
   - Baseline (no new stores)
   - With null-move stores only
   - With static-null stores only
   - With both

2. Metrics to track:
   - Hit rate trend (should improve or stabilize)
   - Node reduction percentage
   - Time-to-depth

3. **Document**: Results in `tt_stores_impact.txt`

**GATE**: SPRT Testing
- **Prompt**: "Ready for SPRT testing of TT store additions. Expected +10-20 ELO from better pruning cache. Branch: bugfix/nodexp/20250905-tt-optimization"
- Wait for human confirmation before proceeding

## Phase 3: Fix Collision Tracking [DIAGNOSTIC CLARITY]

### Step 3.1: Consolidate Collision Metrics
**Purpose**: Single source of truth for collisions

1. Ensure probe-side and store-side collision tracking are consistent
2. Add UCI info output for collision rate
3. Verify collision rate correlates with hashfull

4. **Test**: Validate collision metrics across hash sizes

5. **Commit**: "fix: Consolidate TT collision metrics - bench [X]"
6. **Push**: `git push origin bugfix/nodexp/20250905-tt-optimization`

## Phase 4: Relax Replacement Policy [QUICK WIN]

### Step 4.1: Instrument Current Replacement Decisions
**Purpose**: Understand what's being preserved vs replaced

1. Add detailed tracking:
   ```cpp
   uint64_t replaceEmpty = 0;
   uint64_t replaceOldGen = 0;
   uint64_t replaceSameKey = 0;
   uint64_t replaceCollision = 0;
   uint64_t skipTooShallow = 0;
   ```

2. **Test**: Run and analyze replacement patterns

3. **Commit**: "feat: Add detailed TT replacement tracking - bench [X]"

### Step 4.2: A/B Test Relaxed Thresholds
**Purpose**: Validate impact before committing to change

1. Create toggleable replacement policies:
   - Conservative: depth > entry.depth + 2 (current)
   - Moderate: depth >= entry.depth (proposed)
   - Relaxed: depth >= entry.depth - 1 (aggressive)

2. **Test** each policy:
   - Hit rate trend
   - Node count
   - Search stability

3. **Document**: Results in `tt_replacement_comparison.txt`

### Step 4.3: Implement Best Policy
**Purpose**: Apply validated improvement

1. Based on A/B results, implement winning policy
2. Remove experimental toggles

3. **Commit**: "feat: Optimize TT replacement policy based on testing - bench [X]"
4. **Push**: `git push origin bugfix/nodexp/20250905-tt-optimization`

**GATE**: SPRT Testing
- **Prompt**: "Ready for SPRT testing of complete TT remediation. Includes: missing stores, collision fixes, relaxed replacement. Expected +20-30 ELO. Branch: bugfix/nodexp/20250905-tt-optimization"

## Phase 5: Validate Success Metrics

### Step 5.1: Final Comparison
1. Run comprehensive tests vs original baseline:
   - Hit rate by depth (should increase or stabilize)
   - Collision detection (should show realistic numbers)
   - Node reduction (target: -20% or better)

2. **Document**: Final results in `feature_status_tt_complete.md`

### Step 5.2: Merge Preparation
1. Squash experimental commits if needed
2. Update main documentation
3. Prepare PR description with:
   - Problem statement
   - Solutions implemented
   - Measured improvements
   - SPRT results

## Success Criteria

| Metric | Baseline | Target | Acceptable |
|--------|----------|--------|------------|
| Hit Rate @ depth 10 | 38% | 55% | 48% |
| Hit Rate Trend | Decreasing | Increasing | Stable |
| Collisions Detected | 0% | 5-10% | >0% |
| Node Reduction | 0% | -20% | -10% |
| SPRT Result | N/A | +20 ELO | +10 ELO |

## Risk Mitigation

1. **After each commit**: Run bench, verify it hasn't changed unexpectedly
2. **Before each push**: Run test suite if available
3. **A/B testing**: Always compare against stable baseline
4. **Reversion plan**: Tag baseline commit for easy rollback
5. **Incremental changes**: Small commits that can be bisected

## Future Work (Post-Remediation)

1. **2-way or 4-way set-associative** implementation
2. **Entry quality tracking** (cutoff success rate)
3. **Separate qsearch TT** to prevent pollution
4. **LRU or pseudo-LRU** replacement
5. **Prefetch optimization** for modern CPUs

## Notes for Implementation

- All bench counts must be included in commit messages
- Push after each meaningful change for backup
- Request SPRT testing at marked gates
- Keep detailed measurements for future reference
- This plan addresses root causes, not just symptoms