# SeaJay Chess Engine - Stage 13: Iterative Deepening Implementation Plan

**Document Version:** 2.0  
**Date:** August 14, 2025  
**Stage:** Phase 3, Stage 13 - Iterative Deepening  
**Prerequisites Completed:** Yes  

## Executive Summary

Stage 13 implements production-quality iterative deepening with aspiration windows and sophisticated time management. Our current implementation is just a basic framework from Stage 7. This stage will add aspiration windows (+30-50 Elo), enhanced time management (+20-30 Elo), and move stability tracking to achieve the target +50-100 Elo improvement.

## Current State Analysis

### What Exists from Previous Stages
- Basic iterative deepening framework (Stage 7)
- Progressive depth search (1, 2, 3... until maxDepth)
- Basic time management (5% remaining + 75% increment)
- TT-based best move ordering
- Early termination on mate
- UCI info output per iteration

### What's Missing
1. **Aspiration Windows** - narrow search windows based on previous iteration
2. **Move Stability Tracking** - detect when best move changes between iterations
3. **Sophisticated Time Management** - branching factor predictions, soft/hard limits
4. **Fail High/Low Handling** - re-search with wider windows
5. **Game Phase Awareness** - adjust time usage for opening/middlegame/endgame
6. **Search Statistics** - track iteration costs for better predictions

## Deferred Items Being Addressed

From `/workspace/project_docs/tracking/deferred_items_tracker.md`:
- None specific to iterative deepening (this is a new implementation)

## Implementation Plan

### Pre-Phase Setup (30 minutes)
**Deliverable 0.1**: Safety Infrastructure
- Create feature branch `stage-13-iterative-deepening`
- Set up performance benchmark script
- Create "canary" test positions file
- **Test**: Baseline NPS measurement

**Deliverable 0.2**: Debug Infrastructure
- Add `TRACE_ITERATION` macro (compiled out in release)
- Create iteration logging framework
- Set up regression test suite
- **Test**: Verify debug output works

### Phase 1: Foundation (Day 1)
**Deliverable 1.1a**: Basic type definitions
- Create `iteration_info.h` with forward declarations
- Define `IterationInfo` struct (POD only, no methods)
- **Test**: Compile only
- **Checkpoint**: Git commit

**Deliverable 1.1b**: Enhanced search data structure
- Create `IterativeSearchData` class skeleton
- Add iteration array (no logic)
- **Test**: Instantiation test
- **Checkpoint**: Git commit

**Deliverable 1.1c**: Basic methods
- Add `recordIteration()` method
- Add `getLastIteration()` getter
- **Test**: Unit test recording one iteration
- **Checkpoint**: Git commit, NPS benchmark

**Deliverable 1.2a**: Search wrapper for testing
- Create `searchIterativeTest()` function
- Call existing search without modifications
- **Test**: Identical results to current search
- **Checkpoint**: Git commit

**Deliverable 1.2b**: Minimal iteration recording
- Record only depth 1 iteration
- No modification to search logic
- **Test**: Verify depth 1 data recorded
- **Checkpoint**: Git commit

**Deliverable 1.2c**: Full iteration recording
- Extend to all depths
- Still no search modifications
- **Test**: Verify all iterations recorded
- **Checkpoint**: Git commit, NPS benchmark

### Phase 2: Time Management (Day 2)
**Deliverable 2.1a**: Time management types
- Create `time_management.h` with types only
- Define `TimeInfo` struct
- **Test**: Compile test
- **Checkpoint**: Git commit

**Deliverable 2.1b**: Basic time calculation
- Implement `calculateOptimumTime()` (simple formula)
- No stability tracking yet
- **Test**: Unit test with known inputs/outputs
- **Checkpoint**: Git commit

**Deliverable 2.1c**: Soft/hard limits
- Add `calculateSoftLimit()` and `calculateHardLimit()`
- Safety checks for minimum time
- **Test**: Edge cases (low time, high time)
- **Checkpoint**: Git commit

**Deliverable 2.1d**: Stability tracking structure
- Add stability fields to `IterativeSearchData`
- Add `updateStability()` method (no logic)
- **Test**: Compile and structure test
- **Checkpoint**: Git commit

