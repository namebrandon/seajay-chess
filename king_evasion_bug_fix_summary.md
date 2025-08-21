# King Evasion Bug Fix Summary

## Bug Analysis
The bug in SeaJay's move generation causes illegal King moves during check evasion. The King tries to escape check but moves to squares that are still attacked.

## Root Cause
**Location:** `/workspace/src/core/move_generation.cpp:1002` in `generateKingEvasions()`

**Problem:** When checking if a King's destination square is safe using `isSquareAttacked(board, to, them)`, the King is still on the board at its original position. This causes the King to block sliding piece attacks from Rooks, Bishops, and Queens, making unsafe squares incorrectly appear safe.

## How Other Engines Handle This

### Stockfish
Removes the king from occupancy bitboard before checking attacks:
```cpp
Bitboard occ_no_king = occupied ^ square_bb(kingSq);
// Use occ_no_king for slider attack detection
```

### Ethereal & Weiss
Similar approach with modified occupancy passed to attack detection.

## Correct Fix Approaches

### Approach 1: Specialized King Safety Function (Recommended)
Create a new function `isKingMoveSafe()` that:
1. Removes king from occupancy for slider attacks
2. Checks all attack types with modified occupancy
3. Returns true only if destination is truly safe

```cpp
bool MoveGenerator::isKingMoveSafe(const Board& board, Square from, Square to, Color enemyColor) {
    // Remove king from occupancy
    Bitboard occupancyNoKing = board.occupied() ^ squareBB(from);
    
    // Check non-slider attacks normally
    if (board.pieces(enemyColor, PAWN) & getPawnAttacks(to, ~enemyColor)) return false;
    if (board.pieces(enemyColor, KNIGHT) & getKnightAttacks(to)) return false;
    if (board.pieces(enemyColor, KING) & getKingAttacks(to)) return false;
    
    // Check slider attacks with modified occupancy
    Bitboard bishopsQueens = board.pieces(enemyColor, BISHOP) | board.pieces(enemyColor, QUEEN);
    if (bishopsQueens & getBishopAttacks(to, occupancyNoKing)) return false;
    
    Bitboard rooksQueens = board.pieces(enemyColor, ROOK) | board.pieces(enemyColor, QUEEN);
    if (rooksQueens & getRookAttacks(to, occupancyNoKing)) return false;
    
    return true;
}
```

Then in `generateKingEvasions()`:
```cpp
if (isKingMoveSafe(board, kingSquare, to, them)) {
    // Generate move
}
```

### Approach 2: Make/Unmake (Simpler but slower)
Temporarily make the king move and check if it's safe:
```cpp
Board tempBoard = board;
Board::UndoInfo undo;
tempBoard.makeMove(kingMove, undo);

if (!isSquareAttacked(tempBoard, to, them)) {
    // Move is legal, add to list
}

tempBoard.unmakeMove(kingMove, undo);
```

## Test Positions

```cpp
// Position 1: Rook check on file
"4k3/8/8/8/8/8/8/4R3 b - - 0 1"
// Black king on e8 cannot move to d8 or f8 (blocks own escape)

// Position 2: Bishop check on diagonal  
"8/8/8/8/8/2k5/8/B7 w - - 0 1"
// White king on c3 cannot move along a1-h8 diagonal

// Position 3: Queen check
"3q4/8/8/8/8/8/8/4K3 w - - 0 1"
// White king on e1 has limited escapes
```

## Common Pitfalls
1. Forgetting to initialize attack tables in new functions
2. Incorrect pawn attack perspective (defender vs attacker)
3. Not handling all piece types correctly
4. Performance regression from creating temporary boards

## Related Bugs to Check
1. En passant when capturing pawn is pinned
2. Castling through check (appears correct in current code)
3. Discovery checks when moving non-king pieces

## Implementation Status
**Current Status:** Bug identified, fix approach documented
**Next Steps:** 
1. Implement Approach 1 (specialized function) for best performance
2. Add comprehensive test suite
3. Verify with perft and test positions
4. Run SPRT test to ensure no strength regression

## Performance Impact
- Approach 1: Minimal (one XOR operation per king move)
- Approach 2: Higher cost due to make/unmake operations

## Historical Context
This is a classic bug that has appeared in:
- Crafty (fixed in v12.x)
- Fruit 2.0 (fixed in 2.1)
- Many TCEC tournament entries

The bug is subtle because it only manifests when:
1. King is in check
2. King tries to escape along the line of attack
3. The escape square would be safe if king wasn't blocking

## Verification Method
Use perft with specific positions that trigger the bug. Compare results with Stockfish to ensure correctness.