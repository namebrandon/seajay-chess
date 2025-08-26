# Phase 1.2b Complete Failure Analysis

## Why Phase 1.2b Cannot Work With Current Architecture

### The Fundamental Problem

The static null move pruning code in SeaJay has a deeply coupled structure where:
1. Evaluation only happens conditionally
2. Pruning check is nested inside evaluation condition
3. The material balance check is integral to the control flow

### Current Code Structure (Working)
```cpp
if (staticEval == zero) {
    if (material_balance_check) {
        staticEval = evaluate();
        if (staticEval - margin >= beta) {
            return prune;
        }
    }
}
```

The pruning ONLY happens when:
- We don't have cached eval (staticEval == zero)
- AND material balance passes
- AND we just evaluated
- AND the fresh eval exceeds beta minus margin

### Why My Attempts Failed

#### Attempt 1 (First Catastrophe):
```cpp
if (staticEval == zero) {
    staticEval = evaluate();  // ALWAYS evaluate
}
// ALWAYS check for pruning
if (staticEval - margin >= beta) {
    return prune;
}
```

**Problem:** This prunes even with cached zero values or when we shouldn't evaluate

#### Attempt 2 (Second Catastrophe):
```cpp
if (staticEval == zero) {
    staticEval = evaluate();
}
// Check for pruning outside the conditional
if (staticEval - margin >= beta) {
    return prune;
}
```

**Problem:** When we have cached eval, we skip evaluation but still try to prune. The cached eval might be:
- Actually zero (but marked as non-zero in cache)
- Stale from a different search
- Invalid for current position

### The Real Issue: Cache Ambiguity

The core problem is the cache system:
- Uses `int` for cached eval with 0 meaning "not cached"
- But eval can legitimately be 0
- No way to distinguish "cached zero" from "not cached"

When we have a cached eval of 0 stored as some non-zero sentinel:
1. We retrieve it thinking we have valid eval
2. Skip the evaluation block
3. Try to prune with invalid data
4. Return garbage scores

### Why The Material Balance Check "Works"

The material balance check isn't really about material balance - it's a GUARD that ensures:
- We only evaluate in certain conditions
- We only prune immediately after evaluation
- The control flow stays coupled

Removing it breaks this careful coupling.

### The Only Real Solutions

#### Solution 1: Fix the Cache System
```cpp
struct CachedEval {
    bool valid;
    eval::Score value;
};
```

Then we could safely:
```cpp
if (!cachedEval.valid) {
    cachedEval.value = evaluate();
    cachedEval.valid = true;
}
if (cachedEval.value - margin >= beta) {
    return prune;
}
```

#### Solution 2: Adopt Publius/Stockfish Pattern
Evaluate early and unconditionally:
```cpp
eval::Score staticEval = weAreInCheck ? -INFINITY : board.evaluate();
// Use staticEval everywhere without conditions
```

#### Solution 3: Keep Material Balance Check
Accept that it's part of the architecture and move on.

### Conclusion

Phase 1.2b CANNOT be implemented without either:
1. Fixing the fundamental cache ambiguity problem
2. Completely restructuring how evaluation works
3. Accepting the material balance check as necessary

The repeated catastrophic failures prove this isn't a simple fix - it's an architectural issue.

### Recommendation

**SKIP PHASE 1.2b ENTIRELY**

The material balance check might even be correct:
- It prevents expensive evaluation when we're too far behind
- It's a form of futility pruning in itself
- Removing it causes catastrophic failures

Focus on implementing features that don't require architectural changes:
- Phase 1.3: SEE pruning (independent system)
- Phase 2: Futility pruning (can work around current eval system)
- Phase 3: Move count pruning (doesn't need eval)

Return to static null move improvements only after fixing the evaluation architecture (documented in deferred items tracker).