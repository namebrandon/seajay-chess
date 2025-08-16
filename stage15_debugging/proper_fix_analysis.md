# Proper Fix Analysis

## Root Cause Summary

The confusion comes from different conventions about what PST values mean:

1. **Convention A**: PST values are ALWAYS from White's perspective
   - Positive PST = good square for any piece (White OR Black)
   - When Black piece is on good square, we SUBTRACT from White's eval
   - This requires inverting operations for Black in board.cpp

2. **Convention B**: PST values are color-relative
   - PST returns positive for White pieces on good squares
   - PST returns negative for Black pieces on good squares
   - board.cpp can use simple add/subtract

## What Each Version Did

### Original Stage 15 (WORKS)
- Uses Convention A partially (by accident)
- PST returns positive for both colors
- board.cpp inverts operations for Black (accidentally correct!)

### After aa269a9 (BROKEN)
- Tried to "fix" board.cpp but broke Convention A
- Now neither Convention A nor B is followed

### After d75ee06 (STILL BROKEN)
- Tried to implement Convention B
- But board.cpp was already "fixed" to not follow Convention A
- Result: double negation

## The Solution

We have TWO valid options:

### Option 1: Revert to Original Stage 15
```bash
git checkout 4418fb5 -- src/core/board.cpp
# This restores the "accidentally correct" implementation
```

### Option 2: Implement Convention B Properly
Keep current board.cpp (from aa269a9) but fix PST::value:
```cpp
return (c == WHITE) ? s_pstTables[pt][lookupSq] : -s_pstTables[pt][lookupSq];
```

## Testing Requirements

Any fix MUST produce these evaluations (matching original Stage 15):
- Starting position: ~25 cp
- After 1.e4: ~0 cp  
- After 1.e4 c5: ~40 cp

NOT the broken evaluations we currently have:
- Starting position: 120 cp (WRONG)
- After 1.e4: -120 cp (WRONG)
- After 1.e4 c5: 155 cp (WRONG)