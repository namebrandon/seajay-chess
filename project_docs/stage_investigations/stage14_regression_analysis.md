# Stage 14 Regression Analysis and Ongoing Investigation

**Date:** August 14-15, 2025  
**Issue:** Severe performance regression in Candidates 2-3 after attempting time control fixes  
**Status:** ⚠️ **INVESTIGATION ONGOING** - Not yet convinced issues are fully resolved  
**Current Attempt:** Candidate 5 with clean rebuild and properly reverted code  

## Executive Summary

Stage 14 Candidate 1 was showing exceptional performance (300+ ELO gain) with only 1-2% time losses. Attempts to "fix" these rare time losses in Candidates 2-3 resulted in catastrophic performance regression (60-70% game losses). Analysis revealed that the safety measures were destroying tactical play. 

**Critical Discovery:** Candidate 4 failed to actually revert due to build system issues - stale object files were being linked despite source code changes. Candidate 5 represents a clean rebuild with properly reverted code. **Testing is ongoing to confirm whether this resolves the regression.**

## Performance Timeline

### Candidate 1 (Original Implementation)
- **Performance:** 300+ ELO gain over Stage 13
- **SPRT Result:** Test ended early due to overwhelming success
- **Time Losses:** 1-2% of games
- **Approach:** Unlimited node limit, no emergency cutoff, check time every 1024 nodes

### Candidate 2 (First "Fix" Attempt)
- **Performance:** 60-70% game losses
- **Changes Made:**
  - Added emergency cutoff (skip quiescence if <50ms)
  - Limited nodes to 1M per position
  - Increased time checking frequency (every 64 nodes)
  - Increased time buffer (100ms → 200ms)
- **Result:** Complete failure

### Candidate 3 (Adjusted Fix)
- **Performance:** Still 60-70% game losses
- **Changes Made:**
  - Reduced emergency cutoff to 10ms
  - Increased node limit to 10M
  - Kept frequent time checking and larger buffer
- **Result:** Still failing badly

### Candidate 4 (Failed Reversion Attempt)
- **Approach:** Attempted to revert to Candidate 1 settings
- **Changes in source:**
  - NODE_LIMIT: Back to UINT64_MAX (unlimited)
  - Emergency cutoff: REMOVED entirely
  - Time checking: Back to every 1024 nodes
  - Time buffer: Back to 100ms
- **Actual Result:** ❌ **BUILD SYSTEM FAILURE** - Binary had same size as C2/C3 (384KB)
- **Discovery:** Changes weren't compiled due to stale object files

### Candidate 5 (Clean Rebuild - Testing Pending)
- **Approach:** Clean rebuild with properly reverted code
- **Build Process:**
  - Fixed build scripts to force `make clean`
  - Removed all stale object files
  - Fresh compilation of reverted code
