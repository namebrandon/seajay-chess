# Stage 14 Remediation Audit

## Original Requirements

From `/workspace/project_docs/planning/stage14_quiescence_search_implementation_plan.md`:
- Implement capture-only quiescence search at leaf nodes
- Handle check evasions in quiescence for tactical completeness  
- Maintain search performance while adding 70-90% more nodes
- Integrate seamlessly with existing search infrastructure
- Provide UCI controls for testing and debugging

## Known Issues Going In

1. **Build modes (TESTING/TUNING/PRODUCTION)** - Using compile-time constants for node limits
2. **Multiple build scripts** - build_testing.sh, build_tuning.sh, build_production.sh creating different binaries
3. **ENABLE_QUIESCENCE flag** - History of missing compiler flag causing quiescence to not be compiled in

## Additional Issues Found

### Critical Issues (Must Fix)

1. **Compile-Time Node Limits** (lines 27-40 in quiescence.h)
   ```cpp
   #ifdef QSEARCH_TESTING
       static constexpr uint64_t NODE_LIMIT_PER_POSITION = 10000;
   #elif defined(QSEARCH_TUNING)
       static constexpr uint64_t NODE_LIMIT_PER_POSITION = 100000;
   #else
       static constexpr uint64_t NODE_LIMIT_PER_POSITION = UINT64_MAX;
   #endif
   ```
   - Violates "NO COMPILE-TIME FEATURE FLAGS" principle
   - Requires separate binaries for different test modes
   - CMakeLists.txt maintains QSEARCH_MODE configuration

2. **Missing Best Move Tracking**
   ```cpp
   // Line 424 in quiescence.cpp
   Move bestMove = NO_MOVE;  // No best move tracked in current quiescence
   ```
   - TT entries store NO_MOVE even when good captures found
   - Loses valuable move ordering information
   - Reduces TT effectiveness significantly

3. **Incorrect TT Bound Classification** (lines 426-442)
   - Incorrectly marks bounds as EXACT when they should be LOWER
   - Can cause search instabilities
   - Logic for determining bound type is flawed

4. **Global SEE Pruning State** (lines 17-18 in quiescence.cpp)
   ```cpp
   SEEPruningMode g_seePruningMode = SEEPruningMode::OFF;
   SEEPruningStats g_seePruningStats;
   ```
   - Not thread-safe for parallel search
   - Global state makes testing difficult

### Algorithm Issues

5. **Delta Pruning Margins Too Conservative**
   - Current: 900cp (normal), 600cp (endgame), 400cp (panic)
   - Standard: 200cp + piece value
   - May be missing tactical opportunities

6. **Debug Logging in Hot Path** (line 331)
   - SEE pruning logs every 1000 prunes
   - Performance impact in critical code

7. **Inefficient Move Ordering**
   - Multiple sort/rotate operations (lines 226-262)
   - Could be optimized to single-pass scoring

### Missing Features

8. **No Check Giving Moves**
   - Only handles check evasions, doesn't generate checks
   - Missing tactical opportunities

9. **No Futility Pruning at Frontier Nodes**
   - Could skip hopeless positions earlier

10. **Unused Optimized Implementation**
    - quiescence_optimized.cpp exists but not integrated
    - Could improve performance

## Implementation Deviations

- **Build Mode System**: Original plan called for progressive limiter removal via compile flags for testing phases
- **SEE Integration**: Stage 15 SEE is already partially integrated (should be separate stage)
- **Global Variables**: SEE pruning uses global state instead of proper encapsulation

## Testing Gaps

- No tests for best move tracking
- No verification of TT bound types
- No tests for node limit enforcement at runtime
- No performance benchmarks for different delta margins

## Utilities Related to This Stage

- `build_testing.sh` - Builds with 10K node limit
- `build_tuning.sh` - Builds with 100K node limit
- `build_production.sh` - Builds with unlimited nodes
- Various test utilities in `/workspace/tests/search/` for quiescence testing

## Comparison with Reference Engines

### Stockfish
- **Better**: Tracks best move, generates checks, better pruning margins
- **Similar**: Stand-pat, check evasions, TT integration
- **SeaJay Advantage**: Simpler, easier to understand

### Ethereal
- **Better**: More sophisticated pruning, better move ordering
- **Similar**: Delta pruning approach, check handling
- **SeaJay Advantage**: Cleaner separation of concerns

### Berserk
- **Better**: Highly optimized, minimal overhead
- **Similar**: Core algorithm structure
- **SeaJay Advantage**: More conservative/safe approach

## Estimated Impact of Fixes

- **Runtime node limits**: +0 ELO (functionality fix)
- **Best move tracking**: +20-30 ELO
- **Delta margin adjustment**: +10-15 ELO
- **TT bound fixes**: +5-10 ELO
- **Check giving moves**: +15-20 ELO (deferred to Stage 16)
- **Total potential**: +35-55 ELO (excluding deferred items)

## Remediation Priority

### Phase 1: Critical Fixes (Must Do)
1. Convert compile-time node limits to UCI runtime option
2. Remove QSEARCH_MODE from CMakeLists.txt
3. Consolidate build scripts to single build.sh
4. Fix TT bound classification logic

### Phase 2: Algorithm Improvements (Will Do)
1. Implement best move tracking in quiescence
2. Adjust delta pruning margins to standard values
3. Move SEE pruning mode from global to SearchData
4. Remove debug logging from hot path

### Phase 3: Performance Optimizations (Will Do)
1. Optimize move ordering to single-pass
2. Integrate optimized implementation
3. Cache static evaluation results

### Phase 4: Future Enhancements (Deferred)
1. Add check giving moves (requires SEE, deferred to Stage 16)
2. Implement futility pruning at frontier nodes
3. Add recapture extensions

## Files Requiring Changes

1. `/workspace/src/search/quiescence.h` - Remove compile-time constants, add runtime config
2. `/workspace/src/search/quiescence.cpp` - Fix TT bounds, add best move tracking
3. `/workspace/CMakeLists.txt` - Remove QSEARCH_MODE and related logic
4. `/workspace/build*.sh` - Remove extra scripts, update main build.sh
5. `/workspace/src/uci/uci.cpp` - Add QSearchNodeLimit UCI option
6. `/workspace/src/search/types.h` - Add node limit to SearchData structure
7. `/workspace/Makefile` - Remove QSEARCH_MODE references

## Success Criteria

- [ ] All compile-time node limits removed
- [ ] Single build script produces one binary
- [ ] UCI option controls node limits at runtime
- [ ] TT bound types correctly classified
- [ ] Best move tracked and stored in TT
- [ ] All existing tests continue to pass
- [ ] No performance regression in SPRT testing
- [ ] Documentation updated with new UCI options

## Notes from Stage 14 Development

From the development history, Stage 14 went through multiple iterations:
- **C1 (Golden)**: Initial working version with +300 ELO
- **C2-C4**: Time control issues
- **C5-C8**: Discovery of missing ENABLE_QUIESCENCE flag
- **C9**: Delta margin catastrophe (200cp was too aggressive)
- **C10**: Recovery and stabilization

The current implementation is stable and provides significant ELO gain, but the compile-time flags and missing optimizations limit its potential.

---

**Document Version**: 1.0  
**Date**: 2025-08-17  
**Stage**: 14 - Quiescence Search  
**Status**: Audit Complete - Ready for Remediation Implementation