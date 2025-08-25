# SeaJay Engine Investigation Report: Performance Gap Analysis

## Executive Summary

Based on analysis of 29 games against 3000+ ELO engines (4ku and Laser), SeaJay exhibits a **critical search depth deficit of 10-12 ply**, searching only 10-11 ply while opponents reach 19-22 ply. This fundamental limitation causes SeaJay to miss tactics 8-12 moves deep, resulting in 171 huge blunders (>5.0 eval drops) and a 0% win rate against these engines.

## 1. Critical Issues Identified by Chess Expert

### Issue #1: SEVERE SEARCH DEPTH DEFICIT [CRITICAL]
- **SeaJay**: 10-11 ply average depth (selective depth ~27-30)
- **Opponents**: 19-22 ply average depth (selective depth 40+)
- **Impact**: Missing all tactics beyond 10 ply horizon

### Issue #2: TACTICAL BLINDNESS [HIGH]
- 171 huge blunders (>5.0 eval drops)
- 315 positions with >2.0 eval drops
- Consistently missing 2-3 move combinations

### Issue #3: OVERLY AGGRESSIVE PRUNING [HIGH]
- **SeaJay**: ~1.03M nodes/move average
- **Opponents**: ~1.27M nodes/move average
- Despite similar NPS, exploring 20% fewer positions

## 2. Implementation Analysis: SeaJay vs Reference Engines

### 2.1 Time Management Comparison

#### SeaJay Current Implementation
```cpp
// src/search/time_management.cpp
- Uses estimatedMovesRemaining calculation (15-40 moves)
- Base time = remaining / estimatedMovesRemaining
- Soft limit = optimum time
- Hard limit = 3x optimum (capped at 50% remaining)
- Early exit at 40% of allocated time in iterative deepening
```

**Critical Finding**: SeaJay stops iterative deepening prematurely at line 1123-1125 in negamax.cpp:
```cpp
if (elapsed * 5 > info.timeLimit * 2) {  // elapsed > 40% of limit
    break;
}
```

#### 4ku Implementation
```cpp
- Allocates ~1/3 of available time per move
- Continues searching until hard time limit
- No early exit based on percentage used
```

#### Laser Implementation
```cpp
- Dynamic time allocation based on PV stability
- Adjusts time based on score changes
- More aggressive time usage in unstable positions
```

**ROOT CAUSE #1**: SeaJay's 40% early exit rule prevents reaching competitive depths. While 4ku and Laser use most of their allocated time, SeaJay stops at 40%.

### 2.2 Search Depth and Extensions

#### SeaJay Current Implementation
- **Max depth**: 64 (theoretical)
- **Extensions**: NONE FOUND (only comment about check extensions in quiescence)
- **Actual depth reached**: 10-11 ply

#### 4ku Implementation
- **Max depth**: 127
- **Extensions**: Check extensions (+1 when in check)
- **Actual depth reached**: 19-22 ply

#### Laser Implementation
- **Max depth**: 129
- **Extensions**: 
  - Check extensions (+1)
  - Singular extensions (hash move significantly better)
- **Actual depth reached**: 19-22 ply

**ROOT CAUSE #2**: SeaJay has NO search extensions implemented in main search, missing critical tactical continuations.

### 2.3 Late Move Reductions (LMR)

#### SeaJay Current Implementation (src/search/lmr.cpp)
```cpp
// Simple linear reduction
reduction = baseReduction + (depth - minDepth) / depthFactor;
if (moveNumber > 8) {
    reduction += (moveNumber - 8) / 4;
}
// Conditions: Not in check, not capture, not giving check
```

#### 4ku Implementation
```cpp
// Logarithmic reduction based on depth and move count
// Adjustments for:
- Improving/non-improving positions
- History heuristic scores
- More sophisticated reduction formula
```

#### Laser Implementation
```cpp
// Uses reduction table: lmrReductions[depth][movesSearched]
// Adjustments for:
- Move history
- Node type (PV/cut)
- Evaluation improvement
- Killer moves exempt
```

**ROOT CAUSE #3**: SeaJay's LMR is too aggressive and simplistic, reducing good moves that competitors would explore.

### 2.4 Move Ordering

#### SeaJay Current Implementation
1. TT move
2. Promotions (MVV-LVA)
3. Captures (MVV-LVA)
4. Killer moves (2 slots)
5. History heuristic
6. Countermoves
7. Quiet moves

#### 4ku Implementation
- Multiple scoring techniques combined
- Dynamic move sorting during search
- Piece-square table evaluations added

#### Laser Implementation
- Hash move primary
- MVV-LVA for captures
- Killer moves
- Counter moves
- **Followup moves** (missing in SeaJay)

**Finding**: SeaJay's move ordering is reasonable but missing some optimizations.

### 2.5 Pruning Techniques

#### SeaJay Current Implementation
- Null move pruning (R=2-4 based on depth)
- Static null move (reverse futility) for depth <= 3
- Quiescence search with SEE pruning
- **Missing**: Futility pruning, razoring, move count pruning

#### Laser Implementation
- Null move pruning
- Reverse futility pruning
- **Razoring** (not in SeaJay)
- **Move count pruning** (not in SeaJay)
- **Futility pruning** (not in SeaJay)
- SEE pruning

**ROOT CAUSE #4**: SeaJay lacks several pruning techniques but more importantly may be too aggressive with existing ones.

## 3. Critical Code Issues Found

