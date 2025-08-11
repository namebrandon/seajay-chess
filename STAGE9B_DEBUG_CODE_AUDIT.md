# Stage 9b Debug Code Audit

**Date:** August 11, 2025  
**Purpose:** Identify and document all debug/instrumentation code in hot paths

## 🔴 CRITICAL FINDINGS - Code in Hot Path

### 1. **Instrumentation Counters - ALWAYS RUNNING**

**Location:** `/workspace/src/core/board.cpp`

These counters are **NOT** protected by `#ifdef DEBUG` and run in ALL builds:

```cpp
// Line 1120-1124 in makeMoveInternal() - CALLED MILLIONS OF TIMES
if (m_inSearch) {
    g_searchMoves++;      // ❌ RUNS IN RELEASE
} else {
    g_gameMoves++;        // ❌ RUNS IN RELEASE  
}

// Line 1547 in unmakeMoveInternal()
g_historyPops++;          // ❌ RUNS IN RELEASE

// Line 1956 in pushGameHistory()
g_historyPushes++;        // ❌ RUNS IN RELEASE
```

**Impact:** These increment operations happen on EVERY move during search:
- ~1-10 million times per search
- Memory writes that could inhibit CPU optimizations
- Potential cache line pollution

### 2. **Counter Printing - ALWAYS RUNS**

**Location:** `/workspace/src/search/negamax.cpp:331`

```cpp
// At end of search() function - NO DEBUG GUARD
Board::printCounters();   // ❌ ALWAYS prints debug info
```

**Impact:** Prints 7 lines of debug output after EVERY search command

### 3. **Mode Tracking Counters**

**Location:** `/workspace/src/core/board.h:263-264`

```cpp
void setSearchMode(bool inSearch) {
    if (inSearch) g_searchModeSets++;     // ❌ RUNS IN RELEASE
    else g_searchModeClears++;            // ❌ RUNS IN RELEASE
    // ...
}
```

## 🟡 Non-Critical Debug Code (Protected)

### Properly Guarded Debug Code

These are correctly protected by `#ifdef DEBUG`:

1. **Board validation** (`board.cpp:179-212`)
2. **Zobrist consistency checks** (`board.cpp:1534-1542`)
3. **Various assertions throughout**

## 📊 Performance Impact Analysis

### Estimated Cost

For a typical 10-second search at depth 10-12:
- **~5-10 million moves** made/unmade
- **~10-20 million counter increments** 
- **~7 lines of output** per search

### CPU Impact
- Counter increments prevent compiler optimizations
- Memory writes can cause pipeline stalls
- False sharing if counters on same cache line

### Actual Impact
- Could be 1-5% performance loss
- More significant on CPU-bound positions
- Compounds with other inefficiencies

## ✅ RECOMMENDED FIXES

### Option 1: Complete Removal (BEST)

Remove all instrumentation from production code:

```cpp
// board.cpp - Remove lines 1120-1124
void Board::makeMoveInternal(Move move, UndoInfo& undo) {
    // DELETE THESE LINES:
    // if (m_inSearch) {
    //     g_searchMoves++;
    // } else {
    //     g_gameMoves++;
    // }
    
    // Rest of function...
}

// negamax.cpp - Remove line 331
// DELETE: Board::printCounters();

// board.cpp - Remove line 1547
// DELETE: g_historyPops++;

// board.cpp - Remove line 1956  
// DELETE: g_historyPushes++;

// board.h - Remove lines 263-264
// DELETE counter increments in setSearchMode()
```

### Option 2: Debug-Only Build (GOOD)

Wrap ALL instrumentation in DEBUG guards:

```cpp
// board.cpp
void Board::makeMoveInternal(Move move, UndoInfo& undo) {
#ifdef DEBUG_INSTRUMENTATION
    if (m_inSearch) {
        g_searchMoves++;
    } else {
        g_gameMoves++;
    }
#endif
    // Rest of function...
}

// negamax.cpp
#ifdef DEBUG_INSTRUMENTATION
    Board::printCounters();
#endif
```

### Option 3: Runtime Flag (ACCEPTABLE)

Add a runtime debug flag:

```cpp
// Only if explicitly enabled
if (g_enableInstrumentation) {
    g_searchMoves++;
}
```

## 🎯 Priority Actions

1. **IMMEDIATE:** Remove `Board::printCounters()` from negamax.cpp:331
2. **HIGH:** Remove/guard counter increments in makeMoveInternal()
3. **HIGH:** Remove/guard counter increments in unmakeMoveInternal()
4. **MEDIUM:** Remove/guard counters in setSearchMode()
5. **LOW:** Clean up counter declarations if removing

## 📈 Expected Performance Gain

After removing debug code:
- **1-3% faster NPS** in typical positions
- **Up to 5% faster** in simple endgames (higher move rates)
- **Cleaner output** (no debug spam)
- **Better compiler optimizations** possible

## 🔧 Implementation Commands

To remove all debug instrumentation:

```bash
# Create backup
cp src/core/board.cpp src/core/board.cpp.debug_backup
cp src/core/board.h src/core/board.h.debug_backup
cp src/search/negamax.cpp src/search/negamax.cpp.debug_backup

# Then edit files to remove debug code
```

## 📝 Summary

We have **active debug instrumentation in the hot path** that runs in ALL builds, not just debug builds. This code:

1. Executes millions of times per search
2. Is not protected by DEBUG flags
3. Prints debug output after every search
4. Could be costing 1-5% performance

**Recommendation:** Remove all instrumentation code completely, or at minimum wrap it in `#ifdef DEBUG_INSTRUMENTATION` guards that are OFF by default.