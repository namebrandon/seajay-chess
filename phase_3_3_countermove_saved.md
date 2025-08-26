# Phase 3.3 Implementation - Saved for Later

## The Good Idea from Phase 3.3
Don't prune countermoves in move count pruning. This recovered +14 ELO from the regression.

## Implementation Code to Add Later:
```cpp
// In negamax.cpp, inside the move count pruning block:

// Phase 3.3: Countermove Consideration
// Don't prune countermoves - they're often good responses
if (prevMove != NO_MOVE && move == info.counterMoves.getCounterMove(prevMove)) {
    // This is a countermove, don't prune it
    // Skip the entire move count pruning block for this move
} else {
    // ... rest of move count pruning logic ...
}
```

## Test Results:
- Phase 3.3 alone: +14.16 ELO
- This partially offset the -91 ELO regression from Phase 3.1
- Shows that protecting tactical moves (countermoves) is important

## When to Re-apply:
After we fix Phase 3.1 to not be so aggressive, we should re-apply this countermove protection.