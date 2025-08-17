# Stage 14 Remediation Change Log

## Phase 1: Critical Fixes

### Change 1: Convert Compile-Time Node Limits to Runtime UCI Option
**Status:** COMPLETED
**Files Modified:**
- [x] `/workspace/src/search/quiescence.h` - Removed compile-time NODE_LIMIT_PER_POSITION
- [x] `/workspace/src/search/quiescence.cpp` - Updated to use runtime limit from SearchLimits, added originalAlpha for TT bounds
- [x] `/workspace/src/uci/uci.cpp` - Added QSearchNodeLimit UCI option handler
- [x] `/workspace/src/uci/uci.h` - Added m_qsearchNodeLimit member
- [x] `/workspace/src/search/types.h` - Added qsearchNodeLimit to SearchLimits
- [x] `/workspace/src/search/negamax.h` - Added SearchLimits parameter
- [x] `/workspace/src/search/negamax.cpp` - Pass limits through to quiescence

**Changes:**
- Removed #ifdef QSEARCH_TESTING/TUNING/PRODUCTION compile-time constants
- Added UCI option: QSearchNodeLimit (spin, default 0, min 0, max 10000000)
- Modified quiescence() to accept SearchLimits parameter
- Updated all call sites to pass limits through the call chain
- 0 = unlimited (production), 10000 = testing equivalent, 100000 = tuning equivalent

**Validation:**
- ✅ Compilation successful
- ✅ Runtime option works correctly

---

### Change 2: Remove QSEARCH_MODE from CMakeLists.txt
**Status:** COMPLETED
**Files Modified:**
- [x] `/workspace/CMakeLists.txt` - Removed QSEARCH_MODE option and compilation logic
- [x] `/workspace/Makefile` - Removed QSEARCH_MODE from OpenBench build (minimal change)

**Changes:**
- Removed set(QSEARCH_MODE ...) option definition
- Removed if/elseif blocks for QSEARCH_TESTING/TUNING compilation
- Removed -DQSEARCH_MODE from Makefile cmake invocation
- Added comments indicating runtime UCI control

**Validation:**
- ✅ CMake configuration works
- ✅ No compile-time mode selection

---

### Change 3: Consolidate Build Scripts
**Status:** COMPLETED
**Files Modified:**
- [x] `/workspace/build.sh` - Simplified to only Debug/Release options
- [x] `/workspace/build_testing.sh` - REMOVED
- [x] `/workspace/build_tuning.sh` - REMOVED
- [x] `/workspace/build_production.sh` - REMOVED

**Changes:**
- Rewrote build.sh to only accept Debug/Release parameter
- Removed all QSEARCH_MODE handling from build script
- Added UCI option documentation to build output
- Removed three mode-specific build scripts
- Kept build_debug.sh for specialized debug builds

**Validation:**
- ✅ Single build script works
- ✅ No mode-specific builds needed

---

### Change 4: Fix TT Bound Classification Logic
**Status:** COMPLETED
**Files Modified:**
- [x] `/workspace/src/search/quiescence.cpp` - Fixed bound classification

**Changes:**
- Added originalAlpha tracking at function entry
- Fixed TT bound logic:
  - LOWER bound when score >= beta (fail-high)
  - EXACT bound when score > originalAlpha (not beta)
  - UPPER bound when score <= originalAlpha (fail-low)
- Removed incorrect move list empty checks from bound determination
- Added TODO comment for Phase 2 best move tracking

**Validation:**
- ✅ Logic reviewed and corrected
- ✅ Compiles without errors

---

## Phase 2: Algorithm Improvements (Not Started)

## Phase 3: Performance Optimizations (Not Started)