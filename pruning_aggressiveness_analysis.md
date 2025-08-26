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
   - [x] **Laser:** https://raw.githubusercontent.com/jeffreyan11/laser-chess-engine/refs/heads/master/src/search.cpp
   - [ ] **4ku:** https://raw.githubusercontent.com/kz04px/4ku/refs/heads/master/src/main.cpp
   - [ ] **Publius:** https://raw.githubusercontent.com/nescitus/publius/refs/heads/main/src/search.cpp
   - [ ] **Stashbot:** https://raw.githubusercontent.com/mhouppin/stash-bot/9328141bc001913585fb76e6b38efe640eff2701/src/sources/evaluate.c
   - [x] Document Laser pruning parameters (see detailed analysis below)
   - [x] Compare thresholds and conditions
   - [x] Identify major differences

2. **Create comparison table**
   - [x] SeaJay vs Laser parameters (detailed comparison below)
   - [x] Highlight aggressive outliers (see Critical Findings section)

#### SeaJay vs Laser Pruning Comparison (COMPLETED)

| **Pruning Technique** | **SeaJay** | **Laser** | **Analysis** |
|----------------------|------------|-----------|---------------|
| **Null Move Pruning** |
| Conditions | Not in check, has non-pawn material, no consecutive nulls | Not PV, not in check, depth >= 2, eval >= beta, has non-pawn material | ✅ Similar |
| Reduction | `R = 2 + (depth >= 6) + (depth >= 12)` (max 4) | `R = 2 + (32*depth + min(eval-beta, 384))/128` | 🔴 **DIFFERENT** - SeaJay simpler |
| Static Margin | 120cp per depth | 70cp per depth | 🔴 **MORE AGGRESSIVE** - SeaJay 71% higher |
| Verification | None | At depth >= 10 | 🔴 **MISSING** - Could cause tactical errors |
| **Futility Pruning** |
| Implementation | ❌ **NOT IMPLEMENTED** | ✅ Implemented | 🔴 **MAJOR GAP** |
| Formula | N/A | `staticEval <= alpha - 115 - 90*depth` | Missing 115+90*depth margins |
| Max Depth | N/A | 6 plies | SeaJay has no futility pruning |
| **Razoring** |
| Implementation | ❌ **NOT IMPLEMENTED** | ✅ Implemented | 🔴 **MISSING TECHNIQUE** |
| Formula | N/A | `staticEval <= alpha - 300` | Missing 300cp razoring |
| Max Depth | N/A | 2 plies | Could help with shallow tactics |
| **LMR Parameters** |
| Formula | `0.5 + log(depth) * log(moves) / 2.25` | `0.5 + log(depth) * log(moves) / 2.1` | ✅ Very similar |
| Min Depth | 3 | 3 | ✅ Identical |
| Min Move Number | 6 | 1 (but table gives 0 for moves 1-2) | ✅ Effectively similar |
| **Move Count Pruning** |
| Implementation | ❌ **NOT IMPLEMENTED** | ✅ Implemented | 🔴 **MAJOR GAP** |
| Tables | N/A | Two 13-element arrays by eval improving | Missing systematic late move pruning |
| Max Depth | N/A | 12 plies | Could reduce nodes significantly |
| **SEE Pruning (QS)** |
| Standard | `SEE >= 0` | `SEE >= 0` | ✅ Identical |
| Aggressive Mode | `SEE >= -75` | N/A (always 0) | 🔴 **MORE AGGRESSIVE** |
| Futility in QS | `staticEval + 80cp < alpha` | `staticEval + 80cp < alpha` | ✅ Identical |
| **Additional Techniques** |
| ProbCut | ❌ **NOT IMPLEMENTED** | ✅ Implemented | 🔴 **MISSING** |
| History Pruning | ❌ **NOT IMPLEMENTED** | ✅ Implemented (depth <= 2) | 🔴 **MISSING** |
| Reverse Futility | Partial (static null move) | ✅ Full implementation | 🟡 **INCOMPLETE** |

