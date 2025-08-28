# UCI Info Implementation Plan for SeaJay

## Overview
Implement continuous UCI info updates during search to provide real-time analysis feedback to chess GUIs (like HIARCS), following the pattern used by Stash and other modern engines.

## Current State
- Info updates only sent at end of each depth iteration
- No currmove/currmovenumber reporting
- No time-based periodic updates during search
- Missing UCI eval command
- No hashfull reporting

## Implementation Phases

---

### Phase 1: Add Time-Based Info Update Infrastructure
**Goal**: Create foundation for periodic info updates during search

#### Tasks:
1. Add `lastInfoTime` member to SearchData/IterativeSearchData
2. Create `INFO_UPDATE_INTERVAL` constant (start with 100ms)
3. Add `shouldSendInfo()` method to check if update is due
4. Create `sendCurrentSearchInfo()` method for mid-search updates

#### Files to Modify:
- `/workspace/src/search/types.h` - Add timing members
- `/workspace/src/search/iterative_search_data.h` - Add info timing
- `/workspace/src/search/negamax.cpp` - Add time check logic

#### Testing:
- Verify info is sent every 100ms during long searches
- Ensure info doesn't flood output on fast searches

#### Expected ELO Impact: 0 (display only)

---

### Phase 2: Implement Currmove/Currmovenumber at Root
**Goal**: Show which move is currently being analyzed at root

#### Tasks:
1. Add `currentRootMove` and `currentRootMoveNumber` to SearchData
2. Update these values when searching root moves
3. Send currmove info when time threshold met (e.g., > 3 seconds)
4. Format: `info currmove <move> currmovenumber <num>`

#### Files to Modify:
- `/workspace/src/search/types.h` - Add currmove tracking
- `/workspace/src/search/negamax.cpp` - Track and send currmove at root

#### Testing:
- Verify currmove appears in long searches
- Check move counter increments correctly

#### Expected ELO Impact: 0 (display only)

---

### Phase 3: Add UCI Eval Command
**Goal**: Implement static evaluation display for debugging

#### Tasks:
1. Add `handleEval()` method to UCIEngine
2. Call static evaluation on current position
3. Display breakdown of evaluation components
4. Format output similar to Stockfish's eval command

#### Files to Modify:
- `/workspace/src/uci/uci.h` - Add handleEval declaration
- `/workspace/src/uci/uci.cpp` - Implement eval command
- `/workspace/src/evaluation/evaluate.cpp` - Add detailed eval output option

#### Testing:
- Test eval command in various positions
- Verify output is human-readable

#### Expected ELO Impact: 0 (debugging feature)

---

### Phase 4: Add Hashfull Reporting
**Goal**: Report transposition table usage percentage

#### Tasks:
1. Add `getHashfull()` method to TranspositionTable
2. Calculate percentage of occupied entries (per mil)
3. Include hashfull in info output
4. Format: `hashfull <permil>` (e.g., 523 = 52.3% full)

#### Files to Modify:
- `/workspace/src/core/transposition_table.h` - Add hashfull method
- `/workspace/src/core/transposition_table.cpp` - Implement calculation
- `/workspace/src/search/negamax.cpp` - Include in info output

#### Testing:
- Verify hashfull increases during search
- Check calculation accuracy

#### Expected ELO Impact: 0 (display only)

---

### Phase 5: Structured Info Building System
**Goal**: Create clean, modular info string building

#### Tasks:
1. Create InfoBuilder class with append methods:
   - `appendDepth(depth, seldepth)`
   - `appendScore(score, alpha, beta)`
   - `appendNodes(nodes, elapsed)`
   - `appendPV(moves)`
   - `appendHashfull(permil)`
   - `appendCurrmove(move, number)`
2. Replace current info output with builder pattern
3. Add bounds info (lowerbound/upperbound) for fail high/low

#### Files to Create:
- `/workspace/src/uci/info_builder.h`
- `/workspace/src/uci/info_builder.cpp`

#### Testing:
- Verify all info fields present
- Check formatting consistency

#### Expected ELO Impact: 0 (refactoring)

---

