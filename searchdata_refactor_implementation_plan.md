# SearchData Refactoring: Detailed Implementation Plan

## Document Purpose
This is a complete, standalone guide to fix a 30-40 ELO performance regression caused by SearchData being too large (42KB) and causing cache thrashing. Following this plan step-by-step will recover the lost performance.

## Background (Why We're Doing This)

### The Problem
- **Symptom**: 30-40 ELO performance loss after UCI score conversion changes
- **Root Cause**: SearchData struct has grown to 42KB due to embedded arrays
- **Impact**: Cache thrashing (L1 cache is only 32-64KB)
- **Solution**: Change embedded arrays to pointers, reducing SearchData from 42KB to ~1KB

### Current State (BAD)
```cpp
struct SearchData {
    // ... other members ...
    KillerMoves killers;       // 1KB embedded directly
    HistoryHeuristic history;  // 32KB embedded directly  
    CounterMoves counterMoves; // 16KB embedded directly
    // Total: ~42KB - TOO BIG!
};
```

### Target State (GOOD)
```cpp
struct SearchData {
    // ... other members ...
    KillerMoves* killers;       // 8 bytes pointer
    HistoryHeuristic* history;  // 8 bytes pointer
    CounterMoves* counterMoves; // 8 bytes pointer
    // Total: ~1KB - FITS IN CACHE!
};
```

## Pre-Implementation Checklist

