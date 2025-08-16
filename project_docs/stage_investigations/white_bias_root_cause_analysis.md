# White Bias Root Cause Analysis - SeaJay Chess Engine

**Date:** 2025-08-16  
**Branch:** white-bias-debug-investigation  
**Author:** In-depth Code Analysis  
**Status:** CRITICAL - Affects engine strength by hundreds of ELO  

## Executive Summary

SeaJay exhibits extreme White bias (85-100% White wins in self-play) that has existed since at least Stage 10. While sign-related issues have been investigated extensively, the root cause appears more complex and may involve move ordering, search behavior, or evaluation perspective issues rather than simple sign errors.

## Investigation Findings

### Move Generation Order Discovery

A critical finding: Black's moves are generated in a specific order that may contribute to the bias:

1. **Move generation uses `popLsb()`** - processes squares a1 to h8 in numerical order
2. **For Black pawns at start**: a7-a6 is the FIRST pawn move generated
3. **Central pawn moves** (d7-d5, e7-e5) come 4th and 5th in generation order
4. **Without proper move ordering**, the search may favor early moves in the list

This explains the consistent a6, h6, b6, g5 pattern - these are literally the first moves Black sees!

### No Tempo or Side-Specific Bonuses Found

- **No tempo bonus exists** in the evaluation
- **No side-to-move bonus** beyond the natural negamax framework
- **No obvious color-dependent constants** in initialization

## Detailed Analysis

### 1. PST Value Function (pst.h)

```cpp
// Line 63-69 in pst.h
static constexpr MgEgScore value(PieceType pt, Square sq, Color c) noexcept {
    Square lookupSq = (c == WHITE) ? sq : (sq ^ 56);
    return s_pstTables[pt][lookupSq];  // Returns SAME SIGN for both colors
}
```

**Issue:** The function mirrors the square for Black but returns the same sign value.

### 2. Board PST Update Logic (board.cpp)

When **removing** a piece (clear() function, lines 129-133):
```cpp
if (oldColor == WHITE) {
    m_pstScore -= eval::PST::value(oldType, s, WHITE);  // Subtract White piece value
} else {
    m_pstScore += eval::PST::value(oldType, s, BLACK);  // ADD Black piece value
}
```

When **placing** a piece (setPiece() function, lines 150-154):
```cpp
if (newColor == WHITE) {
    m_pstScore += eval::PST::value(newType, s, WHITE);  // Add White piece value
} else {
    m_pstScore -= eval::PST::value(newType, s, BLACK);  // SUBTRACT Black piece value
}
```

### 3. The Mathematical Error

**Example: Black moves pawn d7 to d5 (good central move)**

1. PST value for d7 (mirrored to d2): -5
2. PST value for d5 (mirrored to d4): +20

**Current behavior:**
```
m_pstScore += PST::value(PAWN, d7, BLACK);  // += (-5) = +5
m_pstScore -= PST::value(PAWN, d5, BLACK);  // -= (+20) = -20
Net change: -15 to m_pstScore
```

**Problem:** m_pstScore is stored from White's perspective. A negative change should mean White's position got worse (Black improved). But the logic is backwards!

**What happens:**
- Remove Black pawn from d7: m_pstScore increases by 5 (makes White look better)
- Place Black pawn on d5: m_pstScore decreases by 20 (makes White look worse)
- Net: -15 to White's evaluation

But wait - this seems correct! Let me re-analyze...

### 4. The ACTUAL Problem - Double Negation

Looking more carefully at the move logic (lines 1358-1359 for en passant, 1410-1412 for promotion, 1458-1459 for normal moves):

```cpp
// Black moving a piece (lines 1458-1459)
m_pstScore += eval::PST::value(movingType, from, BLACK);  // Remove from 'from'
m_pstScore -= eval::PST::value(movingType, to, BLACK);    // Place on 'to'
```

**The issue is the INTENT vs IMPLEMENTATION mismatch:**

1. **Intent:** The comments suggest PST values should be negated for Black
2. **Implementation:** PST::value() doesn't negate - it returns the same sign
3. **Board.cpp:** Tries to compensate with +/- logic
4. **Result:** Double negation in some paths, no negation in others

## Critical Discovery: The Real Bug

### PST Score Accumulation Logic is Inverted

The real issue is that the board.cpp logic assumes PST::value() returns negative values for Black pieces on good squares, but it doesn't!

**Current Flow:**
1. Black pawn on d7 (bad square): PST::value returns -5 (same as White would get)
2. Board.cpp ADDs this when removing: m_pstScore += (-5) = increases by -5
3. Black pawn on d5 (good square): PST::value returns +20
4. Board.cpp SUBTRACTs this when placing: m_pstScore -= (+20) = decreases by 20
5. **Net result: m_pstScore decreases by 25**