#### **Critical Findings:**

**🔴 MAJOR GAPS (Likely causing the 20% fewer nodes):**
1. **No Futility Pruning** - This is standard in all modern engines
2. **No Move Count Pruning** - Laser prunes moves systematically after 2-79 moves depending on depth
3. **No Razoring** - Missing shallow-depth pruning technique
4. **No ProbCut** - Missing capture pruning technique
5. **No History-based Pruning** - Missing move history pruning

**🔴 AGGRESSIVE SETTINGS (Likely causing tactical errors):**
1. **Static Null Move Margin**: 120cp vs 70cp (71% more aggressive)
2. **No Null Move Verification** - Laser verifies at depth >= 10
3. **SEE Aggressive Mode** - Allows -75cp captures vs Laser's 0cp threshold

#### **Recommended Actions:**

**Priority 1 (Implement Missing Techniques):**
- Add futility pruning with Laser's parameters: `115 + 90*depth`
- Add move count pruning with similar tables to Laser
- Add razoring for depth <= 2 with 300cp margin

**Priority 2 (Conservative Adjustments):**
- Reduce static null move margin from 120cp to 90cp
- Add null move verification at depth >= 10
- Default SEE mode to "conservative" instead of "aggressive"

**Expected Impact:**
- More nodes searched (closing the 20% gap)
- Fewer tactical blunders (target: <50 vs current 171)
- Potential small Elo loss initially, but better tactical play

#### Laser Chess Engine Pruning Analysis (COMPLETED)

**Analyzed from:** https://github.com/jeffreyan11/laser-chess-engine/src/search.cpp

##### 1. Null Move Pruning
- **Conditions:**
  - Not a PV node (`!isPVNode`)
  - Not in check (`!isInCheck`)
  - Depth >= 2
  - Static evaluation >= beta (`staticEval >= beta`)
  - Side has non-pawn material (`b.getNonPawnMaterial(color)`)
- **Reduction formula:** `R = 2 + (32 * depth + min(staticEval - beta, 384)) / 128`
- **Verification search:** Performed at depths >= 10 with same reduction
- **Key insight:** Dynamic reduction based on how far ahead the position is

##### 2. Reverse Futility Pruning (Static Null Move)
- **Conditions:**
  - Not a PV node (`!isPVNode`)
  - Not in check (`!isInCheck`)
  - Depth <= 6
  - `staticEval - 70 * depth >= beta`
  - Side has non-pawn material
- **Margin:** 70 centipawns per depth level
- **Returns:** `staticEval` immediately

##### 3. Razoring
- **Conditions:**
  - Depth <= 2
  - `staticEval <= alpha - RAZOR_MARGIN` (300 centipawns)
- **Implementation:** Quick quiescence search to confirm fail-low
- **Margin:** Fixed 300 centipawn threshold

##### 4. Futility Pruning (Main Search)
- **Conditions:**
  - Move is prunable (quiet, non-hash, not giving check)
  - Not in check
  - `pruneDepth <= 6`
  - `staticEval <= alpha - 115 - 90 * pruneDepth`
- **Formula:** Base margin 115cp + 90cp per depth level
- **Special:** Uses `pruneDepth` which is LMR-adjusted depth

##### 5. Late Move Reductions (LMR)
- **Formula:** `lmrReductions[depth][movesSearched] = (int)(0.5 + log(depth) * log(movesSearched) / 2.1)`
- **Table:** Pre-computed 64x64 table
- **Conditions:** Applied to quiet moves at depth >= 3, movesSearched > 1
- **Adjustments:** Various based on history, killer status, etc.

