# Phase 2a Implementation Plan: Ranked MovePicker (Scoring + Shortlist)

## Executive Summary
Phase 2a introduces a lightweight ranked move picker that scores all moves in a single pass and maintains a top-K shortlist for better early decision quality. This phase focuses solely on scoring and ordering - no pruning changes.

## Design Principles
- **Single-pass, O(n) complexity** - No nested loops or quadratic operations
- **Zero dynamic allocations** - Stack-based fixed arrays only
- **Lazy SEE evaluation** - Compute SEE only when needed for shortlist boundary
- **Clean fallback path** - UCI toggle with legacy ordering preserved
- **Root safety** - Keep legacy ordering at ply 0 until proven safe

## Core Components

### 1. RankedMovePicker Class Design

#### Class Structure
```cpp
class RankedMovePicker {
    // No owning pointers, trivially copyable
    // References to existing tables only
    // Stack-based storage for shortlist
};
```

#### Key Design Decisions
- **No RAII/copy issues**: Explicitly delete copy/move or make trivially copyable
- **No ownership**: Only references/raw pointers to existing tables
- **Iterator-like API**: `next()` method for move yielding
- **Pre-rank once**: Compute shortlist during construction for Phase 2a simplicity

### 2. Scoring System

#### Capture Scoring (Lazy SEE)
```
1. Pre-score with MVV-LVA (cheap)
2. Compute SEE only when:
   - Comparing for shortlist boundary
   - MVV-LVA suggests it's "good" (victim_value >= attacker_value)
3. Add promotion bonus (queen > rook > bishop/knight)
4. Small check bonus (+50)
```

#### Quiet Move Scoring
```
score = history_score * 2 +
        killer_bonus * 1000 +
        countermove_bonus * 500 +
        refutation_bonus * 300 +
        check_bonus * 50
```
- Use int16_t for scores (sufficient range)
- Weights are initial estimates, not tuned

#### Tie-Breaking
For deterministic ordering:
1. Score (primary)
2. MVV-LVA value (secondary)
3. Raw move encoding (tertiary)

### 3. Top-K Shortlist Mechanics

#### Implementation
- **Fixed size**: K=10 initially (stack array)
- **Simple insertion**: Track minimum score in shortlist
- **No heap/vector**: Direct array manipulation
- **Deduplication**: Check against TT move during insertion

#### Future Enhancement (not in 2a)
- Depth-aware K: `K = min(12, 8 + depth/2)`

### 4. Move Yield Order

#### Main Search (negamax)
1. **TT move** (if legal and non-null)
2. **Top-K shortlist** (sorted by score)
3. **Remaining captures** (SEE≥0 first, then SEE<0)
4. **Remaining quiets** (unsorted)

#### Quiescence Search
1. **TT move** (if capture/promotion and legal)
2. **Captures** (MVV-LVA ordered, optional lazy SEE)
3. **Promotions** (if not captures)
- **No quiets in qsearch**

### 5. TT Move Handling

#### Validation Protocol
```cpp
if (ttMove != MOVE_NONE) {
    if (isLegal(ttMove)) {
        // Yield first, mark as used
        ttMoveUsed = true;
    } else {
        // Ignore illegal TT move
        ttMove = MOVE_NONE;
    }
}
```

#### Deduplication
- Full move encoding comparison (including promotion piece)
- Skip TT move if already in shortlist

## Integration Points

### Negamax Integration
```cpp
if (UseRankedMovePicker && ply > 0) {  // Skip root for safety
    RankedMovePicker picker(...);
    while (Move move = picker.next()) {
        // existing search logic
    }
} else {
    // Legacy orderMoves() path
}
```

### Quiescence Integration
```cpp
if (UseRankedMovePicker) {
    RankedMovePickerQS picker(...);  // Simplified variant
    while (Move move = picker.next()) {
        // existing qsearch logic
    }
} else {
    // Legacy ordering
}
```

### UCI Toggle
- `UseRankedMovePicker` (default: false)
- No runtime behavior change until wired
- Keep `UseRankedMovePickerAtRoot` internal or implicit (ply > 0 check)

