# SEE (Static Exchange Evaluation) Remediation Plan

Author: SeaJay Development Team
Status: Draft for Review
Date: 2025-01-02

## Executive Summary

Static Exchange Evaluation (SEE) is a critical component for modern chess engines, enabling intelligent capture pruning and move ordering. Our analysis reveals that SeaJay's SEE implementation is complete but entirely disconnected from the search, rendering it ineffective. Additionally, the SEE pruning logic contains a fundamental flaw in its depth tracking. This document outlines a comprehensive plan to remediate these issues while ensuring thread safety for future LazySMP implementation.

## Issues Identified

### Issue 1: SEEMode UCI Option is Dead Code
**Severity:** Critical
**Location:** `src/search/negamax.cpp:98-126`, `src/search/quiescence.cpp:235-236`

**Current State:**
- UCI sets `search::g_seeMoveOrdering` via `SEEMode` option (`src/uci/uci.cpp:820-846`)
- Both negamax and quiescence exclusively use `MvvLvaOrdering` for move ordering
- `SEEMoveOrdering::orderMoves()` is never called anywhere in the codebase

**Impact:**
- SEE-based move ordering is completely non-functional
- Changing SEEMode has zero effect on engine behavior
- Wasted SEE calculations in shadow/testing modes

### Issue 2: SEEPruning Uses Global Ply Instead of Quiescence Depth
**Severity:** Critical
**Location:** `src/search/quiescence.cpp:326`, `355-362`

**Current State:**
```cpp
// Line 326: Using global ply for pruning threshold
int depthBonus = (ply / 2) * 25;

// Lines 355-362: Using global ply for equal exchange pruning
if (ply >= 7) { pruneEqual = true; }
else if (ply >= 5) { ... }
else if (ply >= 3) { ... }
```

**Impact:**
- At iteration depth 10, quiescence starts with ply=10, making pruning overly aggressive
- Pruning aggressiveness scales with main search depth, not quiescence depth
- Can miss critical tactics at higher search depths
- Inconsistent pruning behavior across different search depths

### Issue 3: No SEE Integration in Quiescence Move Ordering
**Severity:** Medium
**Location:** `src/search/quiescence.cpp:235-236`

**Current State:**
- Quiescence uses only MVV-LVA ordering for captures
- SEE information available but unused for ordering

**Impact:**
- Suboptimal move ordering in tactical sequences
- More nodes searched due to examining bad captures first

### Issue 4: Thread Safety Concerns for LazySMP
**Severity:** Medium (Future-facing)
**Location:** `src/search/move_ordering.cpp:521-526`, `src/core/see.cpp:76-82`

**Current State:**
- `SEEMoveOrdering` has a mutable log file handle (not thread-safe)
- SEE cache is shared; `SEECalculator` keeps a non-atomic `m_currentAge` and per-entry `age`
- Global `g_seeMoveOrdering` and global `g_seeCalculator` would be shared across threads

**Impact:**
- Will cause race conditions when implementing LazySMP
- File I/O conflicts in testing mode
- Cache thrashing between threads; potential false sharing and torn writes on ages

## Proposed Solutions

### Solution 1: Wire SEEMode into Search

#### 1A. Modify negamax.cpp orderMoves function (in-place, no extra allocations)
**File:** `src/search/negamax.cpp`
**Lines:** 98-139

```cpp
static void orderMoves(Board& board, MoveList& moves, Move ttMove, 
                      SearchData* searchData, int ply, Move prevMove,
                      int countermoveBonus, const SearchLimits* limits, int depth) {
    const bool useSEEForCaptures = (search::g_seeMoveOrdering.getMode() != search::SEEMode::OFF) && depth >= 3;

    static MvvLvaOrdering mvvLva;

    if (useSEEForCaptures) {
        // Stable-partition: [captures/promos | quiets]
        auto captureEnd = std::stable_partition(moves.begin(), moves.end(), [](const Move& m){
            return isCapture(m) || isPromotion(m) || isEnPassant(m);
        });

        // Sort only the captures prefix via SEE, leave quiets as-is
        if (captureEnd != moves.begin()) {
            MoveList prefix; prefix.assign(moves.begin(), captureEnd);
            search::g_seeMoveOrdering.orderMoves(board, prefix);
            std::copy(prefix.begin(), prefix.end(), moves.begin());
        }

        // Order remaining quiets (and re-insert killers/history) with existing logic
        if (searchData && searchData->killers && searchData->history && searchData->counterMoves) {
            mvvLva.orderMovesWithHistory(board, moves, *searchData->killers, *searchData->history,
                                         *searchData->counterMoves, prevMove, ply, countermoveBonus);
        } else {
            mvvLva.orderMoves(board, moves);
        }
    } else {
        // Existing MVV-LVA + killers/history path
        if (searchData && searchData->killers && searchData->history && searchData->counterMoves) {
            mvvLva.orderMovesWithHistory(board, moves, *searchData->killers, *searchData->history,
                                         *searchData->counterMoves, prevMove, ply, countermoveBonus);
        } else {
            mvvLva.orderMoves(board, moves);
        }
    }

    // TT move ordering remains unchanged
    if (ttMove != NO_MOVE) {
        auto it = std::find(moves.begin(), moves.end(), ttMove);
        if (it != moves.end() && it != moves.begin()) {
            Move tmp = *it;
            std::move_backward(moves.begin(), it, it + 1);
            *moves.begin() = tmp;
        }
    }
}
```

