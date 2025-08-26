# Pruning Aggressiveness Analysis

**Date:** August 26, 2025  
**Branch:** feature/analysis/20250826-pruning-aggressiveness  
**Parent Branch:** feature/20250825-game-analysis  
**Status:** INVESTIGATION PHASE  

## Problem Statement

From the original game analysis (`seajay_investigation_report.md`), SeaJay shows signs of overly aggressive pruning:
- **20% fewer nodes** searched compared to similar engines at same depth
- **171 huge blunders** in 29 games, suggesting important moves being pruned
- Potential tactical blindness due to aggressive forward pruning

## Current Pruning Mechanisms in SeaJay

### 1. Null Move Pruning (ACTIVE)
- **Location:** `src/search/negamax.cpp` (null move implementation)
- **Parameters:** 
  - `useNullMove`: true (enabled)
  - `nullMoveStaticMargin`: 120 centipawns
  - Reduction: R = 2 + depth/4
- **Potential Issues:**
  - May be too aggressive in tactical positions
  - No verification search implemented
  - Static margin might be too large

### 2. Late Move Reductions (LMR) (ACTIVE - Recently Improved)
- **Location:** `src/search/lmr.cpp`
- **Status:** Recently improved with logarithmic table (+28 Elo)
- **Parameters:**
  - `lmrMinDepth`: 3
  - `lmrMinMoveNumber`: 6 (recently tuned)
  - Uses logarithmic reduction formula
- **Assessment:** Likely not the problem after recent improvements

### 3. Futility Pruning (STATUS UNKNOWN)
- **Need to investigate:** Is futility pruning implemented?
- **Common parameters to check:**
  - Futility margins by depth
  - Extended futility pruning
  - Delta pruning in quiescence

### 4. SEE Pruning in Quiescence (ACTIVE)
- **Location:** Quiescence search
- **Parameters:**
  - `seePruningMode`: Can be off/conservative/aggressive
  - Conservative threshold: -100
  - Aggressive threshold: -50 to -75
- **Current Setting:** Need to verify default

### 5. Razoring (STATUS UNKNOWN)
- **Need to investigate:** Is razoring implemented?
- **Typical implementation:** Reduce depth when eval is far below alpha

### 6. Static Null Move Pruning (POSSIBLY ACTIVE)
- **Parameters:** `nullMoveStaticMargin`: 120
- **Need to verify:** Implementation details

## Investigation Plan

### Phase 1: Audit Current Pruning [2 hours]

1. **Catalog all pruning mechanisms**
   - [ ] Search codebase for pruning implementations
   - [ ] Document each mechanism with:
     - Current parameters/thresholds
     - Conditions for activation
     - Depth/score dependencies

2. **Measure pruning statistics**
   - [ ] Add counters for each pruning type
   - [ ] Run test positions and collect:
     - How often each pruning triggers
     - Nodes saved by each technique
     - Correlation with blunders

### Phase 2: Compare to Reference Engines [2 hours]

1. **Study reference engines**
   - [ ] **Laser:** https://raw.githubusercontent.com/jeffreyan11/laser-chess-engine/refs/heads/master/src/search.cpp
   - [ ] **4ku:** https://raw.githubusercontent.com/kz04px/4ku/refs/heads/master/src/main.cpp
   - [ ] **Publius:** https://raw.githubusercontent.com/nescitus/publius/refs/heads/main/src/search.cpp
   - [ ] **Stashbot:** https://raw.githubusercontent.com/mhouppin/stash-bot/9328141bc001913585fb76e6b38efe640eff2701/src/sources/evaluate.c
   - [ ] Document their pruning parameters
   - [ ] Compare thresholds and conditions
   - [ ] Identify major differences

2. **Create comparison table**
   - [ ] SeaJay vs reference parameters
   - [ ] Highlight aggressive outliers

### Phase 3: Tactical Test Suite [3 hours]

1. **Create test positions**
   - [ ] Tactical puzzles that SeaJay fails
   - [ ] Positions from the 171 blunders
   - [ ] Use STS (Strategic Test Suite) for tactical testing
     - Reference: https://www.chessprogramming.org/Strategic_Test_Suite
   - [ ] Avoid WAC (Win at Chess) - outdated and problematic

2. **Test with different pruning settings**
   - [ ] Baseline (current settings)
   - [ ] Conservative (reduced margins)
   - [ ] Disabled (no pruning)
   - [ ] Measure solve rate and time

