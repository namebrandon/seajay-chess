# White Bias Investigation - Critical Finding Addendum

## THE ROOT CAUSE IS FOUND

The issue is NOT in the evaluation function as initially hypothesized. The issue is in how PST values are returned by `PST::value()` for Black pieces.

## The Actual Bug

In `/workspace/src/evaluation/pst.h`, the `PST::value()` function:

```cpp
[[nodiscard]] static constexpr MgEgScore value(PieceType pt, Square sq, Color c) noexcept {
    // For black pieces, mirror the square vertically (rank mirroring)
    Square lookupSq = (c == WHITE) ? sq : (sq ^ 56);
    return s_pstTables[pt][lookupSq];  // BUG: Returns same sign for both colors!
}
```

This function mirrors the square for Black but returns the SAME SIGN value as White would get for the mirrored position.

## The Problem Traced

When Black moves d7 to d5 (objectively good for Black):

1. `PST::value(PAWN, d7, BLACK)` returns -5
   - d7 (square 51) mirrored = square 11 (d2)
   - PST table at d2 = -5
   
2. `PST::value(PAWN, d5, BLACK)` returns +20
   - d5 (square 35) mirrored = square 27 (d4)
   - PST table at d4 = +20

3. In `makeMove()`:
   ```cpp
   m_pstScore -= PST::value(PAWN, d7, BLACK);  // -= (-5) = +5
   m_pstScore += PST::value(PAWN, d5, BLACK);  // += (+20) = +20
   ```
   Net change: +25 to m_pstScore

4. Since m_pstScore is stored from White's perspective, increasing it by 25 makes White's position look BETTER.

5. But Black just made a good move! The position should be WORSE for White!

## The Correct Fix

The `PST::value()` function should return NEGATED values for Black:

```cpp
[[nodiscard]] static constexpr MgEgScore value(PieceType pt, Square sq, Color c) noexcept {
    Square lookupSq = (c == WHITE) ? sq : (sq ^ 56);
    MgEgScore value = s_pstTables[pt][lookupSq];
    return (c == WHITE) ? value : -value;  // NEGATE for Black!
}
```

With this fix:
- `PST::value(PAWN, d7, BLACK)` would return +5 (negated from -5)
- `PST::value(PAWN, d5, BLACK)` would return -20 (negated from +20)
- In makeMove: `m_pstScore -= 5; m_pstScore += (-20)` = net -25
- This correctly makes White's position WORSE when Black advances centrally

## Why This Explains Everything

1. **Why a6 is chosen**: 
   - a7 and a6 both have PST value 0
   - No change to m_pstScore (0 - 0 = 0)
   - Appears neutral, but actually weakens Black's position

2. **Why central moves are avoided**:
   - d7 to d5 increases m_pstScore by 25
   - Engine thinks this helps White
   - So Black avoids it!

3. **Why the pattern is consistent**:
   - All Black's good moves make m_pstScore more positive
   - Engine interprets this as helping White
   - Black systematically avoids all good moves

## Verification

This can be verified by checking any Black move:
- Good moves (central advances) increase m_pstScore (bad for Black in current code)
- Bad moves (wing pawns) keep m_pstScore same or less positive (appears better for Black)

## Impact

This bug affects ALL piece types for Black, not just pawns:
- Knights moving to central squares appear bad for Black
- Bishops on long diagonals appear bad for Black  
- Rooks on 7th rank appear bad for Black
- King castling appears bad for Black

This is why Black plays so terribly - every positionally good move appears to help the opponent!