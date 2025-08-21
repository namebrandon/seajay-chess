# Stage 23: Countermoves Extended Improvement Plan

## Executive Summary

Following expert review of our countermoves implementation which achieved +4 ELO (vs expected 10-15 ELO), this plan outlines incremental improvements through micro-phasing. Each phase will be tested via OpenBench before proceeding.

**Current Status:** +4.71 ELO with bonus=8000 (integration/countermoves branch)
**Target:** Additional +3-7 ELO through targeted improvements
**Approach:** Micro-phased implementation with mandatory testing between phases

## Expert-Identified Issues to Address

### Immediate Actions (This Plan)
1. Fix potential NO_MOVE initialization bug
2. Test higher priority positioning (before killers)
3. Add piece type to indexing for better context

### Future Considerations (Not This Plan)
- Cache efficiency improvements (1D array refactor)
- Thread safety enhancements
- Multiple countermove slots
- Success rate tracking

## Micro-Phasing Strategy

### Starting Point
- **Branch:** Create `feature/20250821-countermoves-extended` from current integration branch
- **Baseline:** Current +4.71 ELO implementation with bonus=8000
- **Testing:** EVERY phase requires OpenBench completion before proceeding

## Phase 1: NO_MOVE Initialization Fix

### Phase 1.1: Diagnostic Infrastructure
**Goal:** Add diagnostic capability without changing behavior
**Changes:**
- Add `validateTable()` method to CounterMoves class
- Add UCI command `validate_countermoves` for debugging
- Add statistics tracking for NO_MOVE vs valid moves
**Expected:** 0 ELO change
**Purpose:** Understand current state without modification

### Phase 1.2: Fix Initialization
**Goal:** Ensure proper NO_MOVE initialization
**Changes:**
```cpp
void clear() {
    for (int from = 0; from < 64; ++from) {
        for (int to = 0; to < 64; ++to) {
            m_counters[from][to] = NO_MOVE;
        }
    }
}
```
**Expected:** 0 to +1 ELO (fixing potential corruption)
**Purpose:** Eliminate potential undefined behavior

### Phase 1.3: Add Validation on Retrieval
**Goal:** Ensure only valid moves are returned
**Changes:**
- Add legality check in getCounterMove()
- Return NO_MOVE if stored move is invalid
- Track statistics on invalid move filtering
**Expected:** 0 to +1 ELO
**Purpose:** Prevent illegal moves from affecting ordering

## Phase 2: Priority Positioning Tests