### Phase 6: Smart Update Throttling
**Goal**: Balance info frequency for different scenarios

#### Tasks:
1. Implement adaptive update intervals:
   - Fast updates (50ms) for first 1 second
   - Medium updates (200ms) for 1-10 seconds  
   - Slow updates (1000ms) for > 10 seconds
2. Add minimum node count between updates
3. Force update on significant score changes
4. Always send info at iteration completion

#### Files to Modify:
- `/workspace/src/search/negamax.cpp` - Adaptive timing logic

#### Testing:
- Test with bullet, blitz, and long time controls
- Verify GUI responsiveness

#### Expected ELO Impact: 0 (display only)

---

### Phase 7: MultiPV Support (Optional, Future)
**Goal**: Support multiple principal variations for analysis

#### Tasks:
1. Add MultiPV UCI option
2. Track N best moves at root
3. Send info for each PV line
4. Sort by score before output

#### Note: This is a larger feature for future consideration

---

## Success Criteria

1. **GUI Compatibility**: HIARCS and other GUIs show continuous analysis
2. **Update Frequency**: Info sent at appropriate intervals (not flooding)
3. **Completeness**: All standard UCI info fields present
4. **Performance**: No measurable slowdown in search speed
5. **Debugging**: Eval command works for position analysis

## Testing Plan

### Manual Testing:
1. Test in HIARCS GUI - verify live analysis display
2. Test in Arena GUI - check all info fields visible
3. Test in command line - ensure readable output
4. Long analysis test - verify updates continue

### Automated Testing:
1. Bench should be unchanged (no ELO impact)
2. Perft should be unchanged
3. Time control tests - verify no timeouts

## Implementation Order

Start with Phase 1-2 for immediate GUI improvement, then proceed through phases sequentially. Each phase should be tested and committed separately.

## Notes

- All phases should maintain backward compatibility
- No changes to search algorithm or evaluation
- Focus on display/debugging features only
- Keep UCI protocol compliance as top priority
- Reference Stash's implementation for best practices

## Status Tracking

| Phase | Status | Branch | Commit | Testing |
|-------|--------|--------|--------|---------|
| Phase 1 | **Completed** | feature/20250827-uci-info | 9a14934 | ✅ Bench unchanged (19191913) |
| Phase 2 | **Completed** | feature/20250827-uci-info | 7c7a684 | ✅ Bench unchanged (19191913), currmove verified |
| Phase 3 | **Completed** | feature/20250827-uci-info | a8719b0 | ✅ Bench unchanged (19191913), eval command working |
| Phase 4 | **Completed** | feature/20250827-uci-info | 379fb1e | ✅ Bench unchanged (19191913), hashfull working (0-121 permil observed) |
| Phase 5 | **Completed** | feature/20250827-uci-info | 508ee04 | ✅ Bench unchanged (19191913), InfoBuilder working |
| Phase 6 | Not Started | - | - | - |
| Phase 7 | Future | - | - | - |

Last Updated: 2025-08-27

## Implementation Notes

### Phase 1 Completed
- Added `m_lastInfoTime` and `INFO_UPDATE_INTERVAL` (100ms) to `IterativeSearchData`
- Implemented `shouldSendInfo()` and `recordInfoSent()` methods
- Created `sendCurrentSearchInfo()` function for mid-search updates
- Added periodic check every 4096 nodes at root (ply==0) using dynamic_cast
- Made `SearchData` polymorphic (added virtual destructor) to enable runtime type checking

### Critical Bug Fixed During Phase 1
**Issue:** Infinite search (`go infinite`) was stopping after depth 1
**Root Cause:** Time management was incorrectly evaluating time limits for infinite searches
**Fixes Applied:**
1. Added `!limits.infinite` guard before time management checks in `searchIterativeTest()`
2. Fixed overflow issues in `calculateTimeLimits()` when handling max milliseconds
3. Added protection in `shouldStopOnTime()` to return false for infinite time
4. Made `SearchData` polymorphic to enable `dynamic_cast` for periodic updates

**Testing:** Verified infinite search now continues indefinitely until stopped