### Phase 4: Parameter Tuning [4 hours]

1. **Null Move Pruning**
   - [ ] Test static margin: 60, 90, 120, 150
   - [ ] Test reduction formula: R=2, R=2+depth/4, R=3
   - [ ] Consider verification search for shallow depths

2. **Futility Pruning** (if implemented)
   - [ ] Test futility margins by depth
   - [ ] Consider position type (endgame vs middlegame)

3. **SEE Pruning**
   - [ ] Test thresholds: -50, -75, -100, -150
   - [ ] Consider phase-based adjustments

### Phase 5: Implementation [3 hours]

1. **Apply conservative settings**
   - [ ] Reduce most aggressive parameters by 25-50%
   - [ ] Add safety conditions (e.g., no pruning in tactical positions)

2. **Add pruning statistics to UCI output**
   - [ ] Report pruning rates
   - [ ] Help future debugging

3. **Create UCI options for key parameters**
   - [ ] Expose all tunable parameters via UCI for SPSA testing
   - [ ] Parameters must be accessible at runtime
   - [ ] Include reasonable min/max bounds for SPSA
   - [ ] Facilitate both SPRT and SPSA testing on OpenBench

### Phase 6: Validation [2 hours]

1. **Tactical suite validation**
   - [ ] Ensure no regression in tactical ability
   - [ ] Target: <50 blunders per 29 games (from 171)

2. **Node count comparison**
   - [ ] Should search more nodes (closer to peer engines)
   - [ ] Target: Within 10% of Laser/4ku at same depth

3. **SPRT testing**
   - [ ] Test conservative settings vs current
   - [ ] Bounds: [0, 5] Elo

## Success Metrics

### Primary Goals:
1. **Reduce tactical blunders** from 171 to <50 per 29 games
2. **Increase node counts** to within 10% of peer engines
3. **No Elo regression** (SPRT validated)

### Secondary Goals:
1. **Better tactical puzzle solving** rate
2. **More consistent play** in tactical positions
3. **Clearer pruning statistics** for future tuning

## Risk Assessment

### Risks:
1. **Performance impact** - More nodes = slower search
   - Mitigation: Careful tuning to maintain depth
2. **Elo regression** - Less pruning might hurt positional play
   - Mitigation: SPRT testing before merge
3. **Complexity increase** - More parameters to tune
   - Mitigation: Good documentation and defaults

## Files to Examine

### Core Search:
- `src/search/negamax.cpp` - Main search with pruning
- `src/search/quiescence.cpp` - Quiescence pruning
- `src/search/null_move.cpp` - Null move implementation (if separate)

### Configuration:
- `src/search/types.h` - SearchLimits and SearchData structures
- `src/uci/uci.cpp` - UCI option definitions

### Related:
- `src/search/see.cpp` - Static exchange evaluation
- `src/evaluation/evaluation.cpp` - Eval for futility margins

## Notes from Original Analysis

From `seajay_investigation_report.md`:
> "SeaJay searches 15-20% fewer nodes to reach the same nominal depth. This suggests either superior move ordering (unlikely given the errors) or overly aggressive pruning that misses important variations."

This strongly suggests pruning is too aggressive rather than move ordering being superior (confirmed by our 76% ordering efficiency).

## Implementation Approach

1. **Start conservative** - Reduce aggression first, tune back up
2. **Measure everything** - Add statistics for all pruning
3. **Test tactically** - Ensure we don't miss tactics
4. **Iterate carefully** - Small changes with validation
5. **Commit frequently** - Every significant change should be committed
6. **OpenBench compatibility** - All commit messages MUST include "bench <node-count>" string
   - Example: "Reduce null move margin to 90cp, bench 1234567"
   - OpenBench parses this exact format for regression testing

## Timeline

- Phase 1-2: Day 1 (4 hours) - Audit and comparison
- Phase 3-4: Day 2 (7 hours) - Testing and tuning  
- Phase 5-6: Day 3 (5 hours) - Implementation and validation

Total estimate: 16 hours

## Important Notes

⚠️ **NEVER merge to main without human approval** - All changes must be validated through:
1. Local testing with tactical suites
2. SPRT testing on OpenBench
3. Human review of results and code changes

## Next Steps

1. Begin Phase 1 audit of current pruning mechanisms
2. Add statistical counters to measure pruning impact
3. Create tactical test suite from blunder positions
4. Ensure all parameters are exposed via UCI for SPSA testing