### Phase 2.1: Position Equal to Killers
**Goal:** Test countermoves at same priority as killers
**Changes:**
- Modify move_ordering.cpp to place countermove with killers
- Keep bonus at 8000 (don't change scoring)
- Just change relative position in ordering
**Expected:** +1 to +2 ELO
**Purpose:** Test if position matters more than score

### Phase 2.2: Position Before Killers
**Goal:** Test countermoves with higher priority than killers
**Changes:**
- Place countermove before killer moves in ordering
- Still using position-based ordering (std::rotate)
- No score changes
**Expected:** +1 to +3 ELO
**Purpose:** Determine optimal position in move ordering

### Phase 2.3: Increase Bonus to 12000
**Goal:** Test higher bonus with better positioning
**Changes:**
- Keep position before killers (if 2.2 successful)
- Increase CountermoveBonus from 8000 to 12000
- Monitor for regression signs
**Expected:** +0 to +2 ELO additional
**Purpose:** Find optimal bonus with new positioning

### Phase 2.4: Fine-tune Bonus
**Goal:** Find optimal bonus value
**Changes:**
- Test values: 10000, 14000, 16000
- Stop if regression detected
- Use best position from 2.1/2.2
**Expected:** Identify optimal configuration
**Purpose:** Maximize ELO without regression

## Phase 3: Piece-Type Indexing Enhancement

### Phase 3.1: Add Piece-Type Infrastructure
**Goal:** Create parallel piece-based table without using it
**Changes:**
```cpp
class CounterMoves {
    Move m_counters[64][64];           // Existing
    Move m_pieceCounters[6][64];       // New: [piece_type][to_square]
    bool m_usePieceIndexing = false;   // UCI controllable
};
```
**Expected:** 0 ELO (not active)
**Purpose:** Add infrastructure safely

### Phase 3.2: Shadow Mode Updates
**Goal:** Update both tables but only use square-based
**Changes:**
- Update both tables in update() method
- Add statistics comparing hit rates
- Still return from square-based table only
**Expected:** 0 ELO
**Purpose:** Verify piece indexing works correctly

### Phase 3.3: Hybrid Mode - Fallback
**Goal:** Use piece-based with square-based fallback
**Changes:**
```cpp
Move getCounterMove(Move prevMove, Piece prevPiece) {
    Move pieceCounter = m_pieceCounters[pieceType(prevPiece)][moveTo(prevMove)];
    if (pieceCounter != NO_MOVE) return pieceCounter;
    return m_counters[moveFrom(prevMove)][moveTo(prevMove)];
}
```
**Expected:** +1 to +2 ELO
**Purpose:** Test if piece context improves move selection

### Phase 3.4: Hybrid Mode - Best-of-Both
**Goal:** Return better scoring move from both tables
**Changes:**
- Check both tables
- Return move with higher history score
- Track which table provides better moves
**Expected:** +1 to +3 ELO
**Purpose:** Maximize information usage

### Phase 3.5: Optimize Based on Results
**Goal:** Use learning from 3.3/3.4 to optimize
**Changes:**
- If piece-based consistently better: make primary
- If square-based better: revert to original
- If hybrid best: optimize the selection logic
**Expected:** Consolidate gains
**Purpose:** Finalize optimal indexing strategy

## Testing Protocol

### For Each Phase:
1. Create clear commit with phase identifier
2. Get bench count: `echo "bench" | ./bin/seajay | grep "Benchmark complete" | awk '{print $4}'`
3. Commit with message: `feat: Countermoves extended Phase X.Y - [description] - bench [count]`
4. Push to branch
5. Run OpenBench test vs current best (integration/countermoves)
6. **WAIT for test completion**
7. Document results in tracking table
8. Decision: proceed, adjust, or stop

### OpenBench Configuration:
- **TC:** 15+0.05
- **Book:** UHO_4060_v3.epd
- **Confidence:** 95%
- **SPRT Bounds:**
  - Diagnostic phases: [-5, 0] (verify no regression)
  - Improvement phases: [0, 5] (verify positive gain)
  - Final validation: [0, 8] (confirm cumulative improvement)

## Success Criteria

### Minimum Success:
- No regression in any phase
- At least +2 ELO cumulative gain
- Clean, maintainable code

### Target Success:
- +3-5 ELO cumulative gain
- Clear understanding of what improvements work
- Foundation for future enhancements

### Optimal Success:
- +5-7 ELO cumulative gain
- Countermoves performing closer to theoretical expectation
- Clear path to remaining improvements

## Risk Management

### Regression Protocols:
1. **Small regression (-1 to -2 ELO):** Continue to next phase, may be noise
2. **Medium regression (-2 to -5 ELO):** Stop, analyze, consider reverting
3. **Large regression (> -5 ELO):** Immediate stop, revert, investigate

### Decision Points:
- After Phase 1: Decide if initialization was an issue
- After Phase 2: Determine optimal positioning/bonus
- After Phase 3: Evaluate piece-type indexing value
- Final: Merge to main or continue improvements

## Timeline Estimate

| Phase Group | Implementation | Testing | Total |
|-------------|---------------|---------|-------|
| Phase 1 (1.1-1.3) | 2 hours | 6 hours | 8 hours |
| Phase 2 (2.1-2.4) | 2 hours | 8 hours | 10 hours |
| Phase 3 (3.1-3.5) | 4 hours | 10 hours | 14 hours |
| **Total** | **8 hours** | **24 hours** | **32 hours** |

## Progress Tracking

| Phase | Status | Commit | Bench | ELO vs Baseline | Notes |
|-------|--------|--------|-------|-----------------|-------|
| 1.1 | Pending | - | - | - | Diagnostic |
| 1.2 | - | - | - | - | Fix init |
| 1.3 | - | - | - | - | Validation |
| 2.1 | - | - | - | - | Equal priority |
| 2.2 | - | - | - | - | Before killers |
| 2.3 | - | - | - | - | Bonus 12000 |
| 2.4 | - | - | - | - | Tune bonus |
| 3.1 | - | - | - | - | Piece infrastructure |
| 3.2 | - | - | - | - | Shadow mode |
| 3.3 | - | - | - | - | Hybrid fallback |
| 3.4 | - | - | - | - | Best-of-both |
| 3.5 | - | - | - | - | Optimize |

## Implementation Notes

### Critical Reminders:
1. **NO PROCEEDING** without OpenBench test completion
2. **DOCUMENT EVERYTHING** - especially unexpected results
3. **BENCH COUNT** in every commit message
4. **POSITION-BASED** ordering only (no score manipulation)
5. **TEST VS BASELINE** (current +4.71 ELO implementation)

### Code Quality Requirements:
- Maintain existing code style
- No compiler warnings
- No #ifdef for features (runtime UCI control only)
- Clear commit messages with phase identification
- Update statistics/diagnostics for analysis

## Decision Tree

```
Phase 1 Results:
├─ Positive → Continue to Phase 2
├─ Neutral → Continue to Phase 2 (no harm)
└─ Negative → Stop, investigate initialization bug

Phase 2 Results:
├─ Before killers better → Use that for Phase 3
├─ Equal to killers better → Use that for Phase 3  
├─ No improvement → Skip Phase 3, consider cache optimization
└─ Regression → Revert to baseline

Phase 3 Results:
├─ Piece indexing helps → Implement fully
├─ Hybrid best → Keep hybrid approach
├─ No improvement → Keep Phase 2 gains only
└─ Regression → Revert to Phase 2 configuration

Final Decision:
├─ ≥ +3 ELO total → Merge to main
├─ +1 to +3 ELO → Merge but plan cache optimization
├─ 0 to +1 ELO → Document learnings, defer improvements
└─ Negative → Revert all changes, investigate deeply
```

## Appendix: Alternative Approaches

If the above phases don't yield expected improvements, consider:

### Alternative A: Score-Based Ordering
Replace position-based ordering with score manipulation:
- Pros: More flexible, easier to tune
- Cons: Risk of score overflow, interaction effects

### Alternative B: Cache Optimization Focus
Refactor to 1D array with better cache locality:
- Pros: Addresses biggest performance issue
- Cons: Larger code change, higher risk

### Alternative C: Multiple Countermove Slots
Store 2-3 countermoves per position:
- Pros: More robust, handles variations
- Cons: Memory increase, complexity

### Alternative D: Statistical Approach
Track success rates and adapt bonuses:
- Pros: Self-tuning, optimal per position
- Cons: Memory overhead, complexity

## Conclusion

This plan provides a systematic approach to improving the countermoves implementation through careful micro-phasing. By testing each small change independently, we can identify exactly what works and avoid the catastrophic regressions seen in the original implementation.

The expected outcome is an additional +3-7 ELO on top of the current +4.71 ELO, bringing the total countermoves contribution to approximately +8-12 ELO, much closer to the theoretical expectation of 10-15 ELO.