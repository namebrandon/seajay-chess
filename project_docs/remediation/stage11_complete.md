# Stage 11 Remediation Complete - MVV-LVA Move Ordering

## Summary

Successfully remediated Stage 11 MVV-LVA implementation, fixing critical scoring bugs and removing compile-time feature flags. MVV-LVA is now always enabled at runtime as recommended by chess-engine-expert.

## Issues Fixed

### 1. Critical Scoring System Bug (✅ FIXED)
- **Problem**: Dual scoring system with mismatched values
  - Header defined formula returning values like PxQ=899
  - Implementation used different table returning PxQ=505
  - Tests expected 899, code returned 505
- **Solution**: Removed precomputed table, use simple formula:
  ```cpp
  score = VICTIM_VALUES[victim] - ATTACKER_VALUES[attacker]
  ```
- **Result**: Consistent scoring, tests pass

### 2. Compile-Time Feature Flag (✅ REMOVED)
- **Problem**: `ENABLE_MVV_LVA` violated no-compile-flags principle
- **Solution**: 
  - Removed all #ifdef blocks from negamax.cpp and quiescence_optimized.cpp
  - Removed flag from CMakeLists.txt
  - MVV-LVA now always active
- **Result**: No compile-time configuration needed

### 3. Stage Mixing with SEE (✅ SEPARATED)
- **Problem**: Stage 11 code was calling Stage 15 SEE functions
- **Solution**: 
  - Changed from `g_seeMoveOrdering` to direct `MvvLvaOrdering` instance
  - Pure MVV-LVA implementation for Stage 11
- **Result**: Clean stage separation

### 4. Sort Stability (✅ FIXED)
- **Problem**: Used `std::sort` causing non-deterministic ordering
- **Solution**: Changed to `std::stable_sort`
- **Result**: Deterministic move ordering

### 5. Code Duplication (✅ REMOVED)
- **Problem**: Inline scoring duplicated scoreMove() logic
- **Solution**: Sort lambda now calls scoreMove() directly
- **Result**: Single source of truth for scoring

## Test Results

### MVV-LVA Unit Tests
```
✓ Phase 1: Infrastructure and type safety
✓ Phase 2: Basic capture scoring (PxQ=899, QxP=91, etc.)
✓ Phase 3: En passant handling (PxP=99)
✓ Phase 4: Promotion handling (Q=102000, etc.)
✓ Phase 5: Stable tiebreaking
✓ Phase 6: Search integration
```

### Performance
- Benchmark: 19,191,913 nodes at 7,315,557 nps
- Build successful with no compile errors
- All tests passing

## Implementation Details

### Scoring Values (Confirmed Correct)
- **Victims**: P=100, N/B=325, R=500, Q=900, K=10000
- **Attackers**: P=1, N/B=3, R=5, Q=9, K=100
- **Examples**: PxQ=899, QxP=91, PxP=99

### Key Design Decisions
1. **No UCI Option**: MVV-LVA always active per expert recommendation
2. **Simple Formula**: Better than table (cache-friendly, maintainable)
3. **Stable Sort**: Preserves original move order for equal scores
4. **Clean Separation**: Stage 11 independent of Stage 15 SEE

## Files Modified

1. `/workspace/src/search/move_ordering.cpp`
   - Replaced table with formula
   - Fixed scoreMove() to use formula
   - Changed to stable_sort
   - Removed code duplication

2. `/workspace/src/search/negamax.cpp`
   - Removed ENABLE_MVV_LVA guards
   - Direct MvvLvaOrdering usage
   - Removed fallback code

3. `/workspace/src/search/quiescence_optimized.cpp`
   - Removed ENABLE_MVV_LVA guards
   - Always use MVV-LVA scoring

4. `/workspace/CMakeLists.txt`
   - Removed `add_compile_definitions(ENABLE_MVV_LVA)`
   - Updated status message

5. `/workspace/tests/test_mvv_lva.cpp`
   - Fixed magic namespace reference

## Validation Checklist

- ✅ All compile flags removed for Stage 11
- ✅ MVV-LVA always active (no UCI option per expert)
- ✅ All tests pass with correct values
- ✅ Benchmark works: 19,191,913 nodes
- ✅ No compilation errors or warnings
- ✅ Documentation updated
- ✅ Code follows project style
- ✅ Expert review completed

## Expert Analysis Highlights

Per chess-engine-expert:
- MVV-LVA should always be enabled (fundamental to alpha-beta)
- Simple formula better than table (cache-friendly)
- Value scales are correct and well-proportioned
- stable_sort sufficient for tiebreaking
- Clean separation from SEE is correct approach

## Lessons Learned

1. **Test-Code Mismatch**: Tests can expect different values than implementation provides
2. **Stage Separation**: Keep stages independent for clean architecture
3. **Simplicity Wins**: Formula beats table for maintainability
4. **Always On**: Some features (like MVV-LVA) should never be optional

## Next Steps

- Ready to merge to main after SPRT testing
- Stage 12 (Transposition Tables) can begin after merge
- Consider updating test warning about "feature flag DISABLED"

## Time Spent

- Audit: 1 hour
- Implementation: 1.5 hours
- Testing: 0.5 hours
- **Total**: ~3 hours (faster than estimated 4-6 hours)

## Commit Message

```
Stage 11 Remediation: Fix MVV-LVA scoring and remove compile flags

- Fixed critical scoring bug (dual system with wrong values)
- Removed ENABLE_MVV_LVA compile flag (always active now)
- Separated MVV-LVA from SEE (Stage 15)
- Fixed sort stability with stable_sort
- Removed code duplication in sort lambda

bench: 19191913

All tests pass. MVV-LVA now uses correct formula (PxQ=899, QxP=91).
No compile-time flags per project guidelines.

🤖 Generated with Claude Code

Co-Authored-By: Claude <noreply@anthropic.com>
```