# PST Sign Convention Analysis

## The Problem

We have two places where PST values interact:
1. `PST::value()` - returns PST values for pieces
2. `board.cpp` - updates m_pstScore when pieces move

## Historical State Analysis

### Original Stage 15 (commit 4418fb5) - WORKS ✓
- `PST::value()`: Returns **positive** for both colors
- `board.cpp` for Black:
  - Removing: `m_pstScore += PST::value()` (WRONG - should subtract)
  - Placing: `m_pstScore -= PST::value()` (WRONG - should add)
- **Result**: Two wrongs make a right! Works correctly.

### After aa269a9 "fix" - BROKEN ✗
- `PST::value()`: Returns **positive** for both colors
- `board.cpp` for Black:
  - Removing: `m_pstScore -= PST::value()` (CORRECT)
  - Placing: `m_pstScore += PST::value()` (CORRECT)
- **Result**: Black pieces increase eval when they should decrease it!

### After d75ee06 "fix" - BROKEN DIFFERENTLY ✗
- `PST::value()`: Returns **negative** for Black
- `board.cpp` for Black:
  - Removing: `m_pstScore -= (-value)` = `+value` (becomes addition)
  - Placing: `m_pstScore += (-value)` = `-value` (becomes subtraction)
- **Result**: 290 cp error from double negation

### Our current "fix" - STILL BROKEN ✗
- `PST::value()`: Returns **positive** for both colors (reverted d75ee06)
- `board.cpp` for Black: 
  - Removing: `m_pstScore -= PST::value()` (from aa269a9)
  - Placing: `m_pstScore += PST::value()` (from aa269a9)
- **Result**: Same as "After aa269a9" - Black eval inverted!

## The Correct Solution

### Option 1: Revert to Original (Two Wrongs)
- Keep `PST::value()` returning positive for both
- Revert board.cpp to original wrong signs
- Pro: Matches original working Stage 15
- Con: Architecturally incorrect

### Option 2: Fix PST::value() Properly
- Make `PST::value()` return negative for Black
- Keep board.cpp with correct signs from aa269a9
- Pro: Architecturally correct
- Con: Changes from original Stage 15

### Option 3: Fix board.cpp Properly
- Keep `PST::value()` returning positive for both
- Fix board.cpp to handle Black differently:
  ```cpp
  if (color == BLACK) {
      m_pstScore -= PST::value() // When ADDING Black piece
      m_pstScore += PST::value() // When REMOVING Black piece
  }
  ```

## Recommendation

**Option 2** is the cleanest: Make PST::value() return negative for Black, keep board.cpp signs correct.

This makes the code self-documenting:
- PST values represent contribution to White's evaluation
- Positive = good for White
- Negative = good for Black
- board.cpp just adds/subtracts naturally