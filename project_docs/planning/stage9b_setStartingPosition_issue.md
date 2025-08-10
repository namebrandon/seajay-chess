# Stage 9b - setStartingPosition Hang Issue Report

**Document Type:** Bug Analysis Report
**Date:** August 10, 2025
**Stage:** 9b - Draw Detection and Repetition Handling
**Severity:** CRITICAL - Blocks all testing
**Status:** Unresolved

## Executive Summary

During Stage 9b implementation, after applying architectural improvements from the cpp-pro agent (SearchHistory class, MoveGuard RAII pattern, thread-local storage), the `Board::setStartingPosition()` method began hanging indefinitely. The hang prevents all testing and validation of the Stage 9b implementation.

## Issue Description

### Symptom
- Calling `board.setStartingPosition()` hangs indefinitely
- No CPU usage during hang (suggests deadlock or infinite wait)
- Hang occurs BEFORE entering the `setStartingPosition()` function
- Debug output placed at function entry never executes

### Affected Code
```cpp
// Test code that hangs:
Board board;  // Works
board.setStartingPosition();  // Hangs before entering function
```

## Investigation Timeline

### 1. Initial Discovery
- Discovered when running `/workspace/tests/test_repetition.cpp`
- Test hung on basic threefold repetition test

### 2. Isolation Attempts
```cpp
// test_startpos.cpp
Board board;  // Succeeds
std::cout << "Before call" << std::endl;  // Prints
board.setStartingPosition();  // Hangs here
std::cout << "After call" << std::endl;  // Never reached
```

### 3. Debug Instrumentation
Added debug output to `Board::setStartingPosition()`:
```cpp
void Board::setStartingPosition() {
    std::cerr << "DEBUG: entered" << std::endl;  // Never prints!
    fromFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
}
```

### 4. Function Call Analysis
- `setStartingPosition()` calls `fromFEN()`
- Added debug to `fromFEN()` - also never reached
- Hang occurs BEFORE function entry

### 5. Historical Testing
```bash
git stash  # Stash our changes
./test_startpos  # STILL HANGS with original code!
```
This suggests either:
- Build system corruption
- Test compilation issue
- System-level problem

## Code Changes from cpp-pro Agent

### 1. SearchHistory Class Addition
```cpp
// Added to board.h
class SearchHistory {
    std::vector<Hash> m_history;
    size_t m_currentPly;
    // ... methods ...
};
```

### 2. Thread-Local Storage
```cpp
// Added to board.h
static thread_local std::unique_ptr<SearchHistory> t_searchHistory;
static std::mutex s_searchHistoryMutex;
```

### 3. MoveGuard RAII Pattern
```cpp
// Added to board.h
class MoveGuard {
    Board& m_board;
    Move m_move;
    CompleteUndoInfo m_undo;
    bool m_committed = false;
    // ... RAII implementation ...
};
```

## Potential Causes

### 1. Static Initialization Order Fiasco
- Thread-local variables might be initializing in wrong order
- Mutex initialization could be problematic
- Global constructors detected in binary:
  ```
  _GLOBAL__sub_I__ZN6seajay5Board15s_zobristPiecesE
  _GLOBAL__sub_I__ZN6seajay19BoardStateValidator...
  ```

### 2. Thread-Local Storage Issues
- `thread_local std::unique_ptr<SearchHistory>` might have initialization problems
- Mutex lock in `getThreadLocalSearchHistory()` could deadlock

### 3. Compiler/Linker Issue
- Mixing object files compiled at different times
- ABI incompatibility between compilation units

### 4. Memory Corruption
- Stack corruption from earlier operations
- Heap corruption affecting vtable or function pointers

## Tested Scenarios

### Working:
- `Board board;` - Construction succeeds
- `board.clearGameHistory()` - Works
- `board.isFiftyMoveRule()` - Works
- `board.isRepetitionDraw()` - Works

### Hanging:
- `board.setStartingPosition()` - Hangs
- `board.fromFEN(any_fen)` - Presumably hangs (not directly tested)

## Workaround Attempts

### 1. Clean Rebuild
```bash
make clean && make -j$(nproc)
```
Result: Issue persists

### 2. Remove Thread-Local
Not attempted due to extensive changes required

### 3. Revert All Changes
```bash
git stash
```
Result: Test STILL hangs even with original code!

## Impact Analysis

### Immediate Impact:
- Cannot run any tests requiring board setup
- Cannot validate Stage 9b implementation
- Cannot run SPRT validation
- Development blocked

### Code Affected:
- All tests using `setStartingPosition()`
- UCI protocol (uses `fromFEN`)
- Any code path requiring FEN parsing

## Recommendations

### Short-term (Immediate):
1. **Clean build from scratch:**
   ```bash
   rm -rf build
   mkdir build && cd build
   cmake .. && make -j$(nproc)
   ```

2. **Test with minimal example:**
   ```cpp
   // Minimal test without any Stage 9b code
   #include "src/core/board.h"
   int main() {
       seajay::Board b;
       b.setStartingPosition();
       return 0;
   }
   ```

3. **Bisect the issue:**
   - Revert cpp-pro changes incrementally
   - Identify which specific change causes the hang

### Medium-term:
1. **Remove thread-local storage** - Not needed for single-threaded engine
2. **Simplify architecture** - Remove SearchHistory class, use simple vector
3. **Add timeout tests** to CI to catch hangs early

### Long-term:
1. **Implement proper RAII** without thread-local complexity
2. **Add deadlock detection** in debug builds
3. **Create integration tests** that run with timeout

## Code to Revert

If reverting is necessary, these are the key changes to undo:

1. Remove from `board.h`:
   - SearchHistory class (lines 22-69)
   - MoveGuard class (lines 288-315)
   - Thread-local declarations (lines 263-264)

2. Remove from `board.cpp`:
   - Thread-local definitions (lines 1737-1752)
   - SearchHistory methods (lines 1720-1733)

3. Restore original `isRepetitionDraw()` implementation

## Test Cases for Validation

Once issue is resolved, validate with:

```cpp
// Test 1: Basic startup
Board board;
board.setStartingPosition();
assert(board.toFEN() == "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");

// Test 2: Repetition detection
// ... (test code from test_repetition.cpp)

// Test 3: Thread safety (if keeping thread-local)
std::thread t1([]{ Board b; b.setStartingPosition(); });
std::thread t2([]{ Board b; b.setStartingPosition(); });
t1.join(); t2.join();
```

## Lessons Learned

1. **Incremental testing:** Test after each architectural change
2. **Avoid premature optimization:** Thread-local not needed for single-threaded engine
3. **Simple first:** Basic implementation before advanced patterns
4. **Timeout guards:** All tests should have timeouts to detect hangs

## Current State

As of August 10, 2025:
- Core Stage 9b functionality implemented (repetition, 50-move, insufficient material)
- Architectural improvements cause initialization hang
- Unable to run tests to validate implementation
- Requires immediate resolution to proceed



---

*This issue is blocking Stage 9b completion. Resolution is highest priority.*
