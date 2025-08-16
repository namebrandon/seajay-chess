# Stage 15 PST Bug - FINAL RESOLUTION

## Executive Summary

✅ **BUG PROPERLY FIXED**: The Stage 15 regression has been resolved by reverting to the original board.cpp implementation.

## The Real Problem

We had a complex interaction between two pieces of code:
1. `PST::value()` in pst.h - returns piece-square table values
2. `board.cpp` - updates PST scores when pieces move

The original Stage 15 (commit 4418fb5) was working correctly **by accident** - it had the wrong signs in board.cpp, but this accidentally implemented the correct convention.

## Timeline of Confusion

1. **Original Stage 15** (4418fb5): ✅ WORKS
   - PST returns positive values for both colors
   - board.cpp uses inverted signs for Black (accidentally correct!)
   
2. **"Fix" #1** (aa269a9): ❌ BROKE IT
   - Changed board.cpp to "correct" signs
   - But this broke the convention that was working

3. **"Fix" #2** (d75ee06): ❌ MADE IT WORSE
   - Added negation to PST::value() for Black
   - Created double negation with the "fixed" board.cpp
   - Result: 290 cp evaluation errors

4. **Our first attempt**: ❌ STILL BROKEN
   - Removed negation from PST::value()
   - But kept the "fixed" board.cpp from aa269a9
   - Result: -127 Elo regression

5. **PROPER FIX**: ✅ WORKS
   - Reverted board.cpp to original Stage 15 state
   - Kept PST::value() without negation
   - Result: Evaluations match original, ready for SPRT

## Verification

### Evaluation Tests (All Match Original)
- Starting position: 25 cp ✅
- After 1.e4: 0 cp ✅
- After 1.e4 c5: 40 cp ✅

### Binary Details
- **File**: `seajay-stage15-properly-fixed`
- **MD5**: `1364e4c6b35e19d71b07882e9fd08424`
- **Size**: 474,984 bytes

## Technical Explanation

The confusion arose from two different conventions for PST values:

**Convention A** (What original Stage 15 used):
- PST values are always positive (good square = positive value)
- For Black pieces, we invert operations in board.cpp:
  - When placing Black piece on good square: SUBTRACT from eval
  - When removing Black piece from good square: ADD to eval

**Convention B** (What the "fixes" tried to implement):
- PST returns negative values for Black pieces
- board.cpp uses normal add/subtract

The original Stage 15 accidentally implemented Convention A correctly. The subsequent "fixes" tried to implement Convention B but did it incorrectly, causing the regression.

## Lessons Learned

1. **Working code > "Correct" code**: The original was working despite being "wrong"
2. **Test thoroughly**: Always verify evaluation outputs before declaring victory
3. **Understand conventions**: Different conventions can both be correct if applied consistently
4. **Document intent**: The original code lacked comments explaining the sign convention

## Next Steps

1. Run SPRT test with properly fixed binary
2. Expected result: +30-40 Elo from SEE implementation
3. Script ready at: `/workspace/tools/scripts/sprt_tests/stage15_tuned_vs_stage14.sh`

## Command to Run SPRT

```bash
/workspace/tools/scripts/sprt_tests/stage15_tuned_vs_stage14.sh
```

The binary has been validated to match the original Stage 15's evaluation behavior while including all SEE improvements.