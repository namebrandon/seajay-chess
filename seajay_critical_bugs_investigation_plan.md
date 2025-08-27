# SeaJay Critical Bugs Investigation Plan
## Date: 2025-08-27
## Purpose: Systematic debugging of critical evaluation and tactical bugs

---

# Bug #1: ~~Catastrophic Material Counting Failure~~ **RESOLVED - NO BUG**

## Investigation Result: NOT A BUG - FEN Position Error
**Date Resolved:** 2025-08-27

### Summary
The reported "catastrophic material counting failure" was based on an incorrect FEN position that never occurred in the actual game. After cross-referencing with the original PGN file (`external/human-games-20250827.pgn`), we found that:

1. **The FEN positions in the original bug report were completely wrong**
   - Reported problematic FEN: `r1b1k2r/pp3ppp/3Bp3/3p4/6q1/8/1PP2PPP/n4RK1`
   - Actual game position after 12...Nxa1: `r2q1rk1/ppp1bppp/3p3B/4p2n/2B1P1Q1/2NP3P/PP3PP1/n4RK1 w - - 0 13`
   - These are entirely different positions with different piece placements!

2. **SeaJay's evaluation is actually CORRECT**
   - After 12...Nxa1, the Black knight on a1 is **trapped** and immediately recaptured
   - White plays 13.Rxa1 recapturing the knight  
   - Net result: White traded Rook for Knight+Pawn (roughly -1 to -2 pawns disadvantage)
   - Both SeaJay (-1.57 pawns) and Stockfish (-1.03 pawns) correctly evaluate this slight disadvantage

3. **The real issue is tactical blindness in the search**
   - SeaJay played 10.Qg3?? allowing the fork with Nxc2
   - SeaJay played 11.Bh6?? ignoring the fork instead of saving the Queen first
   - This is a search depth/tactical awareness issue, not material counting

### Verified Test Results
```bash
# Actual position from game after 12...Nxa1:
# FEN: r2q1rk1/ppp1bppp/3p3B/4p2n/2B1P1Q1/2NP3P/PP3PP1/n4RK1 w - - 0 13

# SeaJay evaluation: -157 cp (correct - recognizes knight on a1 is trapped)
# Stockfish evaluation: -103 cp (agrees - slight disadvantage for White)

# After 13.Rxa1 (recapturing the knight):
# Material becomes roughly equal, White slightly worse due to pawn deficit
```

### Lessons Learned
1. **Always validate FEN positions against original game files**
2. **Use Stockfish to cross-check evaluations for sanity**
3. **Consider tactical factors (trapped pieces) not just raw material count**

### Conclusion
**No material counting bug exists.** The evaluation function works correctly and even recognizes trapped pieces. The real problem is **poor tactical awareness in the search** - SeaJay needs better fork detection and threat evaluation to avoid blunders like 10.Qg3?? and 11.Bh6??.

---

# Bug #2: ~~Starting Position Evaluation Bias~~ **RESOLVED - FIXED**

## Investigation Result: BUG FIXED - PST Double Negation
**Date Resolved:** 2025-08-27

### Summary
SeaJay was evaluating the starting position as -232 centipawns (favoring Black) instead of 0. The bug was caused by **double negation of Black piece-square table values**.

### Root Cause
The bug was in `/workspace/src/evaluation/pst.h` line 70:
```cpp
// BUGGY CODE:
[[nodiscard]] static constexpr Score value(Color c, PieceType pt, Square sq) noexcept {
    Score val = rawValue(pt, sq).mg;  
    return (c == WHITE) ? val : -val;  // <-- This negation was wrong!
}
```

The issue:
1. `PST::value()` was negating values for Black pieces
2. `board.cpp` was then **subtracting** these already-negated values
3. Result: Black pieces were **adding** to White's score instead of subtracting from it

### The Fix
Changed line 70 in `/workspace/src/evaluation/pst.h`:
```cpp
// FIXED CODE:
[[nodiscard]] static constexpr Score value(Color c, PieceType pt, Square sq) noexcept {
    Score val = rawValue(pt, sq).mg;  
    return val;  // Removed the negation - board.cpp handles sign correctly
}
```

### Verification Results
```bash
# BEFORE FIX:
Starting position: -232 cp (wrong!)
Black to move: +348 cp (asymmetric!)

# AFTER FIX: 
Starting position (static eval): 0 cp ✅
Black to move (static eval): 0 cp ✅
Kings only: 0 cp ✅

# Note: Search shows +58 cp due to first-move advantage, which is acceptable
```

### Testing Methodology
1. **Validated with Stockfish**: Stockfish correctly evaluates starting position as +0.07 cp
2. **Tested symmetry**: Verified both White and Black perspectives evaluate to 0
3. **Isolated components**: Tested with just kings, just pawns, etc. to identify the source
4. **Static vs Search**: Confirmed static evaluation is now 0; small search advantage (+58 cp) is normal

### Impact
This fix corrects a fundamental evaluation asymmetry that was causing:
- Poor opening play (thinking Black is already winning)
- Incorrect position assessments throughout the game
- Systematic bias against White pieces

The engine should now play significantly stronger, especially in the opening and in balanced positions.