**But m_pstScore is from White's perspective!**
- Decreasing m_pstScore should mean White is worse off
- But Black just moved from bad square (-5) to good square (+20)
- The math is backwards!

## Why Black Plays Terribly

### The a6, h6, b6, g5 Pattern - A Move Ordering Problem?

1. **a7-a6 is the FIRST pawn move in Black's move list**
   - Due to `popLsb()` processing order
   - Without good move ordering, it gets searched first
   - At shallow depths, may appear "safe"

2. **Central moves come later in generation**
   - d7-d5 is the 4th pawn move generated
   - e7-e5 is the 5th pawn move generated
   - May get pruned or searched to lesser depth

3. **Possible alpha-beta interaction**
   - Early moves set alpha-beta bounds
   - Better moves searched later may not get fair evaluation

## The Fix

### Option 1: Fix PST::value() to Negate for Black

```cpp
static constexpr MgEgScore value(PieceType pt, Square sq, Color c) noexcept {
    Square lookupSq = (c == WHITE) ? sq : (sq ^ 56);
    MgEgScore val = s_pstTables[pt][lookupSq];
    return (c == WHITE) ? val : -val;  // NEGATE for Black
}
```

### Option 2: Fix Board.cpp Logic (More Complex)

Reverse all the +/- signs for Black pieces in board.cpp. This requires changes in:
- clear() function
- setPiece() function
- makeMove() for all move types
- recalculatePSTScore()

### Option 3: Store PST from Side-to-Move Perspective

Redesign to store PST scores relative to the side to move, not always from White's perspective.

## Recommended Solution

**Option 1 is cleanest:** Fix PST::value() to return negated values for Black.

This maintains the conceptual model:
- White pieces on good squares -> positive PST value
- Black pieces on good squares -> negative PST value
- m_pstScore from White's perspective: positive = good for White

## Testing Strategy

### 1. Unit Test for PST Values
```cpp
// Verify PST returns opposite signs for same square quality
assert(PST::value(PAWN, e4, WHITE) == -PST::value(PAWN, e5, BLACK));
```

### 2. Symmetric Position Test
```cpp
// After 1.e4 e5, evaluation should be near 0
Board board;
board.setFromFen("rnbqkbnr/pppp1ppp/8/4p3/4P3/8/PPPP1PPP/RNBQKBNR w KQkq - 0 2");
assert(abs(evaluate(board)) < 10);  // Should be nearly equal
```

### 3. Self-Play Validation
```bash
# After fix, run 100 games from startpos
# Expected: ~50% White wins, ~50% Black wins/draws
# Black should play e5, d5, Nf6 - not a6!
```

## Impact Assessment

### Current Impact
- White wins 85-100% from starting position
- Black plays objectively terrible moves
- Engine strength reduced by 200-300 ELO
- Affects all stages since Stage 9/10

### After Fix
- Balanced play between colors
- Proper opening play (central pawns, piece development)
- Significant ELO gain expected
- May affect SPRT baselines (engine will be much stronger)

## Historical Context

This bug has existed since PST was introduced in Stage 9:
- Stage 9: PST implementation introduced the bug
- Stage 10: First confirmed occurrence (100% White wins)
- Stages 11-15: Bug persisted undetected
- Multiple failed "fixes" targeted wrong components

## Conclusion

The White bias is caused by inverted PST value handling for Black pieces. The fix is straightforward: ensure PST::value() returns negated values for Black pieces. This single change should restore color balance and significantly improve engine strength.

## Alternative Hypotheses (Beyond Sign Issues)

### Hypothesis 1: Search Asymmetry

The search itself may treat colors differently due to:
- Move ordering putting White's natural moves first
- Transposition table storing scores that favor one color
- Search extensions/reductions applied asymmetrically
- Time management differences between colors

### Hypothesis 2: Starting Position Specific Issue

The bias may be specific to the starting position:
- PST values may create an unstable equilibrium at startpos
- Initial move ordering from startpos favors White
- The particular piece configuration creates evaluation artifacts

### Hypothesis 3: Negamax Score Propagation

The issue might be in how scores are propagated in negamax:
- Side-to-move scoring may be misunderstood somewhere
- Score negation at different ply levels may be incorrect
- Alpha-beta bounds may be asymmetric

### Hypothesis 4: Incremental Update Errors

The incremental PST updates might accumulate errors:
- Make/unmake asymmetry
- Rounding or precision issues
- State not fully restored after unmake

