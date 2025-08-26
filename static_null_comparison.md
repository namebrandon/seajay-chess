# Static Null Move Pruning Comparison

## Implementation Comparison Table

| **Engine** | **Depth Limit** | **Margin Formula** | **Additional Checks** | **Key Differences** |
|------------|-----------------|-------------------|----------------------|---------------------|
| **SeaJay** | ≤ 3 | `staticEval - (120 * depth) >= beta` | Material balance pre-check, eval caching | Very conservative depth, complex pre-checks |
| **Laser** | ≤ 6 | `staticEval - (70 * depth) >= beta` | Has non-pawn material | Standard depth, simple margin |
| **Publius** | ≤ 6 | `eval - (135 * depth) > beta` | None beyond basics | Standard depth, aggressive margin |

## Detailed Analysis

### SeaJay (Current Implementation)
```cpp
// Line 330 in negamax.cpp
if (!isPvNode && depth <= 3 && depth > 0 && !weAreInCheck && std::abs(beta.value()) < MATE_BOUND - MAX_PLY) {
    // Complex eval caching logic (lines 335-340)
    // Material balance pre-check (line 346):
    if (board.material().balance(board.sideToMove()).value() - beta.value() > -200) {
        staticEval = board.evaluate();
        eval::Score margin = eval::Score(limits.nullMoveStaticMargin * depth); // 120 (now 90)
        if (staticEval - margin >= beta) {
            return staticEval - margin;  // Returns reduced score
        }
    }
}
```

**Issues:**
1. **Too conservative depth** (3 vs 6)
2. **Extra material balance check** that may prevent pruning
3. **Complex caching logic** that adds overhead

### Laser (Reference Implementation)
```cpp
// Simplified view
if (!isPvNode && !isInCheck && depth <= 6 && hasNonPawnMaterial) {
    if (staticEval - 70 * depth >= beta) {
        return staticEval;
    }
}
```

**Characteristics:**
- Clean, simple implementation
- Moderate margin (70cp per depth)
- Returns full eval (not reduced)

### Publius (Alternative Implementation)
```cpp
// Lines 228-235
if (depth <= 6 && !isInCheck && !isPvNode && !excluded) {
    score = eval - 135 * depth;
    if (score > beta)
        return score;
}
```

**Characteristics:**
- Very aggressive margin (135cp per depth)
- Returns the adjusted score (eval - margin)
- Simplest implementation (no material checks)

## Key Findings

### 1. Depth Limit Issue
- **SeaJay: 3** (TOO LOW - missing 50% of pruning opportunities)
- **Laser & Publius: 6** (standard)

### 2. Margin Comparison
Per depth margins:
- **Publius: 135cp** (most aggressive)
- **SeaJay: 120cp → 90cp** (middle ground after change)
- **Laser: 70cp** (most conservative)

Total margin at depth 6:
- Publius: 810cp
- SeaJay (old): 720cp
- SeaJay (new): 540cp
- Laser: 420cp

### 3. Return Value Differences
- **SeaJay:** Returns `staticEval - margin` (safety reduction)
- **Laser:** Returns `staticEval` (full eval)
- **Publius:** Returns `eval - margin` (the cutoff score)

### 4. Additional Checks
- **SeaJay:** Material balance pre-filter (may be too restrictive)
- **Laser:** Non-pawn material check (zugzwang safety)
- **Publius:** None (simplest, most aggressive)

## Recommendations for SeaJay

### Priority Changes:
1. **Extend depth limit from 3 to 6** (Phase 1.2a - already added to plan)
2. **Remove or relax material balance pre-check** (line 346)
3. **Simplify the implementation** - remove unnecessary caching complexity

### Proposed New Implementation:
```cpp
// Simplified static null move pruning
if (!isPvNode && depth <= 6 && depth > 0 && !weAreInCheck 
    && std::abs(beta.value()) < MATE_BOUND - MAX_PLY
    && board.nonPawnMaterial(board.sideToMove()) > ZUGZWANG_THRESHOLD) {
    
    eval::Score staticEval = board.evaluate();
    eval::Score margin = eval::Score(limits.nullMoveStaticMargin * depth);
    
    if (staticEval - margin >= beta) {
        info.nullMoveStats.staticCutoffs++;
        return staticEval - margin;
    }
}
```

### Why SeaJay Shows No Node Difference with Margin Change:

1. **Depth <= 3 severely limits opportunities** - Most testing is at deeper depths
2. **Material balance pre-check** (line 346) filters out many positions
3. **The condition `material.balance - beta > -200`** may rarely be true when we're behind

The material balance check essentially says "only try static null move if we're not too far behind materially" which defeats the purpose - static null move should trigger when we're AHEAD and can afford to pass.

## Conclusion

SeaJay's static null move implementation is overly conservative in three ways:
1. Depth limit too low (3 vs 6)
2. Material balance pre-check may prevent valid pruning
3. Complex caching adds overhead

The margin change from 120→90 has minimal impact because the pruning rarely triggers due to these restrictions. Phase 1.2a (extending depth to 6) will likely have much more impact than the margin adjustments.