**Deliverable 2.1e**: Stability logic
- Implement stability detection
- Score and move comparison
- **Test**: Unit tests for stable/unstable positions
- **Checkpoint**: Git commit, NPS benchmark

**Deliverable 2.2a**: Time management integration prep
- Add time fields to search
- Keep old calculation alongside new
- **Test**: Both calculations produce values
- **Checkpoint**: Git commit

**Deliverable 2.2b**: Switch to new time management
- Replace old calculation
- Add logging for debugging
- **Test**: Time limits respected in test positions
- **Checkpoint**: Git commit, full regression test

### Phase 3: Aspiration Windows (Day 3-4)
**Deliverable 3.1a**: Window calculation types
- Create `aspiration_window.h` with types
- Define window struct and constants
- **Test**: Compile test
- **Checkpoint**: Git commit

**Deliverable 3.1b**: Initial window calculation
- Implement `calculateInitialWindow()`
- Fixed 16 cp window
- **Test**: Unit test returns correct bounds
- **Checkpoint**: Git commit

**Deliverable 3.1c**: Window widening logic
- Implement `widenWindow()` with delta growth
- Safety clamping to valid range
- **Test**: Unit test widening sequence
- **Checkpoint**: Git commit

**Deliverable 3.2a**: Search parameter modification
- Add alpha/beta parameters to iteration info
- Pass through existing values (no change)
- **Test**: Search results unchanged
- **Checkpoint**: Git commit

**Deliverable 3.2b**: Single aspiration search
- Use aspiration window for depth >= 4
- No re-search yet (fall back to full window)
- **Test**: Verify window used, results same
- **Checkpoint**: Git commit, NPS benchmark

**Deliverable 3.2c**: Basic re-search
- Add single re-search on fail high/low
- Use full window on re-search
- **Test**: Count re-searches on test positions
- **Checkpoint**: Git commit

**Deliverable 3.2d**: Window widening re-search
- Implement progressive widening
- Add attempt counter
- **Test**: Verify widening sequence
- **Checkpoint**: Git commit

**Deliverable 3.2e**: Re-search limits
- Add 5-attempt maximum
- Fall back to infinite window
- **Test**: Pathological position doesn't hang
- **Checkpoint**: Git commit, full regression test

### Phase 4: Branching Factor (Day 5)
**Deliverable 4.1a**: EBF tracking structure
- Add node count array to iterations
- Add EBF field
- **Test**: Compile test
- **Checkpoint**: Git commit

**Deliverable 4.1b**: Simple EBF calculation
- Calculate EBF between consecutive iterations
- Use last 2 iterations only
- **Test**: Manual calculation verification
- **Checkpoint**: Git commit

**Deliverable 4.1c**: Sophisticated EBF
- Use last 3-4 iterations
- Weighted average
- **Test**: Compare to expected values
- **Checkpoint**: Git commit

**Deliverable 4.2a**: Time prediction
- Implement `predictNextIterationTime()`
- Use EBF and current time
- **Test**: Predictions within 2x of actual
- **Checkpoint**: Git commit

**Deliverable 4.2b**: Early termination logic
- Check predicted time vs remaining
- Add safety margin
- **Test**: Stops appropriately on test positions
- **Checkpoint**: Git commit, NPS benchmark

### Phase 5: Polish and Integration (Day 6-7)
**Deliverable 5.1a**: UCI info structure
- Enhance UCI output format
- Add iteration details
- **Test**: Parse output successfully
- **Checkpoint**: Git commit

**Deliverable 5.1b**: Aspiration window reporting
- Show fail high/low in UCI
- Add window size info
- **Test**: Verify output format
- **Checkpoint**: Git commit

**Deliverable 5.2a**: Profile hot paths
- Run profiler on test positions
- Identify bottlenecks
- Document findings
- **Checkpoint**: Performance report

**Deliverable 5.2b**: Optimize critical sections
- Inline small functions
- Cache time checks
- Remove debug code in release
- **Test**: NPS within 5% of baseline
- **Checkpoint**: Git commit