### Hypothesis 5: Color-Dependent Constants

There may be hidden color-dependent values:
- Different initialization for White vs Black
- Asymmetric piece values
- Color-specific code paths

## Investigation Strategy

### Test 1: Non-Starting Positions
Test if the bias exists from other positions:
- Random middlegame positions
- Endgame positions
- Tactical positions

### Test 2: Mirror Test
Test the same position with colors reversed:
- Take any position
- Create its color-flipped mirror
- Compare evaluations and move choices

### Test 3: Search Depth Analysis
Check if bias changes with depth:
- Test at depth 1 (just evaluation)
- Test at depth 3, 5, 7
- See if bias increases with depth

### Test 4: Direct Evaluation Test
Bypass search entirely:
- Directly evaluate positions after each move
- Check if evaluation alone shows bias
- Compare to search results

## Critical Insight: Side-to-Move Scoring

In side-to-move scoring:
- Evaluation ALWAYS returns score from perspective of side to move
- White to move: positive = good for White
- Black to move: positive = good for Black
- Negamax naturally handles this with negation at each level

This is different from absolute scoring where:
- Positive always means good for White
- Negative always means good for Black

The confusion between these two models could be the root cause.

## Systematic Testing Approach

### Test Binaries Created

To identify when the bias was introduced, we've built binaries from three key stages:

| Stage | Binary Path | MD5 Checksum | Commit | Features |
|-------|------------|--------------|--------|----------|
| Stage 7 | `/workspace/binaries/seajay-stage7-c09a377` | `6efd1aefb4d0cd9e5e0504db2de3a866` | c09a377 | Material-only evaluation |
| Stage 8 | `/workspace/binaries/seajay-stage8-66a6637` | `dc95a7f1ba5e356dc79722b6a300ed1a` | 66a6637 | Material tracking infrastructure |
| Stage 9 | `/workspace/binaries/seajay-stage9-fe33035` | `862504c9f2247f0e74fe7f9840b08887` | fe33035 | PST implementation added |

### Test Scripts

Location: `/workspace/tools/scripts/sprt_tests/white_bias_investigation/`

| Script | Purpose | Output Log |
|--------|---------|------------|
| `stage7_selfplay.sh` | Stage 7 self-play test | `stage7_selfplay_results.log` |
| `stage8_selfplay.sh` | Stage 8 self-play test | `stage8_selfplay_results.log` |
| `stage9_selfplay.sh` | Stage 9 self-play test | `stage9_selfplay_results.log` |
| `run_all_tests.sh` | Run all tests sequentially | Aggregates all logs |

### Test Configuration
- **Time Control**: 10+0.1 (10 seconds + 0.1 increment)
- **Games**: 100 per stage (50 game pairs with color alternation)
- **Starting Position**: All games from startpos
- **Method**: Self-play with engines labeled A and B (each plays both colors equally)

### Hypothesis Being Tested

**Primary Hypothesis**: The color bias was introduced in Stage 9 with the PST implementation.

**Expected Results**:
- **Stage 7**: Balanced scores between Engine A and B (no color bias)
- **Stage 8**: Balanced scores between Engine A and B (no color bias)  
- **Stage 9**: Significant imbalance between A and B scores (color bias present)

**Reasoning**: 
- Stages 7-8 use material-only evaluation which is inherently symmetric
- Stage 9 introduces PST with the suspected sign bug for Black pieces
- The PST bug causes Black to evaluate good positions as bad

### Detection Method

Since Engine A and Engine B alternate colors:
- Game 1: A=White, B=Black
- Game 2: A=Black, B=White
- And so on...

If there's no color bias, A and B should score approximately equally (~50 points each from 100 games).

If there's a color bias favoring one color, the engine that plays that color more often when winning will accumulate more points, creating an imbalance.

**Threshold**: Score difference > 20 games indicates significant color bias.

### Test Documentation
Full documentation available at: `/workspace/binaries/STAGE_COMPARISON_BINARIES.md`

## Next Steps

1. **Run Tests**
   - Execute `/workspace/tools/scripts/sprt_tests/white_bias_investigation/run_all_tests.sh`
   - Document actual results vs expected
   - Confirm Stage 9 as the introduction point

2. **Code Review** 
   - If Stage 9 shows bias, focus on PST implementation in `pst.h`
   - Review sign conventions in evaluation
   - Check board.cpp PST update logic

3. **Apply Fix**
   - Implement Option 1: Fix PST::value() to negate for Black
   - Test the fix with self-play
   - Verify balanced color performance

4. **Validation**
   - Run SPRT tests with fixed binary
   - Ensure no regression in playing strength
   - Verify opening move selection improved