- **Binary Details:**
  - Size: 384KB (same as C2-C4, different from C1's 411KB)
  - MD5: 78781e3c12eec07c1db03d4df1d4393a (unique)
- **Status:** ⚠️ **AWAITING TEST RESULTS**

## Root Cause Analysis

### Build System Issues Discovered

**Critical Finding:** The build system was not properly recompiling changed source files:
- CMake cache and object files were persisting between builds
- Build scripts only cleaned CMake files, not compiled objects
- This caused Candidates 2, 3, and 4 to all use incorrect code
- Binary checksums confirm all are different builds, but not with intended changes

**Evidence:**
```
Candidate 1: 411,336 bytes - MD5: 0b0ea4c7f8f0aa60079a2ccc2997bf88
Candidate 2: 384,312 bytes - MD5: e49aaaef8504c59342dfc8b995df45ed  
Candidate 3: 384,312 bytes - MD5: dc9d7ea9fb5725b582ab2c7ab493658d
Candidate 4: 384,312 bytes - MD5: b4248104f35b0a9aeb52c50b8f6371e3
Candidate 5: 384,312 bytes - MD5: 78781e3c12eec07c1db03d4df1d4393a
```

### The Fatal Flaws in Candidates 2-3

1. **Node Limit Catastrophe**
   - Even 10M nodes is insufficient for complex tactical positions
   - Cutting off quiescence mid-calculation causes massive misevaluations
   - Stage 13 could see tactics that Stage 14 couldn't complete

2. **Emergency Cutoff Disaster**
   - Skipping quiescence entirely when time is low = tactical blindness
   - Happens frequently in time pressure situations
   - Static eval is no substitute for tactical resolution

3. **Time Buffer Over-correction**
   - Lost 100ms of search time per move
   - Compounds over the game (6+ seconds in 60-move game)
   - Triggered emergency cutoff more frequently

4. **Overhead from Frequent Checks**
   - System clock calls are expensive
   - Checking every 64 vs 1024 nodes = 16x more overhead
   - Reduced actual search efficiency

## The Compounding Effect

These "safety" measures created a death spiral:
```
Less time allocated → Hit emergency cutoff → Skip quiescence → 
Bad tactical moves → Worse positions → Need more time → 
Have less time → More cutoffs → Tactical collapse → Loss
```

## Key Lesson Learned

**"The cure was worse than the disease"**

- 1-2% time losses with 300+ ELO gain = MASSIVE SUCCESS
- 0% time losses with 60-70% game losses = COMPLETE FAILURE

Occasional time losses in won positions are acceptable. Constant tactical blindness is not.

## Chess-Engine-Expert Analysis

The expert's verdict was clear:
> "Better to lose on time occasionally than lose on the board constantly!"

Professional engines like Stockfish and Leela also experience occasional time losses. It's an acceptable trade-off for tactical excellence.

## Implementation Details

### Candidate 5 Settings (Clean rebuild with C1 approach)
```cpp
// quiescence.h
static constexpr uint64_t NODE_LIMIT_PER_POSITION = UINT64_MAX;

// quiescence.cpp
// Emergency cutoff: REMOVED (confirmed in working directory)
// Time checking: if ((data.qsearchNodes & 1023) == 0)

// time_management.cpp
// Buffer: 100ms (confirmed reverted from 200ms)

// Build scripts now include:
make clean 2>/dev/null || true  # Clean object files
rm -rf CMakeCache.txt CMakeFiles/ Makefile
rm -f *.o src/*.o src/*/*.o  # Force remove lingering objects
```

## Validation Strategy

1. ~~Run SPRT tests with Candidate 4~~ (Failed due to build issues)
2. Run SPRT tests with Candidate 5 (clean rebuild)
3. Expect restoration of 300+ ELO gain
4. Accept 1-2% time losses as normal
5. Monitor for tactical strength restoration
6. **If Candidate 5 still shows regression:** Investigate further differences between C1 and C5

## Current Status: INVESTIGATION ONGOING

⚠️ **We are not yet convinced the issues are fully resolved.** While Candidate 5 represents a clean rebuild with properly reverted source code, several concerns remain:

1. **Binary size discrepancy:** C1 is 411KB while C2-C5 are all 384KB
   - Could indicate optimization differences
   - May suggest additional untracked changes
   
2. **Testing not yet complete:** Need SPRT results to confirm restoration
   
3. **Potential remaining issues:**
   - Other code changes between commits not yet identified
   - Compiler optimization differences
   - Build configuration variations

## Next Steps

1. **Immediate:** Run SPRT tests with Candidate 5
2. **If C5 succeeds:** Document resolution and close investigation
3. **If C5 fails:** 
   - Binary diff analysis between C1 and C5
   - Review all commits between C1 and C2
   - Consider rebuilding C1 from its exact commit

## Files Modified

### For Candidate 5:
- `/workspace/src/search/quiescence.cpp` - Removed emergency cutoff, reverted time checking
- `/workspace/src/search/quiescence.h` - Restored unlimited node limit  
- `/workspace/src/search/time_management.cpp` - Restored 100ms buffer
- `/workspace/src/uci/uci.cpp` - Updated version to Candidate 5
- `/workspace/build_*.sh` - Fixed to force clean rebuilds
- All SPRT test scripts updated to use Candidate 5 binary

---
*Last Updated: August 15, 2025 06:55 UTC*  
*Status: Investigation and testing continues*  
*Analysis supported by chess-engine-expert agent*