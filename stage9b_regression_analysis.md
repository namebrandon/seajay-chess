# Stage 9b -70 Elo Regression Analysis - Living Document

**Last Updated:** August 11, 2025  
**Status:** INVESTIGATION ONGOING - Fix attempt #1 failed

## Executive Summary

**Original Hypothesis:** The -70 Elo regression was caused by vector operations in the hot path of make/unmake move functions.

**Current Status:** 
- ✅ We successfully eliminated vector operations during search
- ❌ Performance got WORSE (-127 Elo instead of -70)
- 🔍 The regression has multiple causes, not just vector operations

## Investigation Progress

### Fix Attempt #1: Remove Vector Operations (August 11, 2025)

**Branch:** `stage9b-debug/vector-ops-fix`  
**Commit:** 768f32e

**What We Did:**
1. Added `m_inSearch` flag to Board class
2. Skip `pushGameHistory()` when in search mode
3. Skip `pop_back()` when in search mode
4. Verified with instrumentation counters

**Results:**
```
SPRT Test (40 games): 
- Elo: -127 ± 80 (WORSE than original -70!)
- W/L: 6-20 (Stage 9b Fixed lost badly)
- Draw rate: 30%

Instrumentation Counters:
- Search moves: 519 ✅
- Game moves: 0 ✅
- History pushes: 0 ✅ (Vector ops eliminated!)
- History pops: 0 ✅ (Vector ops eliminated!)
```

**Conclusion:** Fix worked technically but made performance WORSE. The regression has other causes.

### Original Discovery (August 10, 2025)

Testing with draw detection ON and OFF both showed -70 Elo loss, proving the regression is NOT from draw detection logic itself, but from implementation details.

## Git Organization

### Branches
- `main` - Current development (Stage 9b with regression)
- `stage9b-debug/vector-ops-fix` - First fix attempt (failed)

### Important Tags
- `stage9-complete` (fe33035) - Stage 9 baseline, no regression
- `stage9b-regression` (136d4aa) - Stage 9b with -70 Elo regression

### Key Files for Investigation
- `/workspace/project_docs/tracking/stage9b_performance_log.md` - Detailed log of all attempts
- `/workspace/STAGE9B_GIT_STRATEGY.md` - Git workflow for debugging
- `/workspace/project_docs/tracking/deferred_items_tracker.md` - Future optimizations

### Quick Commands
```bash
# Return to Stage 9 baseline (known good)
git checkout stage9-complete

# Return to Stage 9b with regression
git checkout stage9b-regression

# Current fix attempt
git checkout stage9b-debug/vector-ops-fix

# See all debug branches
git branch | grep stage9b-debug
```

## Validation of cpp-pro's Analysis

### 1. Is the Analysis Correct?

**YES, absolutely.** Adding vector operations (push_back/pop_back) to makeMove/unmakeMove is catastrophic for search performance. Here's why:

- During a typical 4-ply search, the engine examines ~1-10 million positions
- Each position requires a makeMove/unmakeMove pair
- That means **millions of vector operations** (heap allocations, bounds checks, potential reallocations)
- Vector operations involve:
  - Heap memory access (cache-unfriendly)
  - Potential reallocation and copying when capacity exceeded
  - Bounds checking overhead
  - Memory barrier operations

A -70 Elo loss from this change is actually conservative - I'm surprised it's not worse.

## 2. How Other Engines Handle This Problem

### Stockfish's Approach
```cpp
// Stockfish uses a StateInfo stack with pre-allocated memory
StateInfo states[MAX_PLY];
StateInfo* st = &states[ply];
// No dynamic allocation during search!
```

### Ethereal's Pattern
```cpp
// Fixed-size stack allocated once
struct Thread {
    Position positions[MAX_PLY];
    int ply;
};
// Access via: thread->positions[thread->ply]
```

