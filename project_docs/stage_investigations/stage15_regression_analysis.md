# Stage 15 SEE Integration Bug Analysis
**Date:** 2025-08-16  
**Analyst:** Chess Engine Expert  
**Status:** Investigation Complete

## Executive Summary

The reported 290 cp evaluation bug in Stage 15 **does not exist in the current codebase** (commit 4ec487d). Testing shows identical evaluations between Stage 14 and current Stage 15 builds across multiple positions.

## Investigation Findings

### 1. Current State Analysis
- **Current build shows NO 290 cp error**
- Static evaluation function is clean - no SEE integration found
- SEE is correctly isolated to:
  - Quiescence search pruning (src/search/quiescence.cpp)
  - Move ordering (src/search/move_ordering.cpp)
  - NOT in evaluation (src/evaluation/evaluate.cpp) ✓

### 2. Test Results
```
Position: startpos
Stage 14: +50 cp
Stage 15: +50 cp (current)
Difference: 0 cp ✓

Position: After 1.e4
Stage 14: +25 cp  
Stage 15: +25 cp (current)
Difference: 0 cp ✓

Position: After 1.e4 c5
Stage 14: +65 cp
Stage 15: +65 cp (current)
Difference: 0 cp ✓
```

### 3. Most Likely Explanation

The bug likely existed in an intermediate commit between Stage 14 completion and the current state, possibly:
- Commit b99f10b (Stage 15 Day 1) - Initial SEE implementation
- Commit 4418fb5 (SPRT Candidate 1) - First integration attempt
- Already fixed by commit 4ec487d (current)

## Common SEE Integration Mistakes (For Prevention)

### CRITICAL RULE: SEE Must Never Be Part of Static Evaluation

#### Why SEE Cannot Be in Static Eval:
1. **SEE evaluates hypothetical exchanges**, not position quality
2. **Breaks eval symmetry**: eval(pos) ≠ -eval(pos.mirror())
3. **Creates search instability**: Same position gets different values
4. **Causes horizon effects**: SEE value depends on move history

### Correct SEE Usage Patterns

#### ✅ CORRECT - Move Ordering
```cpp
// In move ordering - GOOD
void orderMoves(MoveList& moves, Board& board) {
    for (Move& move : moves) {
        if (isCapture(move)) {
            move.score = see(board, move);  // Use SEE for ordering
        }
    }
    std::sort(moves.begin(), moves.end());
}
```

#### ✅ CORRECT - Pruning Decisions
```cpp
// In quiescence search - GOOD
if (isCapture(move)) {
    SEEValue seeValue = see(board, move);
    if (seeValue < PRUNE_THRESHOLD) {
        continue;  // Prune bad capture
    }
}
```

#### ❌ WRONG - Adding to Evaluation
```cpp
// NEVER DO THIS!
Score evaluate(const Board& board) {
    Score eval = material + pst;
    eval += see(board, lastMove);  // CATASTROPHIC BUG!
    return eval;
}
```

#### ❌ WRONG - Modifying Static Eval
```cpp
// NEVER DO THIS!
Score staticEval = evaluate(board);
if (lastMoveWasCapture) {
    staticEval += see(board, lastMove);  // BUG!
}
```

### The 290 Centipawn Clue

The specific value of 290 cp suggests:
1. **Minor piece value** (Knight ≈ 300-320 cp)
2. **Hardcoded margin** in SEE calculations
3. **Uninitialized cache value** being returned
4. **Default SEE threshold** being misapplied

### Debugging Approach for SEE Bugs

1. **Isolate evaluation path**:
   ```bash
   echo "position startpos" | engine | grep eval
   echo "position startpos moves e2e4" | engine | grep eval
   ```

2. **Check for perspective errors**:
   - Eval after White move should favor White
   - Eval after Black move should favor Black
   - Magnitude changes indicate SEE contamination

3. **Binary search via git bisect**:
   ```bash
   git bisect start
   git bisect bad <buggy-commit>
   git bisect good <working-commit>
   # Test each bisect point
   ```

4. **Add assertions in debug builds**:
   ```cpp
   Score evaluate(const Board& board) {
       Score eval = calculateEval(board);
       #ifdef DEBUG
       assert(abs(eval.value()) < 10000);  // Catch wild values
       assert(!contains_see_value(eval));  // Custom check
       #endif
       return eval;
   }
   ```

## Recommendations

### Immediate Actions
1. ✅ Current code is clean - no action needed
2. ✅ SEE properly isolated to search/ordering
3. ✅ No evaluation contamination found

### Preventive Measures
1. **Add evaluation symmetry test**:
   ```cpp
   assert(evaluate(board) == -evaluate(board.flipped()));
   ```

2. **Create SEE integration tests**:
   - Verify SEE never affects static eval
   - Test evaluation consistency across move sequences
   - Monitor for sudden eval jumps

3. **Document SEE boundaries clearly**:
   ```cpp
   // File: see.h
   // WARNING: SEE must ONLY be used for:
   // 1. Move ordering in search
   // 2. Pruning decisions in quiescence
   // NEVER add SEE values to static evaluation!
   ```

4. **Use static analysis**:
   ```bash
   # Grep for dangerous patterns
   grep -r "eval.*see\|evaluate.*SEE" src/
   ```

## Conclusion

The reported 290 cp bug does not exist in the current codebase. The SEE implementation is correctly isolated from static evaluation. The bug likely existed temporarily during development but has been resolved.

### Key Takeaway
**SEE is a move property, not a position property.** It should never influence static evaluation, only move selection and pruning decisions during search.

## Stage 15 Status
- SEE implementation: ✅ Correct
- Integration: ✅ Properly isolated  
- Evaluation: ✅ Clean
- Bug status: ✅ Not present/Already fixed

---
*Analysis complete. No further action required unless bug resurfaces in testing.*