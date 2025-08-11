# Stage 9b Performance Regression Investigation Log

**Started:** August 10, 2025  
**Issue:** -70 Elo regression from Stage 9 to Stage 9b  
**Goal:** Recover performance while maintaining draw detection  

## Summary of Findings

### Key Discovery (Aug 10-11)
- Regression is **NOT** caused by draw detection logic
- Tests with draw detection ON and OFF both showed -70 Elo loss
- Root cause: Vector operations (push_back/pop_back) in makeMove/unmakeMove hot path

## Investigation Timeline

### Attempt 0: Initial Investigation (Aug 10, 2025)
- **Branch:** main (136d4aa)
- **Discovery:** Stage 9b has -70 Elo regression vs Stage 9
- **Testing:** Ran with draw detection enabled and disabled
- **Result:** Both configurations showed -70 Elo loss
- **Conclusion:** Vector operations are the problem, not draw detection

### Attempt 1: Vector Operations Fix - m_inSearch Flag (Aug 11, 2025)
- **Branch:** stage9b-debug/vector-ops-fix (to be created)
- **Hypothesis:** Skip vector push/pop during search using a flag
- **Changes:** 
  - Added `bool m_inSearch` to Board class
  - Added `setSearchMode(bool)` method
  - Modified makeMoveInternal to skip pushGameHistory() when m_inSearch
  - Modified unmakeMoveInternal to skip pop_back() when m_inSearch
  - Set search mode in negamax entry/exit
- **Testing:** 
  - Local build successful
  - Draw detection confirmed working
  - SPRT test: SPRT-2025-009-STAGE9B-FIXED
- **Early Results:** Stage 9 winning 9-2 against Stage 9b Fixed (BAD)
- **Status:** SPRT in progress, but early results concerning
- **Next Steps:** 
  - If SPRT fails, need to verify fix is actually working
  - Add counters to confirm vector ops aren't being called
  - Profile to find other performance issues

### Attempt 2: [TBD based on Attempt 1 results]
- **Branch:** TBD
- **Hypothesis:** TBD
- **Changes:** TBD
- **Result:** TBD

## Test Results Archive

### SPRT-2025-008-STAGE9B (Original Stage 9b tests)
- Various tests with draw detection on/off
- Consistent -70 Elo loss

### SPRT-2025-009-STAGE9B-FIXED (Vector ops fix)
- Test: Stage 9b Fixed vs Stage 9 Base
- Config: SPRT [30, 60] Elo, α=0.05, β=0.05
- Status: In progress
- Early result: 9W-2L-9D for Stage 9 (concerning)

## Profiling Data

### Baseline (Stage 9)
- Perft 5: [need to measure]
- Bench 5: ~0.02s
- NPS: [need to measure]

### Stage 9b Original  
- Perft 5: Significantly slower
- Bench 5: [need to measure]
- NPS: [need to measure]

### Stage 9b Fixed
- Perft 5: [need to measure]
- Bench 5: ~0.02s
- NPS: [need to measure]

## Key Code Locations

### Problem Areas
1. `Board::makeMoveInternal()` - lines 1110-1403
2. `Board::unmakeMoveInternal()` - lines 1405-1530  
3. `Board::pushGameHistory()` - lines 1925-1941
4. `m_gameHistory` vector operations

### Fix Locations
1. `Board::m_inSearch` flag - board.h
2. `Board::setSearchMode()` - board.h
3. Conditional history updates - board.cpp:1112, 1527
4. Search mode toggling - negamax.cpp

## Lessons Learned

1. **Always profile before optimizing** - We assumed draw detection was the issue
2. **STL containers in hot paths are deadly** - Even simple vector ops kill performance
3. **Test both with and without features** - This revealed the true problem
4. **Early SPRT results can be misleading** - But 9-2 is concerning enough to investigate

## Next Investigation Areas

If current fix doesn't fully work:
1. Verify m_inSearch is actually preventing vector operations
2. Check if there are other vector operations we missed
3. Profile to find other hot spots
4. Consider if SearchInfo itself has overhead
5. Check if the fix is being applied correctly in all code paths

## Success Criteria

- [ ] Recover at least 50 of the 70 Elo lost
- [ ] Maintain full draw detection functionality  
- [ ] Pass SPRT test vs Stage 9
- [ ] No vector operations in search (verified with counters)
- [ ] Clean, maintainable solution