**Deliverable 5.2c**: Final integration test
- Run full test suite
- Verify all features work together
- Check memory usage
- **Test**: All tests pass, no leaks
- **Checkpoint**: Final commit, tag release

## Technical Considerations (cpp-pro Review)

### Code Architecture
```
src/search/
├── iteration_info.h           (NEW: iteration tracking)
├── time_management.h          (NEW: time allocation)
├── time_management.cpp        (NEW: implementation)
├── aspiration_window.h        (NEW: window logic)
├── aspiration_window.cpp      (NEW: implementation)
├── search_stats.h            (NEW: enhanced statistics)
└── negamax.cpp               (modify iterative deepening)
```

### Key Data Structures
```cpp
struct IterationInfo {
    int depth{0};
    eval::Score score{eval::Score::zero()};
    Move bestMove{NO_MOVE};
    uint64_t nodes{0};
    TimeMs elapsed{0};
    
    // Aspiration window data
    eval::Score alpha{eval::Score::minus_infinity()};
    eval::Score beta{eval::Score::infinity()};
    int windowAttempts{0};
    bool failedHigh{false};
    bool failedLow{false};
    
    // Move stability
    bool moveChanged{false};
    int moveStability{0};
    
    // Additional tracking
    bool firstMoveFailHigh{false};
    int failHighMoveIndex{-1};
    eval::Score secondBestScore{eval::Score::minus_infinity()};
    double branchingFactor{0.0};
};

class IterativeSearchData : public SearchData {
public:
    static constexpr size_t MAX_ITERATIONS = 64;
    
    std::array<IterationInfo, MAX_ITERATIONS> m_iterations;
    size_t m_iterationCount{0};
    
    TimeMs m_softLimit{0};
    TimeMs m_hardLimit{0};
    TimeMs m_optimumTime{0};
    
    Move m_stableBestMove{NO_MOVE};
    int m_stabilityCount{0};
    
    // Methods
    void recordIteration(const IterationInfo& info);
    bool shouldStopEarly() const;
    double predictNextIterationTime() const;
    bool isEasyMove() const;
};
```

### Performance Best Practices
- Cache-aligned hot data structures
- Cache `now()` calls - check every 4096 nodes
- Use integer arithmetic when possible
- Inline critical path functions
- No heap allocations in search paths

### Risk Preparation and Mitigation

#### Git Strategy
- **Branch per phase**: `stage-13-phase-1`, `stage-13-phase-2`, etc.
- **Tag before integration**: `stage-13-pre-aspiration`, `stage-13-pre-time-mgmt`
- **Checkpoint commits**: After EVERY deliverable (43 total commits)
- **Rollback plan**: Can revert to any checkpoint

#### Performance Monitoring
```bash
# Run after each deliverable with NPS impact
./benchmark.sh --position startpos --depth 10 --iterations 5
# Fail if NPS drops >5% from baseline
```

#### Canary Tests
```cpp
// tests/canary_tests.cpp
TEST(CanaryTests, BasicSearch) {
    // Must pass after EVERY change
    Board board("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    auto result = search(board, 4);
    EXPECT_EQ(result.bestMove, Move("e2e4"));  // or d2d4
}

TEST(CanaryTests, TacticalPosition) {
    Board board("6k1/5ppp/8/8/8/7P/5PPK/8 w - - 0 1");
    auto result = search(board, 6);
    EXPECT_GT(result.score, Score(0));  // White not losing
}
```

#### Debug vs Release Paths
```cpp
#ifdef TRACE_ITERATIVE_DEEPENING
    #define TRACE_ITERATION(info) logIteration(info)
    #define TRACE_WINDOW(alpha, beta) logWindow(alpha, beta)
    #define TRACE_TIME(used, limit) logTime(used, limit)
#else
    #define TRACE_ITERATION(info) ((void)0)
    #define TRACE_WINDOW(alpha, beta) ((void)0)
    #define TRACE_TIME(used, limit) ((void)0)
#endif
```

#### Integration Testing Protocol
1. **Before each integration**: Full regression test
2. **After each integration**: Canary tests + NPS check
3. **End of each phase**: SPRT test vs previous phase
4. **Final validation**: Full SPRT vs Stage 12 baseline

