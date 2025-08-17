# Stage 14 Bisection Testing - Compilation Fix Tracking

## Purpose
Track compilation fixes needed for Stage 14 phase commits to enable OpenBench bisection testing.

## Testing Strategy
Test each phase sequentially to identify when -350 ELO regression was introduced:
1. Stage 13 → Phase 1
2. Phase 1 → Phase 2  
3. Phase 2 → Phase 3

## Original Phase Commits (with compilation issues)

| Phase | Original Commit | Description | Compilation Status |
|-------|----------------|-------------|-------------------|
| Baseline | `b949c427e811bfb85a7318ca8a228494a47e1d38` | Stage 13 Complete | ✅ Compiles |
| Phase 1 | `e3cd6a8c583e72a19806193b12cffa9ffe06a05c` | Remove compile-time flags | ❌ Compilation errors |
| Phase 2 | `f5b328a74e677d933f9556b706efc4dbea8efe39` | Algorithm improvements | ❓ Not tested |
| Phase 3 | `3a5f4f780d5d0b789a5d86c6436627f65985b4d2` | Performance optimizations | ❓ Not tested |

## Phase 1 Compilation Fixes Applied

**Original Commit:** `e3cd6a8c583e72a19806193b12cffa9ffe06a05c`
**Fixed Commit:** `c8966a678f5a94f9de4fb7074cad52a1f5b2773b`
**Branch:** `bisect/stage14-phase1-fixed`

### Error 1: quiescence_performance.cpp line 123
```
error: invalid initialization of reference of type 'const seajay::search::SearchLimits&' 
from expression of type 'seajay::TranspositionTable*'
```
**Fix Applied:** Added `SearchLimits limits;` before negamax call (line 121)

### Error 2: quiescence_optimized.cpp line 143  
```
error: 'NODE_LIMIT_PER_POSITION' was not declared in this scope
```
**Fix Applied:** Added fallback constant `const uint64_t NODE_LIMIT_PER_POSITION = 10000;`

## Phase 2 Compilation Status

**Original Commit:** `f5b328a74e677d933f9556b706efc4dbea8efe39`
**Status:** ✅ Compiles without fixes
**Branch:** `bisect/stage14-phase2-fixed`

No compilation fixes required - Phase 2 compiles as-is.

## Phase 3 Compilation Status

**Original Commit:** `3a5f4f780d5d0b789a5d86c6436627f65985b4d2`
**Status:** ✅ Compiles without fixes
**Branch:** `bisect/stage14-phase3-fixed`

No compilation fixes required - Phase 3 compiles as-is.

## Fixed Commits for Testing

| Phase/Stage | Commit | Bench Nodes | NPS | Status |
|-------------|--------|-------------|-----|--------|
| **Stage 13 Baseline** | `b949c427e811bfb85a7318ca8a228494a47e1d38` | 19191913 | **915,363** | ✅ Ready |
| **Phase 1** | `c8966a678f5a94f9de4fb7074cad52a1f5b2773b` | 19191913 | **929,929** | ✅ Ready |
| **Phase 2** | `f5b328a74e677d933f9556b706efc4dbea8efe39` | 19191913 | **921,545** | ✅ Ready |
| **Phase 3** | `3a5f4f780d5d0b789a5d86c6436627f65985b4d2` | 19191913 | **929,470** | ✅ Ready |

## 🎯 OpenBench Test Results - Regression Isolated to Phase 3

### Test Results Summary

| Phase | Test vs Stage 13 | ELO Result | Games | Status |
|-------|------------------|------------|-------|--------|
| **Phase 1** | `c8966a678` vs `b949c427` | **+14 ELO** | 372 games | ✅ GOOD - No regression |
| **Phase 2** | `f5b328a74` vs `b949c427` | **+13.90 ± 20.96 ELO** | 300 games (W:83 L:71 D:146) | ✅ GOOD - No regression |
| **Phase 3** | `3a5f4f780` vs `b949c427` | *Testing in progress* | - | ⏳ Awaiting results |

### 🔍 Critical Finding: Phase 3 is the Culprit

**Phase 1 Result:** +14 ELO improvement over Stage 13 baseline
- Removing compile-time flags actually improved performance slightly
- This phase is CLEAN

**Phase 2 Result:** +13.90 ± 20.96 ELO over Stage 13 baseline  
- Algorithm improvements (best move tracking, TT bounds, delta pruning) are working correctly
- Pentanomial: [2, 28, 78, 40, 2] shows stable performance
- This phase is CLEAN

**Phase 3 Expectation:** This is where the -350 ELO catastrophic regression was introduced
- Performance optimizations likely contain the bug
- Awaiting confirmation from OpenBench testing

## 🐛 Root Cause Analysis: Multiple Bugs in Phase 3

### Testing Results Summary
After extensive testing, we identified Phase 3 had MULTIPLE critical bugs:

1. **TT Poisoning Bug (FIXED)**: Storing minus_infinity (-32768) in TT when in check
2. **Fundamental Static Eval Caching Flaw (ROOT CAUSE)**: Using parent's position eval for child position

### The Real Bug: Broken Static Eval Caching

### The Fundamental Flaw in Static Eval Caching

Phase 3 attempted to cache static evaluations to avoid redundant eval calls, but made a **catastrophic conceptual error**:

```cpp
// Phase 3's flawed logic:
// 1. Parent evaluates position A (before move)
// 2. Parent makes capture move to reach position B  
// 3. Parent passes position A's eval to child as "cached"
// 4. Child uses position A's eval for position B!
```

**This is fundamentally wrong!** You cannot use the evaluation of one position as the evaluation of a completely different position after a move has been made.

### Testing Journey to Find the Bug

1. **First Hypothesis**: Negation issue (negamax requires flipping signs)
   - Tested with fix: Still -330 ELO loss
   
2. **Second Hypothesis**: TT poisoning with minus_infinity values
   - Fixed storing -32768 in TT when in check
   - Tested alone: Still -350 ELO loss
   
3. **Combined Testing**: Both negation + TT poisoning fixes
   - Result: Still -350 ELO loss
   
4. **Final Discovery**: The caching itself is broken
   - Parent's eval (before move) != Child's eval (after move)
   - Completely different positions cannot share evaluations!

### The Fix
Remove the entire static eval caching mechanism:
- Don't pass cached eval to children
- Always evaluate fresh at each node
- Accept the performance cost to fix the -350 ELO bug

### NPS Analysis (Not the Problem)
All phases show consistent NPS around 915K-930K, indicating the regression is algorithmic, not performance-related:
- **Stage 13 Baseline:** 915,363 NPS
- **Stage 14 Phase 1:** 929,929 NPS  
- **Stage 14 Phase 2:** 921,545 NPS
- **Stage 14 Phase 3:** 929,470 NPS

## Workflow
1. **Fix compilation issues** in each phase commit
2. **Create bisection branches** with minimal fixes only
3. **Test on OpenBench** to isolate regression
4. **Document results** to identify problematic phase

## Branch Naming Convention
- `bisect/stage14-phase1-fixed` - Phase 1 with compilation fixes
- `bisect/stage14-phase2-fixed` - Phase 2 with compilation fixes  
- `bisect/stage14-phase3-fixed` - Phase 3 with compilation fixes

## Notes
- Only apply **minimal compilation fixes** - no functional changes
- Preserve original functionality as much as possible
- Document all changes made for bisection testing