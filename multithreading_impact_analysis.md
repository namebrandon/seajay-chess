# Multi-Threading Impact Analysis for SearchData Refactoring

## Critical Context: Multi-threaded Search Requirements

In multi-threaded chess engines (like Stockfish's Lazy SMP), each thread typically:
1. Searches different parts of the tree simultaneously
2. Needs its own search state (killers, history, counter-moves)
3. May share some data (transposition table) but NOT move ordering tables

## Impact Analysis by Approach

### Option A: Stack Allocation (Current Recommendation)
```cpp
// In each search thread:
IterativeSearchData info;
KillerMoves killers;          // Thread-local stack
HistoryHeuristic history;     // Thread-local stack
CounterMoves counterMoves;    // Thread-local stack

info.killers = &killers;
info.history = &history;
info.counterMoves = &counterMoves;
```

**Multi-threading Impact: ✅ EXCELLENT**
- Each thread naturally gets its own instances on its stack
- No sharing, no synchronization needed
- Perfectly compatible with Lazy SMP
- This is essentially what Stockfish does

### Option B: Heap Allocation
```cpp
// In each search thread:
info.killers = new KillerMoves();
info.history = new HistoryHeuristic();
info.counterMoves = new CounterMoves();
```

**Multi-threading Impact: ✅ GOOD**
- Each thread allocates its own instances
- Clean separation between threads
- Slightly more overhead than stack allocation
- Need to ensure cleanup in thread termination

### Option C: Static/Thread-Local (Previously Mentioned)
```cpp
thread_local KillerMoves s_killers;
thread_local HistoryHeuristic s_history;
```

**Multi-threading Impact: ⚠️ COMPLICATED**
- Works but creates hidden state
- Each thread gets its own copy (good)
- But harder to manage thread pools
- Can cause issues with thread reuse

### Option D: Global Shared Arrays (DON'T DO THIS)
```cpp
static KillerMoves g_killers;  // Shared by all threads
```

**Multi-threading Impact: ❌ TERRIBLE**
- Would require synchronization (mutexes/atomics)
- Massive contention between threads
- Would destroy multi-threaded performance

## Current Embedded Arrays Problem for Multi-threading

The current approach (42KB embedded in SearchData) is actually **BAD for multi-threading**:

```cpp
struct SearchData {
    KillerMoves killers;       // 1KB embedded
    HistoryHeuristic history;  // 32KB embedded
    CounterMoves counterMoves; // 16KB embedded
};
```

**Why this is problematic:**
1. **Memory bandwidth**: Each thread needs 42KB × search depth
2. **Cache coherence**: Large objects increase false sharing risk
3. **NUMA effects**: Large allocations hurt on multi-socket systems
4. **Thread scaling**: Memory bandwidth becomes bottleneck

## Recommended Multi-threaded Architecture

### Phase 1: Current Fix (Option A with pointers)
```cpp
struct SearchData {
    KillerMoves* killers;
    HistoryHeuristic* history;
    CounterMoves* counterMoves;
};
```

### Phase 2: Future Multi-threaded Enhancement
```cpp
class SearchThread {
private:
    // Per-thread move ordering tables
    KillerMoves m_killers;
    HistoryHeuristic m_history;
    CounterMoves m_counterMoves;
    
public:
    void search() {
        IterativeSearchData info;
        info.killers = &m_killers;
        info.history = &m_history;
        info.counterMoves = &m_counterMoves;
        // ... rest of search
    }
};
```

**This architecture:**
- ✅ Each thread owns its move ordering data
- ✅ No false sharing between threads
- ✅ Optimal cache usage per thread
- ✅ Scales linearly with thread count

## Memory Considerations for Multi-threading

### Current (Embedded Arrays):
- Per thread: 42KB × max_depth (e.g., 42KB × 128 = 5.4MB)
- 16 threads: 86MB just for SearchData!
- Poor cache utilization

### After Refactoring (Pointers):
- Per thread: 1KB × max_depth + 49KB for tables = ~180KB
- 16 threads: ~3MB total
- Excellent cache utilization

## Advanced Considerations

### 1. History Heuristic Sharing
Some engines share history between threads for better move ordering:
```cpp
class SharedHistory {
    std::atomic<int> scores[2][64][64];  // Atomic updates
};
```

With pointers, this is easy to implement:
```cpp
// Shared between threads
SharedHistory* shared_history;

// Or thread-local
HistoryHeuristic* local_history;
```

### 2. Lazy SMP Compatibility
Lazy SMP (used by Stockfish) requires:
- Independent search threads
- Minimal communication
- No explicit work splitting

**Pointer approach is PERFECT for this:**
- Each thread is independent
- No synchronization needed
- Natural work distribution

### 3. NUMA Optimization
On NUMA systems, you want:
- Thread-local data close to the CPU
- Minimal cross-socket traffic

**Stack allocation (Option A) is IDEAL:**
- Data allocated on local NUMA node
- No remote memory access
- Optimal memory bandwidth usage

## Risk Assessment for Multi-threading

### If we DON'T refactor (keep embedded arrays):
- 🔴 **HIGH RISK**: Memory bandwidth bottleneck
- 🔴 **HIGH RISK**: Poor thread scaling
- 🔴 **HIGH RISK**: Cache coherence issues

### If we DO refactor (use pointers):
- 🟢 **NO RISK**: Natural thread separation
- 🟢 **BENEFIT**: Better thread scaling
- 🟢 **BENEFIT**: Easier to add sharing later if needed

## Conclusion

**The pointer refactoring IMPROVES multi-threading readiness!**

1. **Option A (stack allocation) is PERFECT for multi-threading**
   - Each thread gets its own tables
   - No synchronization needed
   - Matches Stockfish's approach

2. **Current embedded arrays are BAD for multi-threading**
   - Too much memory per thread
   - Cache coherence problems
   - Memory bandwidth bottleneck

3. **The refactoring is a necessary step toward multi-threading**
   - Reduces memory footprint per thread
   - Enables flexible ownership models
   - Allows future optimization (shared vs. private tables)

## Recommendation

**DEFINITELY proceed with Option A refactoring.** It not only fixes the current performance regression but also:
- ✅ Prepares the codebase for multi-threading
- ✅ Reduces memory footprint for parallel search
- ✅ Enables flexible threading models
- ✅ Improves cache efficiency for multi-core systems

The refactoring is not just compatible with multi-threading—it's a **prerequisite** for efficient parallel search.