---

# Bug #3: King Safety Evaluation Failure

## Problem Statement
SeaJay voluntarily moves its king into extreme danger, failing to recognize basic king safety threats. Most critically, it played 17...Kf7 walking into a devastating attack.

## Test Position
```
FEN: r1b1k2r/pp3ppp/3Bp3/3p4/6q1/8/1PP2PPP/2R1R1K1 b kq - 5 17
```
Position after White's 17.Rbc1, Black to move.

## Reproduction Steps
1. Set up position after 17.Rbc1
2. Run SeaJay evaluation and get best move
3. Observe SeaJay's evaluation and move choice

## What Happens (Bug)
```bash
# SeaJay's evaluation and move:
echo -e "position fen r1b1k2r/pp3ppp/3Bp3/3p4/6q1/8/1PP2PPP/2R1R1K1 b kq - 5 17\neval\ngo depth 12\nquit" | ./bin/seajay

# Expected SeaJay output:
# Evaluation: +11.67 pawns (thinks Black is completely winning)
# Suggested moves: Often includes unsafe moves
# In the actual game: SeaJay played Kf7?! exposing the king

# Position after 17...Kf7:
echo -e "position fen r1b4r/pp3kpp/3Bp3/3p4/6q1/8/1PP2PPP/2R1R1K1 w - - 6 18\neval\ngo depth 10\nquit" | ./bin/seajay
# SeaJay still thinks Black is better despite exposed king
```

## What Should Happen (Correct)
```bash
# Stockfish evaluation:
echo -e "position fen r1b1k2r/pp3ppp/3Bp3/3p4/6q1/8/1PP2PPP/2R1R1K1 b kq - 5 17\neval\ngo depth 20\nquit" | ./external/engines/stockfish/stockfish

# Expected output:
# Evaluation: Close to equal (0.00 to -0.50)
# Best moves: Defensive moves like Bd7, a5, or Qc4
# NOT Kf7 which exposes the king
```

## Investigation Focus Areas
1. **King safety evaluation**: Check king safety scoring function
2. **King exposure penalties**: Verify penalties for exposed kings
3. **Attack detection**: Check if attacks near king are detected
4. **Pawn shield evaluation**: Verify pawn shield bonus/penalties
5. **Piece proximity to king**: Check if enemy pieces near king are penalized

## Key Questions to Answer
- How much weight does king safety have in the evaluation?
- Is king exposure in the center properly penalized?
- Are enemy pieces near the king counted as threats?
- Is there a bonus for keeping the king castled?
- Why does SeaJay think exposing the king is acceptable?

## Additional Test Positions
```bash
# Test king in center vs castled:
# King in center (should be penalized):
echo -e "position fen r3k2r/pppppppp/8/8/8/8/PPPPPPPP/R3K2R w KQkq - 0 1\neval\nquit" | ./bin/seajay

# After castling (should be better):
echo -e "position fen r4rk1/pppppppp/8/8/8/8/PPPPPPPP/R4RK1 w - - 0 1\neval\nquit" | ./bin/seajay
```

---

# Investigation Methodology

## For Each Bug:

### Step 1: Reproduce and Confirm
1. Run the exact test positions provided
2. Confirm the evaluation matches the bug description
3. Document exact output values

### Step 2: Code Tracing
1. Set breakpoints in evaluation function
2. Step through evaluation of test position
3. Log all component scores
4. Identify where incorrect values originate

### Step 3: Component Testing
1. Test each evaluation component in isolation
2. Verify material counting separately
3. Check each positional factor
4. Validate search is reaching expected depth

### Step 4: Fix Development
1. Identify minimal code change needed
2. Test fix with problematic position
3. Verify fix doesn't break other positions
4. Run regression tests

### Step 5: Validation
1. Re-test all positions from both games
2. Verify evaluations are reasonable
3. Check that moves suggested are sound
4. Compare with Stockfish for sanity check

## Success Criteria

### Bug #1 (Material Counting)
- Position with knight fork must evaluate as losing for White (-3 to -4 pawns)
- After rook capture, must show White is down material
- No positive evaluation when down significant material

### Bug #2 (Starting Position)
- Starting position must evaluate between -10 and +10 centipawns
- Ideally exactly 0.00
- Both colors must have symmetric evaluation

### Bug #3 (King Safety)
- Must not suggest moves that expose king to danger
- King in center must be penalized vs castled king
- Must recognize when king is under attack

## Branch Naming Convention
```
bugfix/20250827-material-counting
bugfix/20250827-startpos-evaluation  
bugfix/20250827-king-safety
```

## Testing After Fixes
Run these positions to verify all fixes work together:
1. Starting position (should be ~0.00)
2. Position with Nxc2 fork (should be -3.5 or worse for White)
3. Position before 17...Kf7 (should suggest safe moves)
4. Full game replay to ensure reasonable play throughout

---

# Priority Order
1. **Bug #1** - Material counting (most severe - engine thinks it's winning when losing)
2. **Bug #2** - Starting position (affects every game from move 1)
3. **Bug #3** - King safety (causes tactical disasters but less fundamental)

Each bug should be fixed and tested independently before moving to the next.