## Chess Engine Considerations (chess-engine-expert Review)

### Critical Values and Formulas
- **Initial aspiration window**: 16 centipawns (Stockfish-proven)
- **Window growth**: delta += delta/3 (approximately 1.33x per fail)
- **Maximum re-searches**: 5 before infinite window
- **Time allocation**: timeLeft/40 + increment*0.8
- **Stability threshold**: 6-8 iterations of same best move
- **Default EBF**: 2.0-2.2 for middlegame positions

### Test Positions

#### Aspiration Window Tests
```
FEN: r1bqkb1r/pppp1ppp/2n2n2/1B2p3/4P3/5N2/PPPP1PPP/RNBQK2R w KQkq - 4 4
Name: "Lasker's Sacrifice" - Stable evaluation (~+0.30)

FEN: r1b1k2r/2qnbppp/p2ppn2/1p4B1/3NP3/2N2Q2/PPP2PPP/2KR1B1R w kq - 0 11
Name: "Wild Sicilian" - Tactical explosion (eval jumps +0.50 to +3.00)
```

#### Time Management Tests
```
FEN: 8/8/4kpp1/3p1b2/p6P/2B5/6P1/6K1 b - - 0 47
Name: "Karpov's Fortress" - Critical move (only move to draw)

FEN: r3k2r/pb1nbppp/1p2pn2/q3N3/2BP4/2N1P3/PP2QPPP/R3K2R w KQkq - 0 12
Name: "Horizon Effect Test" - Move stability (best move changes at depths 9, 11, 13)
```

#### Debug Positions
```
FEN: 8/8/1p1r1k2/p1pPN1p1/P3KnP1/1P6/8/3R4 b - - 0 1
Name: "The Aspiration Killer" - Caught bugs in Crafty and Fruit

FEN: 6k1/5p2/1p3P1K/p1pr4/6P1/P7/1P6/3R4 b - - 0 1
Name: "Time Scramble Special" - Tests time allocation under pressure
```

## Risk Mitigation

### Critical Bugs to Prevent

1. **Window Overflow**
   - Risk: Integer overflow in aspiration windows
   - Mitigation: Always clamp to [-VALUE_INFINITE, VALUE_INFINITE]
   ```cpp
   alpha = std::max(previousScore - delta, -VALUE_INFINITE);
   ```

2. **Time Check Race Conditions**
   - Risk: Race conditions in future SMP implementation
   - Mitigation: Use atomic operations and periodic checks
   ```cpp
   if ((nodes & 0xFFF) == 0) {  // Every 4096 nodes
       if (clock::now() > timeLimit.load()) {
           stopped.store(true);
       }
   }
   ```

3. **Move Stability False Positives**
   - Risk: Incorrectly identifying unstable positions as stable
   - Mitigation: Consider both move and score changes
   ```cpp
   if (bestMove == lastBestMove && abs(score - lastScore) < 20) {
       stability++;
   }
   ```

4. **Aspiration Window Starvation**
   - Risk: Infinite re-searches in pathological positions
   - Mitigation: Hard limit of 5 re-search attempts

5. **Time Overruns**
   - Risk: Exceeding time limits in tactical positions
   - Mitigation: Hard upper bound of 50% remaining time

## Validation Strategy

### Unit Tests
- Test each component in isolation
- Verify window calculations with known values
- Test time management formulas
- Validate stability tracking logic

### Integration Tests
- Test aspiration window convergence rates
- Verify time usage efficiency
- Test move stability detection
- Validate UCI info output

### Performance Tests
- Benchmark NPS before/after implementation
- Measure time usage accuracy
- Track aspiration window efficiency
- Monitor EBF predictions

### SPRT Validation
- Target: +50-100 Elo improvement
- Test against Stage 12 baseline
- Use standard SPRT parameters (alpha=0.05, beta=0.05)

## Items Being Deferred

1. **Multi-PV Support** - Infrastructure only, full implementation in later stage
2. **Contempt-Adjusted Windows** - Advanced feature for Phase 5+
3. **History-Based Windows** - Requires position classification system

## Success Criteria