##### 6. Move Count Pruning (Late Move Pruning)
- **Conditions:** `depth <= 12` and `movesSearched > LMP_MOVE_COUNTS[evalImproving][depth]`
- **Tables:** Two arrays based on whether evaluation is improving:
  ```
  Not improving: {0, 2, 4,  7, 11, 16, 22, 29, 37, 46,  56,  67,  79}
  Improving:     {0, 4, 7, 12, 20, 30, 42, 56, 73, 92, 113, 136, 161}
  ```
- **PV adjustment:** Add `depth` to threshold for PV nodes

##### 7. SEE Pruning (Main Search)
- **Two thresholds:**
  1. `!b.isSEEAbove(color, m, -24 * pruneDepth * pruneDepth)` for depths <= 6
  2. `!b.isSEEAbove(color, m, -100 * depth)` for depths <= 5
- **Conditions:** Applied to prunable moves

##### 8. Quiescence SEE Pruning
- **Standard captures:** Must pass `SEE >= 0`
- **Futility in QS:** If `staticEval < alpha - 80` and `!SEE > 1`, prune
- **QS futility margin:** 80 centipawns

##### 9. History-based Pruning
- **Conditions:** 
  - Move is prunable
  - `pruneDepth <= 2`  
  - Both counter-move and followup history are negative
- **Implementation:** Prunes moves with consistently bad history

##### 10. ProbCut
- **Conditions:** Not PV, not in check, depth >= 6, `staticEval >= beta - 100 - 20*depth`
- **Margin:** `beta + 90`
- **Searches:** Up to 3 capture moves with shallow search

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

## Analysis Summary (August 26, 2025)

### ✅ **Completed:**
- **Phase 2**: Comprehensive analysis of Laser chess engine pruning mechanisms
- **Detailed comparison**: SeaJay vs Laser pruning parameters  
- **Root cause identified**: SeaJay is missing critical pruning techniques that are standard in modern engines

### 🔴 **Key Findings:**

**Primary Issues Causing 20% Fewer Nodes:**
1. **Missing Futility Pruning** - Standard technique with `115 + 90*depth` margins
2. **Missing Move Count Pruning** - Systematic late move pruning (saves many nodes) 
3. **Missing Razoring** - Shallow depth pruning for hopeless positions
4. **Missing ProbCut** - Capture sequence pruning 
5. **Missing History-based Pruning** - Prunes moves with poor history

**Aggressive Settings Causing Tactical Errors:**
1. **Static Null Move Margin**: 120cp vs Laser's 70cp (71% more aggressive)
2. **No Null Move Verification** - Laser double-checks at depth >= 10  
3. **Aggressive SEE Thresholds** - Allows losing captures up to 75cp

### 📋 **Immediate Action Items:**

**Priority 1 - Missing Techniques (High Impact):**
- [ ] Implement futility pruning: `staticEval <= alpha - 115 - 90*depth` for depth <= 6
- [ ] Implement move count pruning with depth-based move limits 
- [ ] Add razoring for depth <= 2 with 300cp threshold

**Priority 2 - Conservative Adjustments (Reduce Tactical Errors):**  
- [ ] Reduce static null move margin from 120cp to 90cp
- [ ] Add null move verification search at depth >= 10
- [ ] Change default SEE mode from "aggressive" to "conservative"

### 🎯 **Expected Outcomes:**
- **Nodes**: Increase by 15-20% (matching peer engines)  
- **Tactical blunders**: Reduce from 171 to <50 per 29 games
- **Playing style**: More solid, fewer obvious tactical errors
- **Elo impact**: Initially small loss, but better tactical reliability

### 📈 **Success Metrics:**
1. Node count within 10% of Laser at same depth
2. <50 tactical blunders per 29 games (vs current 171)
3. SPRT validation showing no significant Elo regression
4. Improved tactical puzzle solving rate

---

## Original Investigation Plan

## Next Steps

1. Begin Phase 1 audit of current pruning mechanisms
2. Add statistical counters to measure pruning impact
3. Create tactical test suite from blunder positions
4. Ensure all parameters are exposed via UCI for SPSA testing