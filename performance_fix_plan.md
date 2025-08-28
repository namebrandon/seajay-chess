# SeaJay Performance Regression Fix Plan

## Root Cause Analysis
The ~80 ELO regression has multiple contributing factors:

### Primary Issues (in order of impact):
1. **Dynamic cast in hot path** (~30 ELO) - Executed every 4096 nodes
2. **DEBUG instrumentation overhead** (~20 ELO) - Timing and output code
3. **Time control/UCI info coupling** (~20 ELO) - Check frequency mismatch
4. **Cache thrashing** (~10 ELO) - Large SearchData structures

## Immediate Fix Steps

### Step 1: Remove DEBUG Code (Quick Win)
Remove lines 157-201 in negamax.cpp (all DEBUG_UCI_REGRESSION code)

### Step 2: Eliminate Dynamic Cast
Replace dynamic_cast with virtual method or type flag:

```cpp
// In types.h - SearchData class
virtual IterativeSearchData* asIterative() { return nullptr; }

// In iterative_search_data.h
IterativeSearchData* asIterative() override { return this; }

// In negamax.cpp line 171
auto* iterativeInfo = info.asIterative();  // No dynamic_cast!
```

### Step 3: Decouple Time Control from UCI Info
Keep separate check intervals:
- Time control: Every 2048 nodes (0x7FF)
- UCI info: Every 16384 nodes (0x3FFF) or time-based only

### Step 4: Optimize shouldSendInfo()
The current implementation is too complex. Simplify to:
```cpp
bool shouldSendInfo() const {
    auto elapsed = std::chrono::steady_clock::now() - m_lastInfoTime;
    return elapsed >= std::chrono::milliseconds(100);  // Fixed 100ms interval
}
```

## Testing Protocol
1. Remove DEBUG code first, test ELO
2. Fix dynamic_cast, test ELO  
3. Adjust check frequencies, test ELO
4. Verify total recovery of 80+ ELO

## Expected Results
- Step 1: +15-20 ELO
- Step 2: +25-30 ELO
- Step 3: +15-20 ELO
- Step 4: +5-10 ELO
Total: +60-80 ELO recovery