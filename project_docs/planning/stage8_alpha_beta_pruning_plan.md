# SeaJay Chess Engine - Stage 8: Alpha-Beta Pruning Implementation Plan

**Document Version:** 1.0  
**Date:** August 10, 2025  
**Stage:** Phase 2, Stage 8 - Alpha-Beta Pruning  
**Prerequisites Completed:** Yes (Stage 7 - Negamax Search)  

## Executive Summary

Stage 8 implements alpha-beta pruning to dramatically reduce the search tree size while maintaining identical move selection. The framework is already in place from Stage 7 - we primarily need to activate beta cutoffs, add basic move ordering, and implement search statistics. Expected outcome is a 5x search speed improvement enabling depth 6 searches in under 1 second.

## Current State Analysis

### From Previous Stages:
- **Stage 7 Complete**: 4-ply negamax search operational
- **SPRT Validated**: +293 Elo improvement over random play
- **Framework Ready**: Alpha-beta parameters already passed through search
- **Cutoff Infrastructure**: Lines 134-145 in negamax.cpp prepared but disabled
- **Material Evaluation**: Basic centipawn-based evaluation working
- **Time Management**: Simple time control implemented

### Code Structure:
- `src/search/negamax.cpp`: Core search with alpha-beta skeleton
- `src/search/types.h`: SearchInfo structure ready for statistics
- `src/core/move.h`: `isCapture()` and `isPromotion()` methods available

## Deferred Items Being Addressed

From `/workspace/project_docs/tracking/deferred_items_tracker.md`:

1. **Active Alpha-Beta Cutoffs** - Main focus of Stage 8
2. **Basic Move Ordering** - Captures/promotions first
3. **Search Tree Statistics** - Beta cutoff tracking

### Items Remaining Deferred:
- MVV-LVA ordering → Stage 9
- Killer moves → Stage 9  
- Aspiration windows → Future stages
- History heuristic → Phase 3

## Implementation Plan

### Phase 1: Enable Beta Cutoffs (1 hour)
1. Activate break statement at line 142 in negamax.cpp
2. Verify fail-soft implementation (return bestScore)
3. Add defensive assertion for alpha < beta
4. Test basic functionality

### Phase 2: Add Search Statistics (2 hours)
1. Extend SearchInfo structure:
   - `uint64_t betaCutoffs`
   - `uint64_t betaCutoffsFirst`
   - `uint64_t totalMoves`
2. Implement tracking in negamax
3. Add efficiency calculations:
   - Effective branching factor
   - Move ordering efficiency
4. Display statistics in UCI info output

### Phase 3: Implement Move Ordering (3 hours)
1. Create `orderMoves()` function:
   - Promotions first (queen promotions highest)
   - Captures second (using `isCapture()`)
   - Quiet moves last
2. Apply ordering after move generation
3. Optimize for small move lists (20-40 moves typical)
4. Avoid heap allocations

### Phase 4: Validation Framework (2 hours)
1. Create comparison test harness:
   - Run search with/without alpha-beta
   - Verify identical best move
   - Verify identical score
   - Measure node reduction
2. Implement test positions from expert review:
   - Bratko-Kopec 1 (tactical)
   - Fine #70 (deep calculation)
   - Kiwipete depth 4 (node count verification)

### Phase 5: SPRT Testing (4 hours)
1. Build two versions:
   - `seajay-negamax`: Stage 7 without alpha-beta
   - `seajay-alphabeta`: Stage 8 with pruning enabled
2. Configure SPRT test:
   - Same depth (4 ply)
   - Verify strength equivalence
   - Measure speed improvement
3. Run extended stability test

## Technical Considerations

### C++ Implementation Details (from cpp-pro review):

1. **Move Ordering Algorithm**:
```cpp
template<std::ranges::random_access_range R>
inline void orderMoves(R&& moves) noexcept {
    auto first = std::ranges::begin(moves);
    auto last = std::ranges::end(moves);
    auto partition_point = first;
    
    // Promotions first
    for (auto it = first; it != last; ++it) {
        if (isPromotion(*it)) {
            if (it != partition_point) {
                std::iter_swap(it, partition_point);
            }
            ++partition_point;
        }
    }
    
    // Then captures
    for (auto it = partition_point; it != last; ++it) {
        if (isCapture(*it)) {
            if (it != partition_point) {
                std::iter_swap(it, partition_point);
            }
            ++partition_point;
        }
    }
}
```

