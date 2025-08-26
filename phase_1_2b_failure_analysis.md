# Phase 1.2b Catastrophic Failure Analysis

## What Happened
Phase 1.2b resulted in a catastrophic -1107 Elo loss, winning 0 games out of 294.

## The Mistake
I removed the material balance check but in doing so, I also changed the control flow incorrectly:

### Original Code Structure:
```cpp
if (staticEval == eval::Score::zero()) {
    if (material_balance_check) {
        staticEval = board.evaluate();
        // check for pruning
    }
}
```

### My Broken Change:
```cpp
if (staticEval == eval::Score::zero()) {
    staticEval = board.evaluate();  // ALWAYS evaluating
}
// ALWAYS checking for pruning even with cached zero eval
```

## The Problem
1. I unconditionally evaluated even when we had cached eval
2. I moved the pruning check outside the conditional
3. This caused pruning to trigger incorrectly with zero or invalid evals

## Correct Fix (for future)
The material balance check IS problematic, but the fix needs to be more careful:
- Keep the nested structure
- Only remove the material balance condition itself
- Maintain the flow control for when to evaluate

## Lesson Learned
When removing conditions, be VERY careful about control flow changes. The structure of the conditionals matters as much as the conditions themselves.

## Next Steps
- Skip Phase 1.2b and 1.2c for now
- Proceed to Phase 1.3 (SEE Pruning)
- Return to static null move improvements later with more careful implementation