### Leela Chess Zero
```cpp
// Pre-allocated position history
class PositionHistory {
    std::array<uint64_t, MAX_GAME_LENGTH> history_;
    size_t game_ply_;  // Separate from search ply
};
```

### Key Pattern: **NO HEAP ALLOCATIONS IN SEARCH**

All successful engines follow this rule religiously:
- Pre-allocate all memory before search starts
- Use stack-based or array-based data structures
- Never use std::vector operations in the search loop

## 3. Standard Pattern for Separating Game vs Search Moves

The industry-standard pattern is the **Two-Phase Approach**:

### Phase 1: Game History (Root Position)
```cpp
class Board {
    // Game history - only updated for actual game moves
    std::vector<Hash> gameHistory;  // OK to use vector here
    size_t gameHistoryLength;
    
    // Called ONLY for game moves (from UCI)
    void makeGameMove(Move m) {
        gameHistory.push_back(zobristKey());
        makeMove(m);
    }
};
```

### Phase 2: Search Stack (During Search)
```cpp
struct SearchStack {
    // Pre-allocated, fixed-size array
    Hash positions[MAX_PLY];
    Move moves[MAX_PLY];
    int ply;
    
    // Zero-cost operations
    void push(Hash h) { positions[ply] = h; }
    void pop() { /* just decrement ply */ }
};
```

### The Critical Distinction
- **Game moves**: Update persistent history (rare, performance not critical)
- **Search moves**: Use pre-allocated stack (millions of times, performance critical)

## 4. Additional Issues Contributing to Regression

Looking at the code, I see several compounding issues:

### Issue A: Unconditional History Updates
```cpp
void Board::makeMoveInternal(Move move, UndoInfo& undo) {
    pushGameHistory();  // ALWAYS called, even in search!
    // ...
}
```
This happens for EVERY move in search, not just game moves.

### Issue B: Expensive Bound Checks
```cpp
void Board::pushGameHistory() {
    m_gameHistory.push_back(zobristKey());  // Heap operation
    if (m_gameHistory.size() > MAX_GAME_HISTORY) {  // Branch
        m_gameHistory.erase(...);  // VERY expensive!
    }
}
```
The erase operation is particularly catastrophic - it moves all remaining elements!

### Issue C: SearchInfo Overhead
Even when not checking for draws, SearchInfo adds:
- Function call overhead
- Extra parameter passing
- Stack memory usage
- Potential cache pollution

### Issue D: Irreversible Move Tracking
```cpp
if (capturedPiece != NO_PIECE || typeOf(movingPiece) == PAWN) {
    m_lastIrreversiblePly = m_gameHistory.size();
}
```
Extra branches and memory updates in the hot path.

## 5. Best Fix Strategy

### Immediate Fix (Minimal Changes)
```cpp
class Board {
    bool m_inSearch = false;  // Add flag
    
    void makeMoveInternal(Move move, UndoInfo& undo) {
        if (!m_inSearch) {  // Only for game moves
            pushGameHistory();
        }
        // ... rest of makeMove
    }
    
    void unmakeMoveInternal(Move move, const UndoInfo& undo) {
        // ... unmake logic
        if (!m_inSearch && !m_gameHistory.empty()) {
            m_gameHistory.pop_back();
        }
    }
};

// In search:
board.setSearchMode(true);
negamax(...);
board.setSearchMode(false);
```

### Proper Fix (Recommended)
```cpp
class Board {
    // Remove ALL history tracking from Board class
    // Board should only handle position state
    
    void makeMove(Move m, UndoInfo& u) {
        // Just update position - no history!
    }
};

class GameController {
    Board board;
    std::vector<Hash> gameHistory;  // Separate from Board
    
    void makeGameMove(Move m) {
        gameHistory.push_back(board.zobristKey());
        board.makeMove(m);
    }
};

class SearchThread {
    Board board;  // Local copy for search
    Hash searchStack[MAX_PLY];  // Pre-allocated
    
    void search() {
        // Use searchStack for repetition detection
        // Never touch gameHistory
    }
};
```