#### 1B. Modify quiescence.cpp for SEE ordering (captures prefix only)
**File:** `src/search/quiescence.cpp`
**Lines:** 235-236

```cpp
// Replace lines 235-236
bool useSEE = (search::g_seeMoveOrdering.getMode() != search::SEEMode::OFF);

if (useSEE && !isInCheck) {
    // Use SEE only for capture ordering; keep quiets untouched
    auto captureEnd = std::stable_partition(moves.begin(), moves.end(), [](const Move& m){
        return isCapture(m) || isPromotion(m) || isEnPassant(m);
    });
    if (captureEnd != moves.begin()) {
        MoveList prefix; prefix.assign(moves.begin(), captureEnd);
        search::g_seeMoveOrdering.orderMoves(board, prefix);
        std::copy(prefix.begin(), prefix.end(), moves.begin());
    }
} else {
    // Fallback to MVV-LVA (or use for in-check positions)
    MvvLvaOrdering mvvLva;
    mvvLva.orderMoves(board, moves);
}
```

### Solution 2: Fix SEEPruning Depth Tracking

#### 2A. Add quiescence depth parameter
**File:** `src/search/quiescence.h`
**Line:** Add new parameter

```cpp
eval::Score quiescence(
    Board& board,
    int ply,
    int qply,  // NEW: quiescence-local depth (starts at 0)
    eval::Score alpha,
    eval::Score beta,
    // ... rest of parameters
);
```

#### 2B. Update quiescence.cpp to use qply
**File:** `src/search/quiescence.cpp`

```cpp
// Line 326: Use qply instead of ply
int depthBonus = (qply / 2) * 25;  // was: (ply / 2) * 25

// Lines 355-362: Use qply for equal exchange pruning
if (qply >= 7) { pruneEqual = true; }
else if (qply >= 5) { ... }
else if (qply >= 3) { ... }

// Cap aggressiveness: never exceed -25 overall to avoid pruning good trades
pruneThreshold = std::max(pruneThreshold, -25);

// Recursive call increments qply
score = -quiescence(board, ply + 1, qply + 1, -beta, -alpha, ...);
```

#### 2C. Update negamax to pass qply=0
**File:** `src/search/negamax.cpp`
**Line:** 224

```cpp
return quiescence(board, ply, 0, alpha, beta, searchInfo, info, limits, *tt, 0, inPanicMode);
//                            ^^^ qply starts at 0
```

### Solution 3: Thread Safety for LazySMP

#### 3A. Make SEEMoveOrdering thread-safe
**File:** `src/search/move_ordering.h/cpp`

```cpp
class SEEMoveOrdering {
private:
    // Remove mutable log file
    // mutable std::ofstream m_logFile;  // REMOVE or guard behind DEBUG and thread-local buffering
    
    // Add thread-local storage for statistics
    struct ThreadLocalStats {
        ComparisonStats stats;
        std::vector<std::string> logBuffer;  // Buffer logs instead of file I/O
    };
    
    static thread_local ThreadLocalStats t_stats;  // thread-local for SMP
    
    // Use thread-local SEE calculator to avoid shared cache contention
    static thread_local SEECalculator t_see;
    SEECalculator& see() const noexcept { return t_see; }
    
public:
    // Remove global instance, use thread-local or pass by parameter
    // static SEEMoveOrdering g_seeMoveOrdering;  // REMOVE
};

// In search code, either:
// Option 1: Thread-local instance
static thread_local SEEMoveOrdering t_seeMoveOrdering;

// Option 2: Pass as parameter through search
void negamax(..., SEEMoveOrdering& seeOrdering, ...) { }
```

#### 3B. Make SEE cache thread-aware
**File:** `src/core/see.h/cpp`

```cpp
class SEECalculator {
private:
    // Preferred: make the entire calculator thread-local, including cache state and ages
    // If a shared cache is kept, make m_currentAge atomic and align entries to 64B to reduce false sharing
    alignas(64) struct CacheEntry {
        std::atomic<uint64_t> key{0};
        std::atomic<SEEValue> value{0};
        std::atomic<uint8_t> age{0};
        char pad[53];
    };
};
```

#### 3C. Global free-function wrappers must be thread-local
**File:** `src/core/see.h`

```cpp
// Replace inline global instance with thread-local to be SMP-safe
// inline SEECalculator g_seeCalculator;  // REMOVE
static thread_local SEECalculator g_seeCalculator;  // NEW
```

