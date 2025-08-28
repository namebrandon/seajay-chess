# SearchData Refactoring Risk Assessment

## Scope of Changes

### Current Usage Analysis

#### 1. **Killers (3 usage points)**
```cpp
info.killers.isKiller(ply, move)     // Line 630, 880
info.killers.update(ply, move)       // Line 901
```

#### 2. **History (3 usage points)**
```cpp
info.history.getScore(side, from, to)         // Line 672
info.history.update(side, from, to, depth)    // Line 905
info.history.updateFailed(side, from, to, depth) // Line 910
```

#### 3. **CounterMoves (5 usage points)**
```cpp
info.counterMoves.getCounterMove(prevMove)    // Line 545, 634, 883
info.counterMoves.update(prevMove, move)      // Line 919
info.counterMoves.clear()                     // Line 1015
```

**Total: 11 direct access points across 1 file**

## Invasiveness Level: MODERATE

### Why It's Moderate (Not High)
1. **Limited scope**: Only 11 access points in negamax.cpp
2. **Single file**: All changes confined to search code
3. **Simple pattern**: All accesses use dot notation (`.`) which becomes arrow (`->`)
4. **No external API changes**: Other parts of engine unaffected

### Why It's Not Low
1. **Memory management**: Need to add allocation/deallocation
2. **Ownership semantics**: Must ensure proper lifetime management
3. **Testing critical**: Errors could cause crashes or memory leaks

## Implementation Approach

### Option A: Minimal Change (RECOMMENDED)
```cpp
// In searchIterativeTest() and search():
IterativeSearchData info;
KillerMoves killers;
HistoryHeuristic history;
CounterMoves counterMoves;

// Initialize pointers
info.killers = &killers;
info.history = &history;
info.counterMoves = &counterMoves;

// Rest of code changes . to ->
```

**Pros:**
- Stack allocation (no new/delete)
- Automatic cleanup
- Minimal code change
- No memory leak risk

**Cons:**
- Objects still exist, just not embedded
- Stack usage (but much better than 42KB in SearchData)

### Option B: Heap Allocation
```cpp
// In searchIterativeTest():
IterativeSearchData info;
info.killers = new KillerMoves();
info.history = new HistoryHeuristic();
info.counterMoves = new CounterMoves();

// At function end:
delete info.killers;
delete info.history;
delete info.counterMoves;
```

**Pros:**
- True heap separation
- Could use smart pointers for safety

**Cons:**
- Need careful cleanup
- Risk of memory leaks
- Slightly more complex

### Option C: Static/Thread-Local (RISKY)
```cpp
thread_local KillerMoves s_killers;
thread_local HistoryHeuristic s_history;
thread_local CounterMoves s_counterMoves;

// In SearchData:
KillerMoves* killers = &s_killers;
```

**Pros:**
- No allocation overhead
- Automatically thread-safe

**Cons:**
- Hidden global state
- Harder to test
- Could affect reentrancy

## Risks and Mitigations

### Risk 1: Null Pointer Dereference
**Severity:** HIGH  
**Likelihood:** LOW  
**Mitigation:** 
- Initialize pointers immediately after creating SearchData
- Add debug assertions: `assert(info.killers != nullptr)`
- Use Option A (stack allocation) to guarantee validity

### Risk 2: Memory Leaks
**Severity:** MEDIUM  
**Likelihood:** LOW (with Option A)  
**Mitigation:**
- Use stack allocation (Option A)
- Or use RAII/smart pointers
- Add leak detection in debug builds

### Risk 3: Performance Regression from Indirection
**Severity:** LOW  
**Likelihood:** LOW  
**Mitigation:**
- Pointer dereference overhead is negligible compared to cache miss savings
- Modern CPUs handle pointer indirection efficiently
- Net gain from better cache usage far outweighs indirection cost

### Risk 4: Search Behavior Changes
**Severity:** HIGH  
**Likelihood:** VERY LOW  
**Mitigation:**
- No algorithmic changes, only memory layout
- Extensive testing with bench command
- Verify identical move selection in test positions

### Risk 5: Compilation Issues
**Severity:** LOW  
**Likelihood:** MEDIUM  
**Mitigation:**
- Simple find/replace of `.` with `->`
- Compiler will catch all type errors
- Can be done incrementally

## Implementation Steps

### Phase 1: Prepare (5 minutes)
1. Back up current code
2. Create new branch
3. Add size measurement debug code

### Phase 2: Refactor SearchData (10 minutes)
```cpp
// In types.h:
struct SearchData {
    // Change from:
    KillerMoves killers;
    HistoryHeuristic history;
    CounterMoves counterMoves;
    
    // To:
    KillerMoves* killers = nullptr;
    HistoryHeuristic* history = nullptr;
    CounterMoves* counterMoves = nullptr;
};
```

### Phase 3: Update Initialization (10 minutes)
```cpp
// In searchIterativeTest():
IterativeSearchData info;
KillerMoves killers;
HistoryHeuristic history;
CounterMoves counterMoves;

info.killers = &killers;
info.history = &history;
info.counterMoves = &counterMoves;
```

### Phase 4: Update Access Points (15 minutes)
```bash
# Simple sed replacement:
sed -i 's/info\.killers\./info.killers->/g' src/search/negamax.cpp
sed -i 's/info\.history\./info.history->/g' src/search/negamax.cpp
sed -i 's/info\.counterMoves\./info.counterMoves->/g' src/search/negamax.cpp
```

### Phase 5: Test and Verify (30 minutes)
1. Compile and fix any errors
2. Run bench - verify it completes
3. Check SearchData size is ~1KB
4. Run OpenBench test

## Expected Outcome

### Before Refactoring:
- SearchData size: 42,176 bytes
- Cache misses: High
- Performance: -30-40 ELO

### After Refactoring:
- SearchData size: ~1,000 bytes
- Cache misses: Normal
- Performance: Baseline restored

## Recommendation

**GO AHEAD WITH OPTION A** (Stack Allocation)

### Reasons:
1. **Low risk**: Stack allocation guarantees no memory issues
2. **Simple implementation**: ~30-40 minutes total work
3. **High reward**: Should recover full 30-40 ELO
4. **Easy rollback**: Can revert if issues arise
5. **Clear testing**: Bench and size checks verify success

### Success Criteria:
1. ✅ SearchData size < 2KB (down from 42KB)
2. ✅ Bench completes without crashes
3. ✅ Bench count remains 19191913 (no logic change)
4. ✅ OpenBench shows +30-40 ELO improvement
5. ✅ No memory leaks in valgrind

## Alternative: Less Invasive Fix

If refactoring is deemed too risky, implement the three quick fixes:
1. Replace dynamic_cast with type flag (2% improvement)
2. Reduce info frequency (0.5% improvement)
3. Optimize InfoBuilder (0.5% improvement)

This would recover ~8-12 ELO with virtually no risk, but leaves 20-30 ELO on the table.

## Conclusion

The refactoring is **MODERATE** invasiveness with **LOW** risk when using Option A (stack allocation). The change is mechanical (replacing `.` with `->`) and confined to a single file. Given the significant performance gain (30-40 ELO), this refactoring is strongly recommended.