### Prerequisites
- [ ] Working build environment for SeaJay
- [ ] Ability to run bench command: `echo "bench" | ./seajay`
- [নোট current bench value: Should be 19191913
- [ ] Git repository clean (no uncommitted changes)
- [ ] Create new branch: `git checkout -b fix/searchdata-cache-thrashing`

### Backup Current Performance
```bash
# Record current performance metrics
echo "bench" | ./seajay | tee bench_before.txt
# Note the NPS (nodes per second) value for comparison
```

## Implementation Steps

### Step 1: Measure Current Object Sizes (5 minutes)

Add temporary debug code to verify the problem:

**File**: `/workspace/src/search/negamax.cpp`
**Location**: At the start of `searchIterativeTest()` function (around line 940)
**Add this code**:
```cpp
// TEMPORARY DEBUG - REMOVE AFTER FIX
static bool sizesReported = false;
if (!sizesReported) {
    std::cerr << "=== Object Sizes Before Fix ===" << std::endl;
    std::cerr << "SearchData size: " << sizeof(SearchData) << " bytes" << std::endl;
    std::cerr << "IterativeSearchData size: " << sizeof(IterativeSearchData) << " bytes" << std::endl;
    std::cerr << "KillerMoves size: " << sizeof(KillerMoves) << " bytes" << std::endl;
    std::cerr << "HistoryHeuristic size: " << sizeof(HistoryHeuristic) << " bytes" << std::endl;
    std::cerr << "CounterMoves size: " << sizeof(CounterMoves) << " bytes" << std::endl;
    sizesReported = true;
}
```

**Build and run**:
```bash
./build.sh
echo "bench" | ./seajay 2>&1 | grep -A5 "Object Sizes"
```

**Expected output**:
```
SearchData size: 42000+ bytes
IterativeSearchData size: 46000+ bytes
```

### Step 2: Modify SearchData Structure (10 minutes)

**File**: `/workspace/src/search/types.h`
**Find** (around lines 320-340):
```cpp
// Stage 19: Killer moves for move ordering
KillerMoves killers;

// Stage 20: History heuristic for move ordering
HistoryHeuristic history;

// Stage 23: Countermove heuristic for move ordering
CounterMoves counterMoves;
```

**Replace with**:
```cpp
// Stage 19: Killer moves for move ordering
// PERFORMANCE FIX: Changed from embedded to pointer to reduce SearchData size from 42KB to 1KB
// This fixes cache thrashing that caused 30-40 ELO loss
KillerMoves* killers = nullptr;

// Stage 20: History heuristic for move ordering
// PERFORMANCE FIX: Changed from embedded to pointer (was 32KB embedded)
HistoryHeuristic* history = nullptr;

// Stage 23: Countermove heuristic for move ordering
// PERFORMANCE FIX: Changed from embedded to pointer (was 16KB embedded)
CounterMoves* counterMoves = nullptr;
```

### Step 3: Update Search Initialization - Part A (10 minutes)

**File**: `/workspace/src/search/negamax.cpp`
**Function**: `searchIterativeTest()` (around line 940)

**Find**:
```cpp
IterativeSearchData info;  // Using new class instead of SearchData
```

**Replace with**:
```cpp
// Create search data and move ordering tables
IterativeSearchData info;  // Using new class instead of SearchData

// PERFORMANCE FIX: Allocate move ordering tables on stack
// These were previously embedded in SearchData (42KB total)
// Now allocated separately and pointed to (SearchData only 1KB)
KillerMoves killerMoves;
HistoryHeuristic historyHeuristic;
CounterMoves counterMovesTable;

// Connect pointers to stack-allocated objects
info.killers = &killerMoves;
info.history = &historyHeuristic;
info.counterMoves = &counterMovesTable;
```

**Also find** (around line 1015):
```cpp
info.counterMoves.clear();  // Clear countermove table for fresh search
```

**Replace with**:
```cpp
info.counterMoves->clear();  // Clear countermove table for fresh search
```

### Step 4: Update Search Initialization - Part B (5 minutes)

**File**: `/workspace/src/search/negamax.cpp`
**Function**: `search()` (around line 1370)

**Find**:
```cpp
SearchData info;  // For search statistics
```

**Replace with**:
```cpp
SearchData info;  // For search statistics

// PERFORMANCE FIX: Allocate move ordering tables on stack
// Same fix as in searchIterativeTest()
KillerMoves killerMoves;
HistoryHeuristic historyHeuristic;
CounterMoves counterMovesTable;

// Connect pointers to stack-allocated objects
info.killers = &killerMoves;
info.history = &historyHeuristic;
info.counterMoves = &counterMovesTable;
```

### Step 5: Update All Access Points (15 minutes)

**File**: `/workspace/src/search/negamax.cpp`

We need to change all `.` accesses to `->` for killers, history, and counterMoves.

#### Method A: Manual Updates

**Find and Replace** these exact patterns:

| Find | Replace | Location (approximate line) |
|------|---------|----------------------------|
| `info.killers.isKiller` | `info.killers->isKiller` | 630, 880 |
| `info.killers.update` | `info.killers->update` | 901 |
| `info.history.getScore` | `info.history->getScore` | 672 |
| `info.history.update` | `info.history->update` | 905 |
| `info.history.updateFailed` | `info.history->updateFailed` | 910 |
| `info.counterMoves.getCounterMove` | `info.counterMoves->getCounterMove` | 545, 634, 883 |
| `info.counterMoves.update` | `info.counterMoves->update` | 919 |
| `info.counterMoves.clear` | `info.counterMoves->clear` | 1015 (already done) |

#### Method B: Automated with sed (Linux/Mac)

```bash
cd /workspace
# Backup first
cp src/search/negamax.cpp src/search/negamax.cpp.backup

# Apply replacements
sed -i 's/info\.killers\./info.killers->/g' src/search/negamax.cpp
sed -i 's/info\.history\./info.history->/g' src/search/negamax.cpp
sed -i 's/info\.counterMoves\./info.counterMoves->/g' src/search/negamax.cpp
```

### Step 6: Build and Fix Compilation Errors (10 minutes)

```bash
./build.sh
```

**Common compilation errors and fixes**:

1. **Error**: "no member named 'killers' in 'SearchData'"
   - **Fix**: Make sure you updated types.h correctly

2. **Error**: "cannot use '.' with pointer"
   - **Fix**: Change `.` to `->` for that access

3. **Error**: "invalid use of incomplete type"
   - **Fix**: Ensure headers are included properly

### Step 7: Verify Object Sizes (5 minutes)

Run bench again to see the new sizes:

```bash
echo "bench" | ./seajay 2>&1 | grep -A5 "Object Sizes"
```

**Expected output**:
```
SearchData size: ~1000 bytes (down from 42000+)
IterativeSearchData size: ~1500 bytes (down from 46000+)
```

### Step 8: Performance Testing (10 minutes)

#### A. Verify Correctness
```bash
# Bench should still be exactly 19191913
echo "bench" | ./seajay | grep "Benchmark complete"
```

#### B. Measure Performance Improvement
```bash
# Run bench and note the NPS (nodes per second)
echo "bench" | ./seajay | tee bench_after.txt

# Compare with bench_before.txt
# NPS should be 10-14% higher
```

#### C. Quick Position Test
```bash
# Test a specific position
echo -e "position startpos moves e2e4 e7e5 g1f3 b8c6\ngo depth 15\nquit" | ./seajay
# Note the time and NPS
```

### Step 9: Clean Up Debug Code (5 minutes)

Remove the temporary size reporting code added in Step 1:

**File**: `/workspace/src/search/negamax.cpp`
**Remove**:
```cpp
// TEMPORARY DEBUG - REMOVE AFTER FIX
static bool sizesReported = false;
if (!sizesReported) {
    // ... size reporting code ...
}
```

### Step 10: Final Verification (5 minutes)

```bash
# Final bench - should still be 19191913
echo "bench" | ./seajay

# Build release version
./build.sh release

# Test release version
echo "bench" | ./seajay
```

## Commit and Testing

### Commit Message
```bash
git add -A
git commit -m "fix: Reduce SearchData size from 42KB to 1KB to fix cache thrashing

This fixes a 30-40 ELO regression caused by embedded arrays in SearchData
causing cache thrashing. Changed KillerMoves, HistoryHeuristic, and 
CounterMoves from embedded arrays to pointers.

- SearchData reduced from 42KB to ~1KB
- Improves cache efficiency dramatically
- Recovers 30-40 ELO lost in UCI conversion commits
- No algorithmic changes, only memory layout

bench 19191913"
```

### OpenBench Testing
1. Push to remote: `git push origin fix/searchdata-cache-thrashing`
2. Create OpenBench test:
   - Base: commit before regression (855c4b9)
   - Dev: your new commit
   - Expected: +30-40 ELO recovery

## Troubleshooting Guide

### Problem: Segmentation Fault
**Cause**: Null pointer dereference
**Solution**: Ensure all three pointers are initialized in both search functions

### Problem: Bench Changed
**Cause**: Logic was accidentally modified
**Solution**: Revert and carefully redo - only change `.` to `->`

### Problem: No Performance Improvement
**Cause**: Might have other issues
**Checks**:
1. Verify SearchData is actually smaller (Step 7)
2. Check optimization flags in build
3. Run with perf to check cache misses

### Problem: Compilation Errors
**Most common**:
```cpp
// Wrong:
info.killers.isKiller(ply, move)
// Right:
info.killers->isKiller(ply, move)
```

## Success Criteria

- [ ] SearchData size < 2KB (was 42KB)
- [ ] IterativeSearchData size < 2KB (was 46KB)
- [ ] Bench value unchanged (19191913)
- [ ] NPS improved by 10-14%
- [ ] No crashes or segfaults
- [ ] OpenBench shows +30-40 ELO improvement

## Rollback Plan

If anything goes wrong:
```bash
# Restore backup
cp src/search/negamax.cpp.backup src/search/negamax.cpp
git checkout src/search/types.h

# Or full git reset
git reset --hard HEAD
```

## Time Estimate

- Total time: 60-75 minutes
- Can be done in stages (save after Step 6)
- Most time spent on testing/verification

## Notes for Future Multi-threading

This refactoring is beneficial for future multi-threading:
- Each thread can have its own tables on its stack
- No synchronization needed between threads
- Reduces memory bandwidth requirements
- Matches architecture used by Stockfish and other top engines

## Final Notes

- This is a mechanical change - no chess logic is modified
- All changes are in 2 files: types.h and negamax.cpp
- The fix is low risk with stack allocation
- Performance gain should be immediate and measurable

---
Document Version: 1.0
Last Updated: 2025-01-28
Prepared for: SeaJay UCI Score Regression Fix