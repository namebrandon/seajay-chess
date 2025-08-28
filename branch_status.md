# UCI Score Conversion Regression Investigation
## Date: 2025-08-28

## Problem Statement
SeaJay has a **-40 ELO performance regression** that appeared after implementing UCI-compliant score conversion (White's perspective). This is a critical issue affecting the engine's playing strength.

## Background
The UCI protocol standard requires engines to report scores from White's perspective:
- Positive scores = good for White
- Negative scores = good for Black

SeaJay uses negamax search, which naturally returns scores from the side-to-move perspective. The regression appeared when we added conversion logic to transform these scores to White's perspective for UCI output.

## Investigation Timeline

### Initial Discovery
- **Baseline (good):** Commit 855c4b9 - No UCI conversion, scores in negamax perspective
- **Problem introduced:** Commit 7f646e4 - Added UCI score conversion infrastructure
- **Regression:** -40 to -46 ELO loss in playing strength

### Previous Attempts (Main Branch)
1. **c75a2e4** - Added `rootSideToMove` to fix perspective issue
   - Result: No improvement, regression persisted
   
2. **f95ee73** - Reduced SearchData size from 42KB to 672B using pointers
   - Result: Structural improvement but regression persisted
   
3. **8b48400** - Changed info output frequency from 4096 to 16384 nodes
   - Result: Made it WORSE (-83 ELO total), reverted in 9ec2cab
   
4. **11c44d8** - Removed DEBUG_UCI instrumentation code
   - Result: Minor improvement (~5-10 ELO) but main regression persisted
   
5. **5b11ae8** - Replaced dynamic_cast with virtual method
   - Result: Minor improvement (~3 ELO) but main regression persisted
   
6. **7a67eae** - Fixed evaluation cache invalidation bug
   - Result: No improvement (wasn't the cause)

### Today's Investigation (Clean Branch Approach)

#### Branch: fix/uci-regression-clean
Starting fresh from the problematic commit (7f646e4) to isolate the issue.

1. **e385033** - Disabled ALL UCI conversion (reverted to negamax perspective)
   - Result: **REGRESSION GONE!** Confirms UCI conversion is the culprit
   
2. **abd2bba** - Re-implemented rootSideToMove fix with proper UCI conversion
   - Result: Regression returned (confirms this approach already tried)

#### Current Hypothesis
The regression is caused by a specific part of the UCI conversion logic, not the concept itself. We need to identify which exact operation causes the problem.

## Active Tests (Granular Isolation)

### Test 1: Centipawn Conversion Only
**Branch:** fix/test-centipawn-only  
**Commit:** 9480ac9  
**Changes:**
```cpp
// ENABLED:
if (sideToMove == BLACK) {
    cp = -cp;  // Negate for Black to get White's perspective
}

// DISABLED:
// Mate score conversion
```

**Hypothesis:** If this causes regression, the centipawn negation logic is flawed.

### Test 2: Mate Score Conversion Only
**Branch:** fix/test-mate-only  
**Commit:** 38e78ec  
**Changes:**
```cpp
// ENABLED:
int uciMateIn = (sideToMove == WHITE) ? mateIn : -mateIn;

// DISABLED:
// Centipawn score conversion
```

**Hypothesis:** If this causes regression, the mate score conversion logic is flawed.

## Possible Root Causes

### Theory 1: Wrong Side-to-Move
We might be passing the wrong `sideToMove` value, causing scores to be negated incorrectly.

### Theory 2: Double Negation
The score might already be in White's perspective somewhere, and we're converting it again.

### Theory 3: OpenBench Integration
Although OpenBench is used by world-class engines, there might be something specific about how we're implementing UCI that conflicts with OpenBench's expectations.

### Theory 4: Timing/Frequency Issue
The conversion might be affecting the frequency or timing of info outputs in a way that impacts search behavior.

### Theory 5: Internal Score Corruption
The conversion might be inadvertently modifying internal scores used by the search, not just the display output.

## Next Steps

Based on test results:

1. **If centipawn conversion fails:** 
   - Check if we're using the correct side-to-move
   - Verify scores are actually in negamax perspective before conversion
   - Test if the negation is happening at the right time

2. **If mate score conversion fails:**
   - Check mate score calculation logic
   - Verify mate-in-N calculations are correct
   - Test if mate scores need different handling

3. **If both pass individually but fail together:**
   - Look for interaction effects
   - Check if there's shared state being corrupted
   - Test order of operations

4. **If both fail:**
   - The problem is deeper than just the conversion
   - Check if `sideToMove` parameter is correct throughout
   - Look for side effects in the info building process

## Success Criteria
The regression is fixed when:
1. UCI output shows scores from White's perspective (UCI compliant)
2. No ELO loss compared to baseline (855c4b9)
3. Bench remains at 19191913 nodes

## Important Code Locations
- `/workspace/src/uci/info_builder.cpp` - UCI score conversion logic
- `/workspace/src/search/negamax.cpp` - Where info functions are called
- `/workspace/src/search/types.h` - SearchData structure definition

## Testing Protocol
All tests against baseline commit 855c4b9 using OpenBench SPRT with bounds [-2.00, 2.00] or similar.

---

*This investigation is critical as a 40 ELO loss represents a significant degradation in playing strength. The solution must maintain UCI compliance while recovering full engine strength.*