1. **Aspiration Windows**: 90%+ of searches stay within initial window
2. **Time Usage**: Use 95%+ of available time without overruns
3. **Move Stability**: Correctly identify and handle unstable positions
4. **SPRT Validation**: +50-100 Elo improvement confirmed
5. **No Performance Regression**: Maintain current NPS
6. **All test positions pass**: Correct best moves found within time limits

## Timeline Estimate

- **Total Duration**: 7 days
- **Phase 1**: 1 day (foundation)
- **Phase 2**: 1 day (time management)
- **Phase 3**: 2 days (aspiration windows)
- **Phase 4**: 1 day (branching factor)
- **Phase 5**: 2 days (polish and validation)

---

## Appendix A: Detailed Expert Recommendations

### Aspiration Windows (chess-engine-expert)
- **Initial window**: 16 centipawns (Stockfish-style)
- **Window growth**: delta += delta/3 (approximately 1.33x per fail)
- **Depth adjustment**: Slightly wider windows at higher depths
- **Maximum attempts**: 5 re-searches before infinite window

### Time Management (chess-engine-expert)
- **Base allocation**: timeLeft/40 + increment*0.8
- **Soft limit**: optimumTime (can be exceeded if position unstable)
- **Hard limit**: min(timeLeft*0.95, optimumTime*5)
- **Stability threshold**: 6-8 iterations of same best move
- **Default EBF**: 2.0-2.2 for middlegame positions

### Common Implementation Pitfalls
1. **Window clamping**: Always clamp to [-VALUE_INFINITE, VALUE_INFINITE]
2. **TT handling**: Don't update TT during aspiration fails
3. **Time caching**: Cache now() calls - they're expensive
4. **Emergency buffer**: Keep 5% time reserve
5. **Atomic stops**: Use atomic flags for time checks

## Appendix B: Implementation Examples from Other Engines

### Stockfish's Asymmetric Windows
Uses asymmetric adjustments when failing low to avoid missing good moves. Beta stays closer on fail-low to catch improvements.

### Ethereal's Fail Counters
Tracks separate fail high/low counts with exponential growth: delta * (1 << failCount). Hard limit of 5 total re-searches.

### Komodo's Move Stability
```cpp
if (bestMove == previousBestMove && score > previousScore - 20) {
    stability++;
    if (stability > 8 && timeUsed > optimumTime * 0.15) {
        // Easy move - can stop early
    }
} else {
    stability = 0;
    optimumTime *= 1.2;  // Unstable - need more time
}
```

## Appendix C: Forum Wisdom

**Andrew Grant (Ethereal)**: "Don't trust the first fail high/low. Always re-search at least once."

**Robert Hyatt (Crafty)**: "Crafty lost a world championship by using 58 minutes on move 12. Always set upper bounds on time allocation."

**Marco Costalba (Stockfish)**: "Score stability matters as much as move stability. If score drops 50+ cp, allocate more time."

**Tord Romstad (Stockfish)**: "We once had a bug where Stockfish would use 0 time on the last move before time control. Never trust movesToGo == 1."

## Appendix D: Safety Implementation Patterns

### Window Overflow Protection
```cpp
eval::Score clampToValidRange(eval::Score score) {
    constexpr auto MAX_VALID = eval::Score(1000000);
    return std::clamp(score, -MAX_VALID, MAX_VALID);
}
```

### Time Calculation Safety
```cpp
TimeMs calculateSafeTimeLimit(TimeMs remaining) {
    constexpr auto MIN_BUFFER = TimeMs(50);
    constexpr auto MIN_TIME = TimeMs(5);
    
    if (remaining <= MIN_BUFFER) return MIN_TIME;
    return remaining - MIN_BUFFER;
}
```

### Debug Infrastructure
```cpp
#ifdef DEBUG
#define TRACE_ITERATION(info) \
    std::cerr << "Iteration " << info.depth \
              << ": " << info.nodes << " nodes" \
              << ", window: [" << info.alpha << ", " << info.beta << "]" \
              << std::endl;
#else
#define TRACE_ITERATION(info) ((void)0)
#endif
```

---

**Next Steps**: Begin with Phase 1, Deliverable 1.1 - create test harness and data structures. Use the provided test positions for validation throughout development.