## 6. Additional Performance Issues to Fix

### Remove These from Hot Path:
1. **Vector operations** - Already discussed
2. **Material tracking updates** - Should be incremental, not recalculated
3. **Eval cache invalidation** - Should be smarter about when to invalidate
4. **Bounds checking in release builds** - Use asserts only
5. **Virtual function calls** - Ensure makeMove is not virtual

### Add These Optimizations:
1. **Prefetching** - Prefetch hash table entries
2. **Branch prediction hints** - Use [[likely]]/[[unlikely]] attributes
3. **SIMD operations** - For bitboard operations
4. **Copy-make pattern** - Sometimes faster than make/unmake

## 7. Verification Strategy

After implementing fixes:

### Performance Tests
```bash
# Measure nodes per second
./seajay perft 6 > /dev/null
# Should see 2-5M nps on modern hardware

# Profile with perf
perf record ./seajay bench
perf report
# makeMove should be <5% of runtime
```

### Elo Testing
```bash
# SPRT test against unfixed version
cutechess-cli -engine cmd=./seajay_fixed -engine cmd=./seajay_9b \
    -each proto=uci tc=10+0.1 -games 1000 -sprt elo0=0 elo1=50 \
    alpha=0.05 beta=0.05
```

## Conclusion

The cpp-pro's analysis is spot-on. This is a textbook case of what NOT to do in chess engine hot paths. The fix is straightforward:

1. **Immediate**: Add a flag to skip history updates during search
2. **Proper**: Separate game history from search completely
3. **Verify**: Ensure >50 Elo gain after fix

This regression perfectly illustrates why chess engines need specialized data structures and careful attention to the search hot path. Every instruction counts when you're executing it millions of times per second.

## Expert Recommendations

1. **Fix this immediately** - It's blocking all Stage 9b benefits
2. **Never use STL containers in search** - Pre-allocate everything
3. **Profile regularly** - Catch these issues early
4. **Study Stockfish's architecture** - It's the gold standard for a reason
5. **Consider copy-make for simple positions** - Sometimes faster than make/unmake

The -70 Elo loss confirms what every chess programmer learns: the make/unmake functions are the most performance-critical code in the entire engine. Even small inefficiencies here cascade into massive performance losses.

## Next Investigation Steps (Updated August 11, 2025)

Since removing vector operations made things WORSE (-127 Elo vs -70), we need to investigate:

### Hypothesis 1: The Fix Broke Something
- The `m_inSearch` flag might be interfering with draw detection
- Draw detection might not be working properly during search
- The fix might have introduced a logic bug

**Test:** Verify draw detection still works correctly during search

### Hypothesis 2: Multiple Performance Issues
- Vector operations were only part of the problem
- Other Stage 9b changes are also causing regression
- SearchInfo operations might be expensive

**Test:** Profile the engine to find other hot spots

### Hypothesis 3: The Original Problem Was Misidentified
- Maybe the vector operations weren't the actual bottleneck
- The regression might be from something completely different
- Need to do A/B testing of individual Stage 9b changes

**Test:** Selectively disable Stage 9b features one by one

### Immediate Action Items
1. **Profile the engine** - Use `perf` to find actual hot spots
2. **Check draw detection correctness** - Ensure it's working during search
3. **Revert fix and try different approach** - Maybe remove history tracking entirely
4. **Binary search the regression** - Find exact commit that introduced it
5. **Test with draw detection completely disabled** - Isolate the issue

### Tools and Commands
```bash
# Profile the engine
perf record ./build/seajay bench 5
perf report

# Test draw detection
echo -e "position fen 8/8/8/4k3/8/3K4/8/8 w - - 0 1\ngo depth 5\nquit" | ./build/seajay

# Compare performance
./run_stage9b_fixed_sprt.sh

# Check specific commits
git bisect start
git bisect bad stage9b-regression
git bisect good stage9-complete
```