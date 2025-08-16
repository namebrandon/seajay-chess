# White Bias Bug - Complete Analysis

## Critical Finding

**The White bias bug exists in BOTH Stage 14 and Stage 15!**

This is not a Stage 15 regression - it's a long-standing bug that predates Stage 15.

## Test Results

### Stage 14 Self-Play (from startpos)
- White wins: 92.3%
- Black wins: 0%
- Draws: 7.7%

### Stage 15 "Properly Fixed" Self-Play (from startpos)
- White wins: 84.6%
- Black wins: 7.7%
- Draws: 7.7%

Both show extreme White bias when playing from starting position.

## Timeline Reconstruction

1. **Stage 14 and earlier**: White bias bug already exists
2. **Stage 15 Original (4418fb5)**: Inherits White bias from Stage 14
3. **aa269a9**: First attempt to fix White bias by changing PST signs in board.cpp
4. **d75ee06**: Second attempt to fix by negating PST values for Black
5. **Our "proper fix"**: Reverted to original Stage 15, which restored the original bias

## The Real Problem

The White bias is NOT caused by the PST handling we've been debugging. It's a deeper issue that exists in the engine's evaluation or search, possibly:

1. **Asymmetric evaluation features** that favor White
2. **Search extensions/reductions** that treat colors differently  
3. **Move ordering** that gives White better moves first
4. **Tempo bonus** that's too large or applied incorrectly
5. **Initialization values** that favor White

## What This Means

### For Stage 15 SPRT Testing

1. **Stage 14 vs Stage 15 tests are still valid** - both have the same bias
2. The bias affects self-play but not engine vs engine matches
3. SPRT with opening books masks the bias by providing diverse positions

### For the PST "Fixes"

The commits aa269a9 and d75ee06 were trying to fix the White bias, but:
- They didn't actually fix the bias (still 90%+ White wins)
- They introduced the 290 cp evaluation error
- They made the engine weaker overall

## Recommendations

### Short Term (for Stage 15 completion)

1. **Use the "properly fixed" version** (MD5: 1364e4c6b35e19d71b07882e9fd08424)
   - It matches Stage 14's behavior (including the bias)
   - It includes SEE improvements
   - SPRT tests against Stage 14 will be valid

2. **Use opening books for testing** to avoid startpos bias affecting results

### Long Term (future fix)

The White bias needs to be fixed, but it's a separate issue from Stage 15:

1. **Create a new bug report** for the White bias
2. **Investigate evaluation asymmetries** 
3. **Check tempo bonus and side-to-move bonus**
4. **Verify color symmetry in all evaluation terms**

## Test Script

To reproduce the White bias in any version:
```bash
$FASTCHESS \
    -engine cmd="$ENGINE" name=White \
    -engine cmd="$ENGINE" name=Black \
    -each tc=1+0.01 \
    -games 2 -rounds 10 -repeat
```

Expected: ~50% wins each
Actual: 85-95% White wins

## Conclusion

The White bias is a **pre-existing bug** from Stage 14 or earlier, not a Stage 15 regression. The PST changes we've been debugging were misguided attempts to fix this bias that actually made things worse.

For Stage 15 SPRT testing, proceed with the properly fixed version that matches Stage 14's behavior.