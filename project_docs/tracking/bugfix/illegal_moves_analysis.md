# Illegal Move Generation Analysis

## Summary of Illegal Moves

Analyzed 8 games where SeaJay generated illegal moves. All illegal moves occurred in the endgame phase.

## Illegal Moves Identified

### Game 1
- **Position:** After 14. Nxd8+
- **Illegal Move:** f7g8 (King move)
- **Context:** King at f7 was in check from d8, tried to move to g8

### Game 2  
- **Position:** After 62. Nd6+
- **Illegal Move:** b7b8 (King move)
- **Context:** King at b7 was in check from d6, tried to move to b8

### Game 3
- **Position:** After 23. Nxg7+
- **Illegal Move:** e6d7 (King move)
- **Context:** King at e6 was in check from g7, tried to move to d7

### Game 4
- **Position:** After 48. e8=Q+
- **Illegal Move:** d7c6 (King move)
- **Context:** King at d7 was in check from newly promoted Queen at e8

### Game 5
- **Position:** After 50. Ne5+
- **Illegal Move:** f7g7 (King move)
- **Context:** King at f7 was in check from e5, tried to move to g7

### Game 6
- **Position:** After 36. Kd2 c1=Q+
- **Illegal Move:** d2e3 (King move)
- **Context:** King at d2 was in check from newly promoted Queen at c1, tried to move to e3

### Game 7
- **Position:** After 31. Rb7+
- **Illegal Move:** b6a5 (King move)
- **Context:** King at b6 was in check from b7, tried to move to a5

### Game 8
- **Position:** After 28. Rxe5+
- **Illegal Move:** e4e3 (King move)
- **Context:** King at e4 was in check from e5, tried to move to e3

## Critical Pattern Identified

### 🔴 ALL 8 ILLEGAL MOVES ARE KING MOVES WHILE IN CHECK

**Common characteristics:**
1. **100% are King moves** - Every single illegal move involves the King
2. **100% occur while in check** - The King is always under attack when the illegal move is made
3. **All are evasion attempts** - The King tries to escape check but moves to an illegal square
4. **Check sources vary:**
   - Knight checks: 3 cases (Games 1, 2, 3)
   - Rook checks: 2 cases (Games 7, 8)  
   - Queen checks: 3 cases (Games 4, 5, 6)

## Specific Bug Pattern

The illegal moves suggest the King is trying to move to squares that are:
1. **Still attacked** by the checking piece or other pieces
2. **Blocked** by own pieces
3. **Invalid** due to moving into check

## Most Likely Bug Location

The bug is almost certainly in the **check evasion move generation** code, specifically:
- When generating King moves to escape check
- The validation of whether the destination square is safe
- Possibly missing validation for:
  - Squares still attacked by the checking piece
  - Squares attacked by other enemy pieces
  - Squares blocked by friendly pieces

## Code Areas to Investigate

1. **Check evasion generator** in move_generation.cpp
2. **King move validation** when in check
3. **Square attack detection** for King destinations
4. **Legal move filtering** after generation

## Test Cases Needed

1. King in check from Knight - verify all escape squares
2. King in check from Rook/Queen on same rank/file
3. King in check from Bishop/Queen on diagonal
4. King in check with limited escape squares
5. King in check near board edges
6. Double check scenarios

## Recommended Fix Strategy

1. **Locate** check evasion code in move_generation.cpp
2. **Add validation** to ensure King's destination square is not attacked
3. **Test** with positions from these games
4. **Add perft tests** for check evasion positions
5. **Verify** no performance regression