## Telemetry (Low Overhead)

### Metrics to Track
1. **Best-move rank distribution**
   - Where does eventual PV move appear in ordering?
   - Track via histogram: rank_1, rank_2_5, rank_6_10, rank_11+

2. **First-move cutoff rate**
   - Already tracked in existing stats
   - Compare before/after

3. **Shortlist coverage**
   - Fraction of quiet PV moves in top-K
   - Helps tune K value

4. **SEE call efficiency**
   - Count SEE evaluations vs total captures
   - Verify lazy SEE is working

### Implementation
```cpp
#ifdef SEARCH_STATS
    struct MovePickerStats {
        uint64_t bestMoveRank[4];  // [1], [2-5], [6-10], [11+]
        uint64_t shortlistHits;
        uint64_t seeCallsLazy;
        uint64_t capturesTotal;
    };
#endif
```

## Risk Mitigation

### Performance Risks
- **SEE overhead**: Mitigated by lazy evaluation
- **Shortlist overhead**: Fixed-size, O(n) single pass
- **Cache misses**: Keep data structures compact

### Correctness Risks
- **TT move illegality**: Explicit validation
- **Move deduplication**: Full encoding comparison
- **Promotion handling**: Include piece in all comparisons

### Regression Risks
- **Root ordering**: Keep legacy at ply 0
- **Qsearch expansion**: No quiets, minimal changes
- **Bench deviation**: Accept small changes, document

## Testing Protocol

### Build Discipline
```bash
# Clean rebuild for each test
make clean
rm -rf build
./build.sh Release
```

### Validation Steps
1. **Unit tests**: All must pass
2. **Perft**: Unchanged (move gen correctness)
3. **Bench**: Document any change (ordering affects nodes)
4. **Tactical suite**: ≥ baseline pass rate
5. **Depth comparison**: Similar depth at fixed time

### A/B Testing
```
Branch: feature/20250908-ranked-movepicker-2a
Base: current HEAD (Phase 1 complete)
Time Control: 10+0.1
Bounds: [-3.00, 3.00] (non-regression)
Book: UHO_4060_v2.epd
```

## Implementation Checklist

### Phase 2a Core Tasks
- [ ] Create feature branch from current HEAD
- [ ] Implement RankedMovePicker class
  - [ ] Scoring functions (lazy SEE for captures)
  - [ ] Top-K maintenance
  - [ ] Iterator interface
  - [ ] TT move handling
- [ ] Implement RankedMovePickerQS (simplified)
- [ ] Wire UCI toggle (no behavior change initially)
- [ ] Integrate with negamax (ply > 0 only)
- [ ] Integrate with quiescence
- [ ] Add telemetry hooks
- [ ] Test locally (unit, perft, bench, tactical)
- [ ] Push to remote with proper bench in commit
- [ ] OpenBench A/B test

### Success Criteria
- No regression in tactical strength
- First-move cutoff rate maintained or improved
- Clean, maintainable code
- Bench documentation in commit

## Notes for Implementation

### Code Organization
```
src/search/
  ranked_move_picker.h       # Main class
  ranked_move_picker.cpp     # Implementation
  ranked_move_picker_qs.h    # Quiescence variant
  ranked_move_picker_qs.cpp  # QS implementation
```

### Memory Layout
```cpp
struct RankedMovePicker {
    // Fixed arrays on stack
    Move shortlist[10];
    int16_t scores[10];
    int shortlistSize;
    
    // References to tables
    const Board& board;
    const HistoryHeuristic& history;
    const KillerMoves& killers;
    // ...
};
```

### Performance Notes
- Inline hot functions (scoring)
- Minimize branches in inner loops
- Use __builtin_expect for likely paths
- Consider prefetch for history table access

## Conclusion
Phase 2a provides a focused, low-risk improvement to move ordering through lightweight scoring and shortlist extraction. By avoiding pruning changes and keeping a clean fallback path, we minimize regression risk while setting up the foundation for Phase 2b's rank-aware pruning gates.