### Solution 4: Configuration Management

#### 4A. Create search configuration struct
**File:** `src/search/types.h`

```cpp
struct SearchConfig {
    // Move ordering
    SEEMode seeMode = SEEMode::OFF;
    bool useSEEInQuiescence = false;
    int seeMinDepth = 3;              // Minimum depth to use SEE
    int seeMaxCapturesPrefix = 16;    // Limit SEE sorting to top-K captures (perf guard)
    
    // Pruning
    SEEPruningMode seePruningMode = SEEPruningMode::CONSERVATIVE;
    bool useQplyForPruning = true;   // NEW: flag to use fixed version
    int seeEqualPruneQply = 5;       // Start equal-exchange pruning at qply >= N
    
    // Thread safety
    bool useThreadLocalSEE = false;  // For LazySMP preparation
};
```

## Implementation Plan

### Phase 1: Fix Critical Issues (Immediate)
1. **Week 1:** Implement Solution 2 (Fix SEEPruning depth tracking)
   - Add qply parameter
   - Update all quiescence calls
   - Test for tactical regressions

2. **Week 1-2:** Implement Solution 1A (Wire SEEMode in negamax, in-place prefix sort)
   - Add conditional SEE ordering
   - Preserve existing quiet move ordering
   - Validate with perft and bench

3. **Week 2:** Implement Solution 1B (Wire SEEMode in quiescence, captures prefix only)
   - Add SEE ordering option
   - Test tactical suite

### Phase 2: Testing and Validation
4. **Week 3:** Comprehensive testing
   - Run tactical test suites
   - Compare node counts at fixed depth
   - OpenBench SPRT tests as originally planned in Phase B2

### Phase 3: Thread Safety (Future)
5. **Later:** Implement Solution 3 when preparing for LazySMP
   - Convert to thread-local storage
   - Validate with thread sanitizer
   - Benchmark cache hit rates

## Testing Strategy

### Unit Tests
```cpp
// Test SEE ordering is actually called
TEST(SEEOrdering, IsCalledWhenEnabled) {
    // Set SEEMode to production
    // Verify orderMoves was called via mock/spy
}

// Test qply tracking
TEST(Quiescence, QplyStartsAtZero) {
    // Verify qply=0 at quiescence entry
    // Verify qply increments on recursion
}
```

### Integration Tests
1. **Fixed position tests:**
   - Position with many captures
   - Measure nodes with SEE on/off; track SEE cache hit rate
   - Verify node reduction

2. **Tactical suite:**
   - WAC, ECM, or similar
   - Verify no tactical regressions
   - Compare solve times

### Performance Tests
```bash
# Node count comparison at fixed depth
echo "position startpos moves e2e4 e7e5 d2d4 e5d4" | ./seajay
echo "go depth 10" | ./seajay  # Record nodes with SEE off
echo "setoption name SEEMode value production" | ./seajay
echo "go depth 10" | ./seajay  # Record nodes with SEE on

# Also test SEEPruning qply fix
echo "setoption name SEEPruning value aggressive" | ./seajay
echo "go depth 10" | ./seajay  # Compare tactical solve rate before/after qply fix
```

## Success Metrics

1. **Functional:** SEEMode changes actually affect move ordering
2. **Correctness:** No tactical regressions with fixed qply
3. **Performance:** Measurable node reduction (target 10–20%) with SEE ordering enabled on tactical positions
4. **Efficiency:** <5% time overhead at depth 10; SEE cache hit rate >50% in hot nodes
5. **Thread Safety:** No race conditions detected by thread sanitizer

## Risk Mitigation

1. **Tactical Blindness:** Extensive tactical testing before deployment
2. **Performance Regression:** Profile SEE calculation overhead
   - Guard with `seeMaxCapturesPrefix` and `seeMinDepth`; disable SEE ordering at shallow depths or when few captures
3. **Compatibility:** Maintain backward compatibility with UCI options
4. **Thread Safety:** Incremental approach, test single-threaded first

## Timeline

- **Week 1:** Core fixes (Solutions 1-2)
- **Week 2:** Testing and validation
- **Week 3:** OpenBench SPRT tests
- **Future:** Thread safety when implementing LazySMP

## Appendix: Code Locations

Key files and line numbers for reference:
- `src/search/negamax.cpp:98-139` - orderMoves function
- `src/search/quiescence.cpp:35-45` - quiescence signature
- `src/search/quiescence.cpp:235-236` - capture ordering
- `src/search/quiescence.cpp:326,355-362` - ply usage for pruning
- `src/core/see.h` - global SEE instance; make thread_local
- `src/uci/uci.cpp:820-846` - SEEMode UCI handling
- `src/search/move_ordering.cpp:666-693` - SEEMoveOrdering::orderMoves
- `src/core/see.cpp` - SEE implementation
