# Stage 15 SEE Regression Debugging

## Critical Bug Identified

We have identified a **catastrophic evaluation bug** in Stage 15 that is causing a 74 Elo regression compared to Stage 14.

## Summary of Findings

### 1. Performance Regression
- **SPRT Results**: Stage 15 vs Stage 14 = **-37.79 ± 32.67 Elo** (FAILING)
- **Expected**: +36 Elo improvement from SEE
- **Actual**: 74 Elo LOSS
- **Win Rate**: 44.58% (should be ~55%)

### 2. Evaluation Bug Discovered

Testing reveals **massive evaluation discrepancies** between Stage 14 and Stage 15:

| Position | Stage 14 (Correct) | Stage 15 (Buggy) | Difference |
|----------|-------------------|------------------|------------|
| Starting position | +50 cp | **-240 cp** | -290 cp error |
| After 1.e4 | +25 cp | **+315 cp** | +290 cp error |
| After 1.e4 c5 | +65 cp | **-225 cp** | -290 cp error |
| After 1.e4 e5 | +50 cp | **-240 cp** | -290 cp error |
| After 1.d4 | +25 cp | **+315 cp** | +290 cp error |
| After 1.d4 d5 | +50 cp | **-240 cp** | -290 cp error |
| After 1.Nf3 | 0 cp | **+290 cp** | +290 cp error |

### 3. Bug Pattern Analysis

The bug shows a **systematic ±290 centipawn error** pattern:
- **After White moves**: Stage 15 adds ~290 cp (overvalues White)
- **After Black moves**: Stage 15 subtracts ~290 cp (undervalues White)
- **Constant magnitude**: Always approximately 290 cp (2.9 pawns)
- **Color dependent**: Flips sign based on who just moved

### 4. Root Cause Hypothesis

Based on the patterns:
1. **SEE is being incorrectly added to static evaluation**
2. **The 290 cp corresponds to roughly a minor piece value** (Knight = 320 cp)
3. **SEE may be double-counting or misapplying piece values**
4. **Color-dependent sign error** in how SEE is integrated

### 5. Additional Evidence

From game analysis:
- **Identical evaluation patterns** across multiple games
- **Color performance gap**: 60.3% as White vs 33.5% as Black
- **Timeout issues**: Stage 15 times out 4x more than Stage 14
- **Deterministic bug**: Same positions always produce same wrong evaluations

## Debugging Strategy

### Phase 1: Isolate the Bug (CURRENT)
✅ **COMPLETE** - Identified systematic 290 cp evaluation error

### Phase 2: Git Bisect (NEXT)
Use commit history to find exactly when bug was introduced:

```bash
# Key commits to test:
7ecfc35 - Stage 14 FINAL (baseline - working)
b99f10b - Stage 15 Day 1 (SEE implementation start)
4418fb5 - Stage 15 SPRT Candidate 1
13198cf - Stage 15 complete pre-bugfix
aa269a9 - PST bug fix #1
d75ee06 - PST bug fix #2
c570c83 - Parameter tuning complete
```

### Phase 3: Code Investigation
Focus areas based on findings:
1. **SEE integration in static evaluation**
2. **How SEE value is calculated and applied**
3. **Sign/color handling in SEE**
4. **Check if SEE is being called inappropriately**

### Phase 4: Fix and Validate
Once root cause found:
1. Apply minimal fix
2. Test evaluation returns to normal
3. Run quick SPRT to confirm strength restored

## Test Scripts Available

All scripts in `/workspace/stage15_debugging/scripts/`:

1. **`simple_eval_test.sh`** - Quick evaluation comparison
2. **`test_multiple_positions.sh`** - Tests various opening positions
3. **`test_evaluation_bug.sh`** - Comprehensive evaluation testing
4. **`debug_stage15_regression.sh`** - Git bisect helper (builds each commit)

## Quick Test Command

To reproduce the bug immediately:
```bash
cd /workspace/stage15_debugging
./scripts/simple_eval_test.sh
```

## Key Code Locations

Based on the bug pattern, investigate:
- `/workspace/src/evaluation/evaluation.cpp` - Where SEE might be integrated
- `/workspace/src/evaluation/see.cpp` - SEE implementation
- `/workspace/src/search/quiescence.h` - SEE usage in search
- `/workspace/src/core/board.cpp` - Evaluation caching

## Next Immediate Steps

1. **Run git bisect** to find exact commit that introduced bug
2. **Search for "290" or "2.9"** in code (might be hardcoded)
3. **Check SEE integration** - is it being added to eval when it shouldn't?
4. **Verify SEE is only for captures** not all positions

## Critical Insight

The **constant 290 cp error** strongly suggests SEE is being misapplied to static evaluation. SEE should ONLY be used for capture move ordering/pruning, NOT for position evaluation!

This is likely a fundamental misunderstanding of how SEE should be integrated - it's a **tactical tool**, not an **evaluation component**.