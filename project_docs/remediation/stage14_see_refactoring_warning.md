# ⚠️ WARNING: Complex SEE Pruning Refactoring

## Date: 2025-08-17
## Stage: 14 Remediation Phase 2

## Critical Change Made

We refactored SEE (Static Exchange Evaluation) pruning from global state to thread-local SearchData for thread safety. This is a **COMPLEX CHANGE** that touches multiple critical components.

## What Changed

### Before (Global State):
```cpp
// Global variables in quiescence.cpp
SEEPruningMode g_seePruningMode = SEEPruningMode::OFF;
SEEPruningStats g_seePruningStats;
```

### After (Thread-Local):
- Mode moved to `SearchLimits.seePruningMode` (string)
- Stats moved to `SearchData.seeStats` (struct)
- Mode parsed on every quiescence call
- Stats updated through SearchData reference

## Files Modified

1. `/workspace/src/search/quiescence.h` - Removed globals, kept enum/structs
2. `/workspace/src/search/quiescence.cpp` - Major refactoring of SEE logic
3. `/workspace/src/search/types.h` - Added SEEStats to SearchData
4. `/workspace/src/uci/uci.cpp` - Changed how SEE mode is set
5. `/workspace/src/search/negamax.cpp` - Updated SEE stats reporting

## Potential Bug Areas

### 1. Mode Parsing Overhead
- Now parsing string to enum on EVERY quiescence call
- Could impact performance in deep searches
- Watch for time losses

### 2. Stats Accuracy
- Stats now per-thread (when we add threading)
- Aggregation logic not yet implemented
- Could report incorrect stats

### 3. Initialization Issues
- SearchData must properly initialize seeStats
- Reset must clear all SEE counters
- Missing initialization could cause undefined behavior

### 4. String Comparison
- Using string comparison for mode ("off", "conservative", "aggressive")
- Case sensitivity issues possible
- Typos in UCI could silently fail

## Testing Recommendations

If you encounter bugs after this date related to:
- SEE pruning not working
- Incorrect move pruning
- Performance regression
- Stats reporting issues
- Crashes in quiescence

**CHECK THESE AREAS FIRST:**

1. Verify `limits.seePruningMode` is correctly set from UCI
2. Check SearchData initialization includes seeStats
3. Verify string parsing in quiescence.cpp line ~293
4. Check stats aggregation if multi-threaded
5. Ensure SearchData::reset() clears SEE stats

## Why This Change Is Risky

1. **Hot Path Impact**: Quiescence is called millions of times
2. **String Operations**: Added string parsing in performance-critical code
3. **State Distribution**: Moved from single global to distributed state
4. **Complex Logic**: SEE pruning has multiple conditions and thresholds
5. **Limited Testing**: Only tested single-threaded so far

## Rollback Instructions

If this causes issues, to rollback:
1. Restore globals in quiescence.cpp
2. Remove SEE fields from SearchData/SearchLimits
3. Revert UCI changes for SEE mode setting
4. Rebuild and test

## Original Commit
- Branch: `remediate/stage14-quiescence-search`
- Phase: 2 (Algorithm Improvements)
- Date: 2025-08-17

## Notes
This refactoring was done for thread safety in preparation for Lazy SMP, but adds complexity that may not be worth it if we don't implement multi-threading soon. Consider keeping this change isolated until multi-threading is actually implemented.