2. **Statistics without performance impact**
3. **Debug-only assertions for alpha-beta bounds**
4. **Stack allocation for MoveList is appropriate**
5. **Use `[[unlikely]]` attribute for cutoff branch**

## Chess Engine Considerations

### From chess-engine-expert review:

1. **Move Ordering Priority**:
   - Promotions (especially queen)
   - Captures
   - Quiet moves

2. **Critical Validation**:
   - Best move must remain identical
   - Score must remain identical  
   - Only node count should change

3. **Expected Performance**:
   - 65-85% node reduction at depth 4
   - >60% first-move cutoff rate
   - Effective branching factor ~5-6

4. **Common Pitfalls to Avoid**:
   - Don't update bestMove on beta cutoff
   - Root node needs special handling
   - Maintain fail-soft for future PVS

## Risk Mitigation

### Risk 1: Different Move Selection
**Mitigation**: Comprehensive validation suite comparing with/without pruning

### Risk 2: Incorrect Pruning 
**Mitigation**: Test edge cases (zugzwang, stalemate, repetition)

### Risk 3: Performance Regression
**Mitigation**: Track statistics, expect >50% node reduction minimum

### Risk 4: Search Instability
**Mitigation**: SPRT test for strength equivalence

### Risk 5: Move Ordering Bug
**Mitigation**: Unit tests for ordering function, verify captures/promotions first

## Validation Strategy

### Correctness Tests:
1. Compare 50+ positions with/without alpha-beta
2. Verify identical best moves and scores
3. Test special positions (stalemate, zugzwang, deep tactics)

### Performance Tests:
1. Measure node reduction (target >65%)
2. Track first-move cutoff rate (target >60%)
3. Calculate effective branching factor (target <6)

### SPRT Validation:
- H0: Elo difference = 0 (same strength)
- H1: Elo difference = 5
- Alpha = 0.05, Beta = 0.05
- Expected: Pass H0 (equivalent strength, fewer nodes)

## Items Being Deferred

### To Stage 9:
1. MVV-LVA capture ordering
2. Killer move heuristic
3. More sophisticated move ordering

### To Future Phases:
1. Aspiration windows
2. History heuristic
3. Transposition table move ordering
4. Late move reductions

## Success Criteria

Stage 8 is complete when:
1. ✅ Beta cutoffs activated and working
2. ✅ Basic move ordering implemented (promotions/captures first)
3. ✅ Search statistics tracking operational
4. ✅ >65% node reduction at depth 4
5. ✅ >60% first-move cutoff rate
6. ✅ All validation tests pass (same moves/scores)
7. ✅ SPRT shows equivalent strength
8. ✅ Reaches depth 6 in <1 second from start position
9. ✅ No memory leaks or crashes
10. ✅ Documentation updated

## Timeline Estimate

- **Phase 1**: 1 hour - Enable cutoffs
- **Phase 2**: 2 hours - Statistics tracking  
- **Phase 3**: 3 hours - Move ordering
- **Phase 4**: 2 hours - Validation framework
- **Phase 5**: 4 hours - SPRT testing
- **Total**: ~12 hours

## Implementation Notes

### Key Files to Modify:
1. `src/search/negamax.cpp` - Enable cutoff, add ordering
2. `src/search/types.h` - Extend SearchInfo
3. `tests/search_test.cpp` - Add validation tests

### Testing Commands:
```bash
# Build
cd /workspace/build && cmake .. && make -j

# Run validation
./bin/seajay test alpha-beta

# Run SPRT
./tools/scripts/run-sprt.sh seajay-negamax seajay-alphabeta
```

## Review Sign-offs

- **cpp-pro**: Reviewed C++ implementation approach ✓
- **chess-engine-expert**: Reviewed chess-specific aspects ✓
- **Risk Analysis**: Completed with mitigations ✓
- **Deferred Items**: Tracked and documented ✓

---

**Ready to proceed with Stage 8 implementation**