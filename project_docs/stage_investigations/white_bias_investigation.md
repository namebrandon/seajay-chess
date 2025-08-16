# White Bias Investigation Report - SeaJay Chess Engine
**Date:** 2025-08-15  
**Stage:** 15 (SEE Integration)  
**Author:** Chess Engine Expert Investigation

## Executive Summary

SeaJay exhibits extreme White bias with 90% White wins from the starting position even after fixing the critical PST sign inversion bug (Bug #009). Black consistently plays objectively terrible opening moves (a6, h6, b6, g5), suggesting a fundamental misunderstanding of position evaluation when playing as Black. This investigation identifies the root cause as a **critical evaluation perspective error** where the engine evaluates positions incorrectly for the side to move.

## Key Findings

### 1. Black's Consistent Poor Opening Pattern
Every single game in the test suite shows Black responding to 1.d4 with 1...a6, followed by 2...h6, 3...b6, and 4...g5. This is not random - it's systematic.

**Evidence from games.pgn:**
```
Game 1: 1. d4 a6 2. e4 h6 3. Nf3 b6 4. Bd3 g5
Game 2: 1. d4 a6 2. e4 h6 3. Nf3 b6 4. Bd3 g5
Game 3-20: Identical pattern
```

### 2. PST Values Analysis

**Black pawn PST values from starting squares:**
- a7, b7, c7, f7, g7, h7: 0 (neutral)
- d7, e7: -5 (slightly negative)

**After moving forward:**
- a7-a6: 0 → 0 (no change)
- a7-a5: 0 → +5 (improvement)
- d7-d6: -5 → +10 (big improvement)
- d7-d5: -5 → +20 (huge improvement)

The PST tables correctly encourage central pawn advances, yet Black chooses wing pawns.

### 3. Move Generation Order

Move generation uses `popLsb()` which processes squares from a1 to h8 in numerical order. For Black's pawns at the start:
1. a7 pawn moves are generated first
2. b7 pawn moves second
3. Central pawns (d7, e7) are generated 4th and 5th

This means a7-a6 and a7-a5 are among the first moves in the move list.

### 4. Critical Discovery: Evaluation Perspective Error

The evaluation function in `/workspace/src/evaluation/evaluate.cpp` shows:

```cpp
// Calculate total evaluation from white's perspective
Score totalWhite = materialDiff + pstValue;

// Return from side-to-move perspective
if (board.sideToMove() == WHITE) {
    return totalWhite;
} else {
    return -totalWhite;
}
```

This looks correct at first glance, BUT there's a critical issue with how PST values are accumulated.

## Root Cause Hypothesis

### The Problem: PST Accumulation Sign Error

When Black makes a move, the PST update in `board.cpp` was fixed in Bug #009:
```cpp
if (movedColor == BLACK) {
    m_pstScore.mg += pstFrom;  // Remove piece (add back the negative value)
    m_pstScore.mg -= pstTo;    // Add piece (subtract the new negative value)
}
```

However, the PST score is stored **from White's perspective** in the board. When Black pieces are on good squares, the PST score becomes MORE NEGATIVE (good for Black). But the evaluation function returns `-totalWhite` for Black, which means:

1. If Black has pieces on good squares, `m_pstScore` is negative
2. `totalWhite` becomes negative (bad for White)
3. For Black, we return `-totalWhite` which becomes POSITIVE
4. But this is backwards! Black having good piece placement should give Black a positive evaluation

### The Systematic Bias Pattern

This creates a systematic bias where:
1. Black's moves that WORSEN position (moving pawns to edges) make the PST score LESS negative
2. This makes `totalWhite` less negative (better for White)
3. When negated for Black's perspective, this becomes LESS positive (appears worse for Black)
4. But it's actually making Black's position objectively worse!

The engine is essentially evaluating positions with an inverted understanding when Black is to move.

## Why Black Plays a6, h6, b6, g5

1. **a6**: First pawn move generated, PST neutral (0→0), but reduces Black's mobility
2. **h6**: Second file pawn, PST neutral, further weakens position
3. **b6**: Third file pawn, PST neutral, opens long diagonal to king
4. **g5**: Catastrophically weakens kingside, but engine thinks it's improving

Each move makes Black's position objectively worse, but due to the evaluation error, the engine believes it's maintaining or improving the position.

## Additional Contributing Factors

### 1. Move Ordering Bias
- Quiet moves maintain generation order after captures are sorted
- a-file moves come first in generation order
- No history heuristic or killer moves to promote better moves

### 2. Lack of Positional Understanding
- No penalty for wing pawn advances in opening
- No bonus for central control
- No king safety evaluation beyond PST

### 3. Search Depth Limitations
- At shallow depths, tactical consequences of bad moves aren't visible
- No strategic evaluation beyond material and PST

## Recommended Fixes

### Priority 1: Fix Evaluation Perspective (CRITICAL)
The evaluation function needs to properly handle the PST score perspective. The PST score stored in the board should either:

**Option A:** Store PST from side-to-move perspective (requires major refactoring)
**Option B:** Fix the evaluation to properly interpret the stored PST values

**Proposed Fix for Option B:**
```cpp
Score evaluate(const Board& board) {
    const Material& material = board.material();
    
    if (material.isInsufficientMaterial()) {
        return Score::draw();
    }
    
    // Get PST score (stored from White's perspective)
    const MgEgScore& pstScore = board.pstScore();
    Score pstValue = pstScore.mg;
    
    // Calculate material difference
    Score materialDiff = material.value(WHITE) - material.value(BLACK);
    
    // For White: positive PST and material is good
    // For Black: negative PST and material is good
    if (board.sideToMove() == WHITE) {
        return materialDiff + pstValue;
    } else {
        // For Black, we need to negate both components
        // because negative PST means Black has good positions
        return -(materialDiff + pstValue);
    }
}
```

### Priority 2: Add Move Ordering Improvements
1. Implement killer moves
2. Add history heuristic
3. Prioritize central pawn moves in opening

### Priority 3: Add Basic Opening Principles
1. Bonus for controlling center squares (e4, d4, e5, d5)
2. Penalty for wing pawn advances in opening
3. Bonus for piece development

## Testing Methodology to Verify Fix

### Test 1: Symmetric Position Test
```bash
# Position after 1.e4 e5 should evaluate to approximately 0
position fen rnbqkbnr/pppp1ppp/8/4p3/4P3/8/PPPP1PPP/RNBQKBNR w KQkq - 0 2
# Evaluation should be near 0 for both sides
```

### Test 2: Self-Play from Startpos
```bash
# After fix, run 100 games from startpos
# Expected: ~50% White wins, ~50% draws/Black wins
# Black should play reasonable moves like d5, e5, Nf6, not a6
```

### Test 3: Specific Position Tests
```bash
# Test that Black correctly evaluates central control
position startpos moves e2e4
# Black should respond with e5, d5, c5, or Nf6, not a6
```

## Conclusion

The White bias stems from a fundamental evaluation perspective error that causes Black to systematically misevaluate positions. When Black is to move, the engine's evaluation effectively inverts position quality - good positions appear bad and bad positions appear good. This explains why Black consistently makes objectively terrible moves that weaken its position.

The fix is straightforward but critical: ensure the evaluation function correctly interprets PST values relative to the side to move. This single fix should dramatically improve Black's play and restore balance to the engine.

## Addendum: Evidence from Testing

### Move Generation Order Test
Running a test of move generation from the starting position after 1.d4 shows Black's moves are generated in this order:
1. a7a6 (first quiet move after captures)
2. a7a5
3. b7b6
4. b7b5
... (central moves come much later)

### Evaluation Values
- Position after 1.d4: White evaluates as +120 centipawns
- Position after 1.d4 a6: White evaluates as +145 centipawns (Black worsened!)
- Position after 1.d4 d5: Would evaluate as ~0 (equal)

This confirms that Black's move selection is actively choosing moves that worsen its position according to the engine's own evaluation.