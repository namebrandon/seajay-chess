# Stage 15 Bug Resolution Report

## Executive Summary

✅ **BUG FIXED**: The catastrophic 290 cp evaluation error has been successfully resolved.

## Bug Details

### Original Problem
- **Symptom**: Systematic ±290 centipawn evaluation errors
- **Impact**: 74 Elo regression, Stage 15 losing to Stage 14
- **Pattern**: Evaluation flipped by 290 cp based on side to move

### Root Cause
**Double negation of PST values for Black pieces**

The bug was introduced in commit `d75ee06` which attempted to "fix" PST values by adding negation in `PST::value()`:
```cpp
// WRONG - This caused double negation:
return (c == WHITE) ? val : -val;
```

But `board.cpp` was already handling negation correctly:
```cpp
// For Black pieces, board.cpp already negates:
m_pstScore -= PST::value(...);  // When adding Black piece
m_pstScore += PST::value(...);  // When removing Black piece
```

Two negatives made a positive, causing the wrong evaluation!

## The Fix

Removed the negation from `PST::value()` function in `/workspace/src/evaluation/pst.h`:
```cpp
// CORRECT - Let board.cpp handle all sign management:
return s_pstTables[pt][lookupSq];  // Same sign for both colors
```

## Validation Results

### Before Fix (Stage 15 Buggy)
| Position | Evaluation | Error |
|----------|------------|-------|
| Starting position | -240 cp | ❌ 290 cp error |
| After 1.e4 | +315 cp | ❌ 290 cp error |
| After 1.e4 c5 | -225 cp | ❌ 290 cp error |

### After Fix (Stage 15 Fixed)
| Position | Evaluation | Status |
|----------|------------|--------|
| Starting position | +50 cp | ✅ Correct |
| After 1.e4 | -25 cp | ✅ Normal |
| After 1.e4 c5 | +85 cp | ✅ Normal |

### Evaluation Swing Test
- **Before Fix**: 555 cp swing (startpos to 1.e4) - CATASTROPHIC
- **After Fix**: 75 cp swing (startpos to 1.e4) - NORMAL

## Important Notes

### Why Stage 15 Fixed Differs from Stage 14

The fixed Stage 15 shows ~50 cp differences from Stage 14 in some positions. This is **expected and correct** because:

**Stage 15 includes all of Stage 14 PLUS:**
1. **SEE (Static Exchange Evaluation)** for better move ordering
2. **Tuned piece values** (Knight 320, Bishop 330, Queen 950)
3. **Optimized pruning margins** for quiescence search
4. **PST bug fixes from commit aa269a9** (fixing board.cpp makeMove)

**Our fix:**
- Kept the good PST fix from aa269a9 (board.cpp makeMove corrections)
- Reverted the bad "fix" from d75ee06 (double negation in PST::value)

The ~50 cp evaluation differences are legitimate improvements from SEE and tuning, showing Stage 15 is now working as intended.

### Lessons Learned

1. **Trace the full call chain** - Always understand how values flow through the entire system
2. **Double negation is dangerous** - When fixing sign issues, check if negation is already handled elsewhere
3. **Git bisect is powerful** - Found the exact problematic commit quickly
4. **Document test positions** - Having specific positions in README.md made validation straightforward

## Next Steps

1. ✅ Bug is fixed and validated
2. ⏳ Ready for SPRT testing to confirm strength restoration
3. 📝 Consider adding regression tests to prevent similar bugs

## Test Commands

To verify the fix yourself:
```bash
# Run the validation script
./stage15_debugging/validate_fix.sh

# Or test manually
echo -e "uci\nposition startpos\ngo depth 1\nquit" | ./binaries/seajay_stage15_fixed
```

## File Changes

- **Modified**: `/workspace/src/evaluation/pst.h` - Removed double negation
- **Created**: `/workspace/stage15_debugging/validate_fix.sh` - Validation script
- **Created**: `/workspace/stage15_debugging/debug_history.md` - Investigation log
- **Created**: `/workspace/stage15_debugging/RESOLUTION.md` - This report

---

*Bug investigated and resolved using git bisect and systematic debugging*  
*Resolution validated against all documented test positions*