### Issue #1: Premature Time Management Exit
**File**: src/search/negamax.cpp:1123-1125
```cpp
// BUG: Stops at 40% of time limit
if (elapsed * 5 > info.timeLimit * 2) {  // elapsed > 40% of limit
    break;
}
```
**Impact**: Prevents reaching competitive depths even with available time.

### Issue #2: No Search Extensions
**File**: src/search/negamax.cpp
```cpp
// MISSING: No check extensions, singular extensions, etc.
// All moves searched with depth - 1, never extended
```
**Impact**: Misses critical tactical sequences in forcing positions.

### Issue #3: Overly Simple LMR
**File**: src/search/lmr.cpp:28-36
```cpp
// Too simplistic reduction formula
int reduction = params.baseReduction;
reduction += (depth - params.minDepth) / params.depthFactor;
```
**Impact**: Reduces moves that should be searched deeper.

### Issue #4: Missing Aspiration Windows at Low Depths
**File**: src/search/negamax.cpp:780-783
```cpp
if (limits.useAspirationWindows && 
    depth >= AspirationConstants::MIN_DEPTH &&  // MIN_DEPTH = 4
    previousScore != eval::Score::zero()) {
```
**Impact**: Not using aspiration windows for depths 1-3 wastes nodes.

## 4. Performance Impact Analysis

### Depth Impact on Tactics
- **10 ply horizon**: Can see 5 moves ahead
- **20 ply horizon**: Can see 10 moves ahead
- **Tactical blindness**: 5+ move combinations invisible to SeaJay

### Node Efficiency
```
SeaJay efficiency = 1.03M nodes/move at 10 ply
Opponent efficiency = 1.27M nodes/move at 20 ply
Relative efficiency = SeaJay uses similar nodes for HALF the depth
```

### Time Usage
```
SeaJay: Uses 40% of allocated time
Opponents: Use 80-90% of allocated time
Wasted potential: 50% of thinking time unused
```

## 5. Remediation Plan

### Phase 1: Critical Time Management Fix [IMMEDIATE]
**Severity**: CRITICAL
**Expected Impact**: +5-8 ply depth, +100-200 ELO

1. **Remove 40% early exit rule** (negamax.cpp:1123-1125)
2. **Implement proper time management check**:
   ```cpp
   // Replace with:
   if (depth >= 6 && elapsed > limits.soft && stable) {
       break;  // Only stop if deep enough and stable
   }
   if (elapsed + predictedNextTime > limits.hard) {
       break;  // Stop if next iteration would exceed hard limit
   }
   ```
3. **Test**: Verify reaching 15+ ply depths

### Phase 2: Implement Search Extensions [HIGH PRIORITY]
**Severity**: HIGH
**Expected Impact**: +2-3 ply effective depth, +50-100 ELO

1. **Check Extensions**:
   ```cpp
   if (inCheck(board)) {
       depth++;  // Extend when in check
   }
   ```
2. **One-Reply Extensions**:
   ```cpp
   if (moves.size() == 1) {
       depth++;  // Forced move, search deeper
   }
   ```
3. **Test each extension separately via OpenBench**

### Phase 3: Refine LMR [MEDIUM PRIORITY]
**Severity**: MEDIUM
**Expected Impact**: +1-2 ply effective depth, +30-50 ELO

1. **Implement logarithmic reduction table**
2. **Add conditions**:
   - Don't reduce killer moves
   - Don't reduce moves with good history
   - Reduce less in PV nodes
3. **Add improving/non-improving logic**

### Phase 4: Advanced Pruning [LOWER PRIORITY]
**Severity**: MEDIUM
**Expected Impact**: +1 ply depth, +20-40 ELO

1. **Futility Pruning**: Skip moves that can't improve position
2. **Razoring**: Drop to quiescence early for hopeless positions
3. **Move Count Pruning**: Limit moves searched at low depths

### Phase 5: Move Ordering Optimization [LOWER PRIORITY]
**Severity**: LOW
**Expected Impact**: Better node efficiency, +10-20 ELO

1. **Add followup history**
2. **Implement piece-square ordering for quiet moves**
3. **Tune history decay and bonus values**

## 6. Testing Strategy

### Immediate Verification (After Phase 1)
1. Run test positions and verify depth reaches 15+ ply
2. Check time usage reaches 70-80% of allocated
3. Run games vs 2000 ELO engine to verify improvement

### OpenBench Testing Protocol
Following feature_guidelines.md:
- Each phase as separate commit with bench number
- Test with appropriate SPRT bounds
- Phase 1: [-5.00, 15.00] (expecting major gain)
- Phase 2-3: [0.00, 8.00] (expecting solid gains)
- Phase 4-5: [0.00, 5.00] (expecting modest gains)

## 7. Conclusion

SeaJay's architecture appears sound with good foundations (TT, null move, quiescence, MVV-LVA, killers, history). The **critical failure is premature search termination** at 40% time usage, preventing competitive search depths. 

**The single line of code at negamax.cpp:1124 is likely costing 200+ ELO.**

Secondary issues (no extensions, simple LMR) compound the problem but fixing time management alone should dramatically improve performance.

## Appendix: Quick Wins Checklist

- [ ] Remove 40% time limit check (1 line change)
- [ ] Add check extension (3 lines)
- [ ] Don't reduce killer moves in LMR (2 lines)
- [ ] Use aspiration windows from depth 2 (1 line)
- [ ] Increase null move reduction at high depths (1 line)

These 5 changes (~8 lines total) could yield +150-250 ELO based on the depth deficit analysis.