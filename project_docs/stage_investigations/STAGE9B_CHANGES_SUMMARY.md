# Stage 9b Changes Summary - Comprehensive Analysis

**Generated:** August 11, 2025  
**Purpose:** Understand all changes between Stage 9 (fe33035) and Stage 9b (136d4aa) to identify regression sources

## Executive Summary

Stage 9b added draw detection and repetition handling, but the implementation touched many core systems. The -70 Elo regression appears to stem from multiple changes, not just the vector operations we initially suspected.

## Changes Grouped by Theme

### 1. Draw Detection Core Implementation

#### **Position History Tracking** (PRIMARY SUSPECT)
- **File:** `src/core/board.h/cpp`
- **Added:**
  - `std::vector<Hash> m_gameHistory` - Position history for repetition detection
  - `size_t m_lastIrreversiblePly` - Track last pawn/capture move
  - `pushGameHistory()` - Add position to history (vector push_back)
  - `clearGameHistory()` - Clear history
  - `isRepetitionDraw()` - Check for threefold repetition
  - `isRepetitionDrawInSearch()` - Check repetition during search
  - `isFiftyMoveRule()` - Check 50-move rule
  - `isInsufficientMaterial()` - Check for insufficient material
  - `isDrawInSearch()` - Combined draw check for search

**Impact:** Vector operations in makeMove/unmakeMove hot path

#### **Search-Specific History** 
- **File:** `src/search/search_info.h` (NEW FILE)
- **Added:**
  - `SearchInfo` class - Separate position tracking for search
  - `SearchStack` structure - Pre-allocated array for search positions
  - `isRepetitionInSearch()` - Check repetitions in search history
  - Fixed-size array `std::array<SearchStack, MAX_PLY>`

**Impact:** Additional parameter passing and memory operations

### 2. Move Making/Unmaking Changes

#### **Modified makeMove/unmakeMove**
- **File:** `src/core/board.cpp`
- **Changes:**
  - Lines 1112: Added `pushGameHistory()` call (ALWAYS called)
  - Lines 1130: Track irreversible moves
  - Lines 1527-1528: Added `pop_back()` on unmake
  - Modified both UndoInfo and CompleteUndoInfo versions

**Impact:** Every move now has vector operations

### 3. Search Algorithm Modifications

#### **Negamax Changes**
- **File:** `src/search/negamax.cpp`
- **Added:**
  - SearchInfo parameter to all functions
  - Draw detection checks (lines 107-149)
  - Strategic draw checking logic
  - Position tracking in search stack
  - Quiescence search draw handling

**Changes:**
```cpp
// Old signature
eval::Score negamax(Board& board, int depth, int ply, 
                   eval::Score alpha, eval::Score beta);

// New signature  
eval::Score negamax(Board& board, int depth, int ply,
                   eval::Score alpha, eval::Score beta, 
                   SearchInfo& searchInfo);
```

**Impact:** Extra parameter passing, draw checks in hot path

### 4. UCI Protocol Extensions

#### **New UCI Draw Integration**
- **File:** `src/uci/uci_draw_integration.cpp` (NEW FILE - 328 lines)
- **Added:**
  - Draw detection options
  - Repetition handling
  - Debug commands for draw testing
  - Test position setups

#### **UCI Command Extensions**
- **File:** `src/uci/uci.cpp`
- **Added:**
  - `setposrepeat` command
  - `testdraw` command
  - Draw-related options
  - Modified position handling

### 5. Bug Fixes During Implementation

#### **Bug #010: setStartingPosition() Hang**
- **File:** `src/core/board.cpp`
- **Fixed:**
  - Bounds checking issues
  - Material update problems
  - Include path issues
  - Assert protection

**Note:** These fixes added extra validation checks

#### **Other Fixes**
- En passant check evasion (Bug #001)
- Zobrist initialization (Bug #002)
- Various test fixes

### 6. Material and Evaluation Changes

#### **Material Class Updates**
- **File:** `src/evaluation/material.h`
- **Added:**
  - `isInsufficientMaterial()` method
  - Bounds checking in Debug builds
  - Material counting logic

#### **PST Updates**
- **File:** `src/evaluation/pst.h`
- Minor adjustments for Stage 9b compatibility

### 7. Testing Infrastructure

#### **Extensive Test Suite Added**
- `tests/test_stage9b_draws.cpp` (770 lines)
- `tests/test_draw_detection_comprehensive.cpp` (511 lines)
- `tests/test_repetition.cpp` (240 lines)
- `tests/test_uci_draws.cpp` (269 lines)
- `tests/benchmark_draws.cpp` (236 lines)

**Total:** Over 2000 lines of test code added

### 8. Build System Changes

#### **CMake Updates**
- Added GoogleTest support
- Platform-specific configurations
- New test targets

## Performance Impact Analysis

### Hot Path Changes (Most Likely Culprits)

1. **Vector Operations in makeMove/unmakeMove**
   - `pushGameHistory()` - heap allocation
   - `pop_back()` - heap deallocation
   - Size checks and bounds validation
   - **Frequency:** MILLIONS of times during search

2. **SearchInfo Parameter Passing**
   - Extra parameter in every negamax call
   - Stack operations for search history
   - Memory access patterns changed
   - **Frequency:** Every recursive call

3. **Draw Detection Checks**
   - `isDrawInSearch()` calls
   - Repetition checking logic
   - 50-move rule checks
   - Insufficient material checks
   - **Frequency:** Strategic points but still thousands of times

4. **Additional Validation**
   - Bounds checking from Bug #010 fixes
   - Material validation
   - Extra safety checks
   - **Frequency:** Every piece operation

### Secondary Changes (Possible Contributors)

5. **Memory Layout Changes**
   - Board class grew with new members
   - Cache line alignment possibly affected
   - More data to copy during make/unmake

6. **Code Size Increase**
   - More instructions in hot functions
   - Possible instruction cache pressure
   - Branch prediction affected

## Why Our Fix Made Things Worse

Our m_inSearch flag successfully eliminated vector operations, but the -127 Elo result suggests:

1. **SearchInfo overhead is significant** - Even without vector ops, the SearchInfo parameter and operations are expensive

2. **Draw detection logic is costly** - The checks themselves, not just the history tracking

3. **Multiple small overheads compound** - Many small changes together create large impact

4. **Possible logic bug introduced** - The fix might have broken something subtle

## Recommendations for Next Investigation

### Priority 1: Profile to Find Real Hot Spots
```bash
perf record -g ./build/seajay bench 5
perf report
```

### Priority 2: Selectively Disable Features
1. **Test with NO draw detection at all**
   - Comment out all draw checks
   - Remove SearchInfo parameter
   - Measure performance

2. **Test with ONLY repetition detection**
   - Keep SearchInfo but disable other draws
   - Measure impact

3. **Test with simplified SearchInfo**
   - Use smaller data structure
   - Reduce operations

### Priority 3: Check for Logic Bugs
1. Verify draw detection works correctly during search
2. Check if positions are being misevaluated
3. Ensure search isn't terminating early

## Conclusion

Stage 9b made extensive changes beyond just adding draw detection. The regression appears to be caused by:
1. Vector operations (partially addressed)
2. SearchInfo overhead (not addressed)
3. Draw detection logic (not addressed)
4. Cumulative small overheads (not addressed)
5. Possible logic bugs (not investigated)

The fact that draw detection ON/OFF both show -70 Elo suggests the overhead is in the infrastructure, not the detection logic itself.