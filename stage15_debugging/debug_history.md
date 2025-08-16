# Stage 15 SEE Regression Debug History

## Bug Summary
- **Symptom**: 74 Elo regression (Stage 15 vs Stage 14)
- **Pattern**: Systematic ±290 centipawn evaluation error
- **Hypothesis**: SEE incorrectly integrated into static evaluation

## Debug Session Log

### Session 1: Initial Investigation
**Date**: 2025-08-16
**Status**: Bug isolated, starting git bisect analysis

#### Findings:
1. Consistent 290 cp error pattern across positions
2. Error flips sign based on side to move
3. Magnitude suggests minor piece value involvement
4. SEE appears to be misapplied to static evaluation

#### Next Steps:
- Use git bisect to find introduction commit
- Analyze SEE integration code
- Check for hardcoded values around 290

---

## Git Commit History for Investigation

Key commits to examine:
- `7ecfc35` - Stage 14 FINAL (baseline - working)
- `b99f10b` - Stage 15 Day 1 (SEE implementation start)
- `4418fb5` - Stage 15 SPRT Candidate 1
- `13198cf` - Stage 15 complete pre-bugfix
- `aa269a9` - PST bug fix #1
- `d75ee06` - PST bug fix #2
- `c570c83` - Parameter tuning complete
- `4ec487d` - Current (has bug)

---

## Investigation Results

### Bisect Results:
- **First bad commit**: d75ee06 "fix: CRITICAL - Fix PST value negation for Black pieces"
- **Last good commit**: 70b4b2b (commit before d75ee06)
- The "fix" actually introduced the bug!

### Code Analysis:
The commit d75ee06 added negation to PST::value() for Black pieces:
```cpp
return (c == WHITE) ? val : -val;  // This caused double negation!
```

But board.cpp was already handling the negation correctly:
- For Black pieces: `m_pstScore -= PST::value(...)` when adding
- For Black pieces: `m_pstScore += PST::value(...)` when removing

This created double negation, causing the 290 cp error!

### Root Cause:
**Double negation of PST values for Black pieces**
1. PST::value() was negating values for Black (after d75ee06)
2. board.cpp was also negating when updating m_pstScore for Black
3. Two negatives make a positive = wrong evaluation!

### Fix Applied:
Revert the PST::value() change - it should return the same sign for both colors.
The negation is already handled correctly in board.cpp.

### Validation:
**Tests Passed!** ✅
- Starting position: +50 cp (correct, was -240)
- After 1.e4: -25 cp (correct, was +315)
- After 1.e4 c5: +85 cp (correct, was -225)
- After 1.d4: -25 cp (correct, was +315)

The 290 cp error has been completely eliminated!

---

## Summary

**Bug**: Stage 15 showed systematic ±290 cp evaluation errors
**Root Cause**: Double negation of PST values for Black pieces
- Commit d75ee06 added negation in PST::value() for Black
- But board.cpp was already handling negation correctly
- Two negatives = positive = wrong evaluation

**Fix**: Removed the negation from PST::value()
- Let board.cpp handle all sign management
- PST::value() now returns same sign for both colors

**Impact**: This bug was introduced while trying to fix a different PST bug, showing the importance of understanding the full call chain before making changes.

**Lesson Learned**: When fixing sign issues, trace through ALL code paths to avoid double negation!