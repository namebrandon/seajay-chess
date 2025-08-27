# PST Evaluation Bug Fix Summary

## The Bug
SeaJay was evaluating the starting position as -232 centipawns (favoring Black), when it should be 0 due to the perfect symmetry.

## Root Cause: Double Negation
The bug was caused by double negation of Black piece PST values:

1. **PST::value()** in `pst.h` was negating values for Black pieces: `return (c == WHITE) ? val : -val;`
2. **board.cpp** was then subtracting these already-negated values: `m_pstScore -= eval::PST::value(pt, sq, BLACK);`

This resulted in: `score -= (-val)` = `score += val`, causing Black pieces to ADD to White's evaluation instead of subtracting.

## The Fix
Changed `PST::value()` in `/workspace/src/evaluation/pst.h` (line 70) to NOT negate for Black pieces:

```cpp
// OLD (BUGGY):
return (c == WHITE) ? val : -val;

// NEW (FIXED):  
return val;  // Return raw value, let board.cpp handle perspective
```

## Why This Works
- `board.cpp` already handles the perspective correctly:
  - When adding a Black piece: `m_pstScore -= PST::value(..., BLACK)`
  - When removing a Black piece: `m_pstScore += PST::value(..., BLACK)`
- PST::value() now returns the raw (positive) table value for both colors
- The mirroring for Black pieces (rank flip via XOR 56) is still applied correctly

## Verification
After the fix:
- Starting position: **0 cp** ✓ (was -232 cp)
- Starting position with Black to move: **0 cp** ✓ (was +348 cp from Black's view)
- Position after 1.e4 e5: **0 cp** ✓ (symmetric, as expected)

## Impact
This fix corrects a fundamental evaluation asymmetry that was causing SeaJay to:
- Incorrectly favor Black in the opening
- Misevaluate symmetric positions
- Make poor strategic decisions based on biased positional assessments

## Files Modified
- `/workspace/src/evaluation/pst.h` - Removed negation for Black pieces in PST::value()

The fix is minimal, surgical, and addresses the root cause without requiring changes to the numerous call sites in board.cpp.