# Stage 11 MVV-LVA SPRT Failure Investigation

## Date: 2025-08-13

## Problem Statement
Stage 11 MVV-LVA implementation showed negative performance in SPRT testing:
- **10+0.1 time control**: -2.5 to -2.9 ELO regression
- **60+0.6 time control**: -10 ELO regression (worse at deeper search!)
- Expected: +50-100 ELO improvement from better move ordering

## Root Cause Analysis

### Critical Finding: Sorting ALL Moves Instead of Just Captures
The implementation was applying MVV-LVA to all moves, including quiet moves, which:
1. Destroyed the natural move ordering from the generator
2. Added massive overhead for no benefit (quiet moves all score 0)
3. Got exponentially worse at deeper search depths

### Performance Bottlenecks Identified

#### 1. Memory Allocation in Hot Path
```cpp
std::vector<MoveScore> scoredMoves;  // Heap allocation at EVERY node
scoredMoves.reserve(moves.size());   // Another potential allocation
```
- At depth 14: ~100M nodes × allocation = 100M heap operations
- Cache pollution from allocator metadata
- Heap fragmentation

#### 2. Double Iteration and Copying
```cpp
// First pass: score all moves
for (Move move : moves) { /* score */ }
// Sort...
// Second pass: copy back
moves.clear();
for (const auto& ms : scoredMoves) { moves.add(ms.move); }
```

#### 3. Multiple Board Accesses Per Move
```cpp
Piece attackingPiece = board.pieceAt(fromSq);  // Cache miss
Piece capturedPiece = board.pieceAt(toSq);     // Another cache miss
```

#### 4. Unnecessary std::stable_sort
- Using stable_sort when stability not needed
- Complex tiebreaker logic adding overhead

#### 5. Thread-Local Storage Overhead
```cpp
static thread_local MvvLvaOrdering mvvLvaOrdering;  // TLS overhead
```

## Why Deeper Search Made It Worse

Counter-intuitively, the -10 ELO at 60+0.6 (depth 10-14) was worse than -2.5 ELO at 10+0.1 (depth 6-8) because:

1. **Exponential Node Growth**: Overhead multiplies with node count
   - Depth 8: ~10K nodes
   - Depth 14: ~100M nodes
   - 10,000x more allocations!

2. **Cache Thrashing at Scale**: More nodes = more cache pollution

3. **Destroying Natural Order**: Move generator already produces good ordering:
   - Castling moves first
   - Tactical moves early
   - Pawn pushes before piece moves
   
   By completely reordering, we made quiet move ordering random!

## Solution: Only Sort Captures

### Core Principle
MVV-LVA should ONLY reorder captures, preserving the generator's natural ordering for quiet moves.

### Implementation Changes Required

1. **In-place sorting** - No heap allocation
2. **Partition moves** - Separate captures from quiet moves
3. **Sort only captures** - Leave quiet moves in natural order
4. **Remove TLS** - Not needed for stateless object
5. **Inline scoring** - Avoid function call overhead

### Fixed Implementation
See `/workspace/src/search/move_ordering_fixed.cpp` for the corrected version.

## Expected Results
- **Immediate**: +8-10 ELO recovery from fixing allocation overhead
- **With optimization**: Possible slight improvement over Stage 10
- **Key metric**: 15-30% node reduction in tactical positions

## Lessons Learned

1. **Profile before optimizing** - The allocation overhead wasn't caught
2. **Respect natural ordering** - Don't destroy existing good properties
3. **Test at multiple time controls** - Issues may worsen at depth
4. **MVV-LVA is for captures only** - Common misconception to sort all moves
5. **In-place algorithms** - Critical for hot path code

## Next Steps

1. Implement the fixed version as Stage 11 SPRT Candidate #2
2. Update UCI version string to reflect new candidate
3. Rerun SPRT tests at both time controls
4. If successful, document the fix in development journal

## Technical Details for Implementation

### Priority 1: Core Fix
- Use `std::stable_partition` to separate captures/quiets
- Sort only the capture partition
- No memory allocation

### Priority 2: Optimizations
- Cache victim piece type in move generation
- Use `std::partial_sort` for top N captures
- Consider static array for very small sorts

### Priority 3: Cleanup
- Remove thread_local
- Simplify scoring function
- Add compile-time MVV-LVA table validation

## Validation Plan

1. Verify no heap allocations in hot path
2. Check move ordering preservation for quiet moves
3. Benchmark ordering time per position
4. Run perft to ensure correctness maintained
5. SPRT test at 10+0.1 and 60+0.6

## Success Criteria

- No ELO regression at any time control
- Ideally +20-50 ELO improvement
- Move ordering time < 1% of search time
- 15-30% node reduction in tactical positions