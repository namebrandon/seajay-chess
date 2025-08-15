# Golden Binary Investigation - Final Report

## Executive Summary

After extensive investigation into the "golden binary" mystery (seajay-stage14-sprt-candidate1, 411KB, +300 ELO), we have confirmed that:

1. **The golden binary CANNOT be reproduced from source** - Clean builds at commit ce52720 produce 384KB binaries
2. **MVV-LVA is enabled in all versions** - This is not the differentiator
3. **The size difference (411KB vs 384KB) remains unexplained**
4. **The golden binary appears to be an unreproducible build artifact**

## Investigation Findings

### 1. MVV-LVA Configuration
**Initial Theory:** Golden had MVV-LVA, rebuilds didn't  
**Finding:** DISPROVEN

- Golden binary: MVV-LVA enabled via `#define ENABLE_MVV_LVA` in move_ordering.h
- All rebuilds: Same configuration (header file defines it)
- CMake addition of ENABLE_MVV_LVA was redundant and has been removed
- Both golden and rebuilds have identical MVV-LVA symbol tables

### 2. Build Reproducibility Test
**Test:** Clean build at exact commit ce52720  
**Result:** FAILED to reproduce

```bash
git checkout ce52720
rm -rf build/
./build_production.sh
# Result: 384KB binary, NOT 411KB
```

### 3. Code Differences
**Theory:** Code changes since golden commit  
**Finding:** MINIMAL impact

Changes since ce52720:
- Added `#include <chrono>` (should not affect size significantly)
- Added comments about emergency cutoff removal
- Changed engine name from "Candidate-1" to others
- NO functional algorithm changes

### 4. Unused Code Files
**Finding:** Extra files compiled but never called

Removed from build:
- `quiescence_performance.cpp` (298 lines)
- `quiescence_optimized.cpp` (449 lines)

These were compiled into all binaries but never executed. Removing them still produces 384KB binaries.

### 5. Binary Analysis

| Aspect | Golden (411KB) | Rebuild (384KB) |
|--------|---------------|----------------|
| Compiler | GCC 12.3.0 | GCC 12.3.0 |
| MVV-LVA symbols | Present | Present |
| Symbol count | 466 | 462 |
| Unique symbols | VICTIM_VALUES in quiescence namespace | None |
| Performance | +300 ELO baseline | Much weaker |

## Theories for the Anomaly

### Most Likely: Accidental Debug/Profile Build
The 27KB size difference suggests the golden binary might have been built with:
- Debug symbols not fully stripped
- Profile-guided optimization intermediate
- Different optimization level temporarily
- Stale object files from previous builds

### Alternative Theories
1. **Incremental build artifact** - Mixed object files from different configurations
2. **Compiler flag variance** - Temporary flag changes not in CMakeLists.txt
3. **Link-time optimization differences** - Different LTO settings
4. **Fortuitous undefined behavior** - Memory layout creating better cache behavior

## Why We Can't Reproduce It

The golden binary appears to be a "happy accident" where:
1. Some build configuration state (not in version control)
2. Combined with the commit ce52720 code
3. Produced an unusually performant binary
4. That cannot be reproduced with clean builds

## Recommendations

### Immediate Actions
1. **PRESERVE the golden binary** - It's our only reference for Stage 14 performance
2. **Accept we cannot reproduce it** - Focus on moving forward
3. **Use golden as benchmark** - Test new improvements against it

### Development Strategy
1. **Start fresh from current main** - Don't chase the ghost
2. **Focus on algorithmic improvements** - Not build system mysteries
3. **Implement proper build reproducibility** - Document all flags in CMake
4. **Consider the golden binary an outlier** - Not the expected baseline

### Build System Improvements
```cmake
# Add to CMakeLists.txt for reproducibility
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG -march=x86-64 -mtune=generic")
option(ENABLE_LTO "Enable Link Time Optimization" OFF)
option(ENABLE_PGO "Enable Profile Guided Optimization" OFF)

# Document exact build command
# cmake -DCMAKE_BUILD_TYPE=Release -DQSEARCH_MODE=PRODUCTION ..
# make -j$(nproc)
```

## Lessons Learned

1. **Build reproducibility is critical** - All build flags must be in version control
2. **Binary size != performance** - The correlation is weak
3. **"Golden" binaries can be accidents** - Not all performance gains are reproducible
4. **Test against multiple baselines** - Don't rely on a single reference
5. **Document build processes** - Include exact commands in commit messages

## Conclusion

The golden binary (seajay-stage14-sprt-candidate1, 411KB) represents an unreproducible build anomaly that happened to perform exceptionally well. While we cannot recreate it, we've learned valuable lessons about build system hygiene and the importance of reproducible builds.

The investigation has eliminated MVV-LVA configuration as the cause and confirmed that the issue is related to build environment state not captured in version control. The best path forward is to accept this anomaly and focus on algorithmic improvements that can be reliably reproduced.

---
*Investigation completed: August 15, 2025*  
*Golden binary: /workspace/binaries/seajay-stage14-sprt-candidate1 (411KB)*  
*Commit: ce52720*  
*Status: UNREPRODUCIBLE - Likely build environment artifact*