# Golden Binary Mystery Analysis Report

## Executive Summary
The golden binary (seajay-stage14-sprt-candidate1, 411KB) from commit ce52720 outperforms all rebuilt versions despite enabling the same MVV-LVA feature. After extensive investigation, we've identified several potential causes but the exact reason remains elusive.

## Binary Comparison

| Binary | Size | MVV-LVA | Performance |
|--------|------|---------|-------------|
| Golden (C1) | 411KB | Yes (via header) | +300 ELO baseline |
| Original rebuild | 384KB | No | Much weaker |
| Candidate 6 | 402KB | Yes (via CMake) | Still weaker than golden |
| Current rebuild | 384KB | Yes (via header) | Unknown |

## Key Findings

### 1. MVV-LVA Configuration Difference
- **Golden binary**: MVV-LVA enabled through `#define ENABLE_MVV_LVA` in move_ordering.h
- **Candidate 6**: MVV-LVA enabled through CMake `add_compile_definitions(ENABLE_MVV_LVA)`
- **Issue**: Double definition might cause compilation differences

### 2. Removed Extra Files
The following files are compiled but never used:
- `quiescence_performance.cpp` (298 lines)
- `quiescence_optimized.cpp` (449 lines)

These add ~750 lines of unused code but shouldn't affect runtime performance.

### 3. Symbol Analysis
Golden binary unique symbols:
- `_ZN6seajay6searchL13VICTIM_VALUESE` - VICTIM_VALUES array in quiescence namespace
- Different quiescence function template instantiations

This suggests the golden binary has slightly different code generation for the quiescence search.

### 4. Minor Code Changes Since Golden
- Added `#include <chrono>` to quiescence.cpp
- Added comments about emergency cutoff removal
- Changed engine name from "Candidate-1" to "Candidate-6"
- No functional changes to the algorithm

## Potential Causes for Performance Difference

### Theory 1: Compiler Optimization Variance
The 411KB vs 384KB size difference (27KB) suggests different optimization or inlining decisions. The golden binary might have:
- Different inlining thresholds
- Loop unrolling differences
- Better cache alignment by chance

### Theory 2: Build Environment Differences
- Golden was built on commit ce52720 with a clean environment
- Subsequent builds might have stale object files or different CMake cache
- Incremental vs clean builds can affect optimization

### Theory 3: Undefined Behavior or Race Condition
- The golden binary might have benefited from fortunate undefined behavior
- Memory layout differences could affect cache performance
- Uninitialized variables that happen to have good values

### Theory 4: Hidden Conditional Compilation
There might be other `#ifdef` blocks we haven't found that were active during the golden build but not now.

## Recommended Actions

### Immediate Steps
1. **Clean rebuild at exact commit**:
   ```bash
   git checkout ce52720
   rm -rf build/
   mkdir build && cd build
   cmake -DCMAKE_BUILD_TYPE=Release -DQSEARCH_MODE=PRODUCTION ..
   make -j
   ```

2. **Binary comparison**:
   ```bash
   objdump -d golden > golden.asm
   objdump -d rebuild > rebuild.asm
   diff golden.asm rebuild.asm
   ```

3. **Performance profiling**:
   - Run both binaries through `perf` to identify hot spots
   - Compare cache miss rates and branch prediction

### Investigation Areas
1. Check if there are environment variables affecting compilation
2. Verify exact GCC version and flags used for golden
3. Look for timing-dependent code that might behave differently
4. Check if TT (transposition table) sizing affects binary size

## Code Quality Issues Found

### Issue 1: Double Definition of ENABLE_MVV_LVA
- Defined in both move_ordering.h and CMakeLists.txt
- Should only be in one place

### Issue 2: Unused Code Files
- quiescence_performance.cpp and quiescence_optimized.cpp are compiled but never called
- Increases binary size unnecessarily

### Issue 3: Inconsistent Node Limit Comments
Comments mention different node limits but the actual compile-time values are:
- TESTING: 10K via `#ifdef QSEARCH_TESTING`
- TUNING: 100K via `#ifdef QSEARCH_TUNING`
- PRODUCTION: No limit (default)

## Conclusion

The golden binary mystery appears to be a case of "accidental optimization" where a specific combination of:
- Build environment state
- Compiler optimization decisions
- Code/data layout
- Possible undefined behavior

Created a binary that performs exceptionally well. The fact that we can't reproduce it exactly suggests the improvement might be fragile and environment-dependent rather than algorithmic.

## Next Steps

1. Test the current rebuild (384KB with header-based MVV-LVA) in SPRT
2. If still weaker, try building with different optimization flags (-O2 vs -O3, -march=native)
3. Consider that the golden binary might be an outlier and focus on algorithmic improvements
4. Document this as a lesson in reproducible builds and the importance of build system hygiene

---
*Analysis conducted: August 15, 2025*
*Golden binary: seajay-stage14-sprt-candidate1 (411KB)*
*Commit: ce52720*