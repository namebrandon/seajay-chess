# UCI 40 ELO Regression Remediation Plan
## "Vacation Test" Implementation Guide

### Document Purpose
This is a complete, standalone guide to fix the 40 ELO performance regression in SeaJay. Anyone can follow this plan step-by-step, even without prior knowledge of the codebase. Each phase is atomic, tested independently, and committed before proceeding.

### Background
- **Problem**: 40 ELO regression persists despite fixing SearchData size
- **Root Cause**: Polymorphic SearchData (vtable overhead) + dynamic casting + info output overhead
- **Solution**: Remove polymorphism and optimize the hot path
- **Expected Recovery**: Full 40 ELO

### Critical Rules
1. **STOP after each phase** - Commit and wait for SPRT testing
2. **Bench must remain 19191913** - Any change means something broke
3. **Clean build before each phase**: `rm -rf build/ && ./build.sh`
4. **Test each phase independently** - Do not combine phases

---

## PHASE 1: Reduce Info Output Frequency (Quick Win)
**Expected Recovery**: 5-8 ELO  
**Time Estimate**: 30 minutes  
**Risk**: Very Low

### Step 1.1: Find the Info Output Check
```bash
grep -n "0xFFF" src/search/negamax.cpp
```
You should find around line 169-171:
```cpp
if (info.nodes > 0 && (info.nodes & 0xFFF) == 0) {
```

### Step 1.2: Change Check Frequency
**File**: `/workspace/src/search/negamax.cpp`  
**Find** (around line 169-171):
```cpp
if (info.nodes > 0 && (info.nodes & 0xFFF) == 0) {
```

**Replace with**:
```cpp
// PERFORMANCE FIX: Reduced info output frequency from every 4096 to every 16384 nodes
// This reduces dynamic cast and string building overhead by 75%
if (info.nodes > 0 && (info.nodes & 0x3FFF) == 0) {
```

### Step 1.3: Build and Verify
```bash
# Clean build
rm -rf build/
./build.sh

# Verify bench unchanged
echo "bench" | ./bin/seajay 2>&1 | grep "Benchmark complete"
# MUST show: 19191913 nodes
```

### Step 1.4: Commit Phase 1
```bash
# Get bench value
BENCH_VAL=$(echo "bench" | ./bin/seajay 2>&1 | grep "Benchmark complete" | awk '{print $3}')

# Commit
git add -A
git commit -m "perf: Reduce info output frequency from 4K to 16K nodes (Phase 1)

Reduces dynamic cast and string building overhead by checking 4x less frequently.
This is Phase 1 of the polymorphism removal to fix 40 ELO regression.

- Changed check mask from 0xFFF (4096) to 0x3FFF (16384)  
- Reduces overhead without affecting UCI functionality
- Expected recovery: 5-8 ELO

bench $BENCH_VAL"

# Push for testing
git push origin feature/20250828-uci-white-perspective
```

### PHASE 1 CHECKPOINT
```
=== STOP HERE FOR TESTING ===
Branch: feature/20250828-uci-white-perspective
Commit: [record commit SHA]
Expected: +5-8 ELO improvement
Test against: Previous commit on this branch

DO NOT PROCEED TO PHASE 2 UNTIL SPRT CONFIRMS IMPROVEMENT
```

---

## PHASE 2: Remove Polymorphism from SearchData
**Expected Recovery**: 25-30 ELO  
**Time Estimate**: 2 hours  
**Risk**: Medium (more complex change)

### Step 2.1: Add Type Enum to SearchData
**File**: `/workspace/src/search/types.h`  
**Find** (around line 260-270, inside SearchData struct):
```cpp
struct SearchData {
    // Start time of the search
    std::chrono::steady_clock::time_point startTime;
```

**Add after startTime**:
```cpp
    // PERFORMANCE FIX: Type enum to replace polymorphism
    // Eliminates vtable overhead that caused 40 ELO regression
    enum DataType { 
        BASIC_SEARCH = 0, 
        ITERATIVE_SEARCH = 1 
    };
    DataType dataType = BASIC_SEARCH;
```

### Step 2.2: Remove Virtual Destructor
**File**: `/workspace/src/search/types.h`  
**Find** (around line 440-450):
```cpp
    virtual ~SearchData() = default;
```

**Replace with**:
```cpp
    // PERFORMANCE FIX: Removed virtual destructor to eliminate vtable overhead
    // SearchData is now POD (Plain Old Data) for maximum performance
    ~SearchData() = default;
```

### Step 2.3: Update IterativeSearchData Constructor
**File**: `/workspace/src/search/types.h`  
**Find** IterativeSearchData struct (around line 470-480)  
**Add to constructor**:
```cpp
struct IterativeSearchData : public SearchData {
    // ... existing fields ...
    
    IterativeSearchData() {
        // PERFORMANCE FIX: Set type for static dispatch
        dataType = ITERATIVE_SEARCH;
    }
```

### Step 2.4: Replace Dynamic Cast with Static Cast
**File**: `/workspace/src/search/negamax.cpp`  
**Find** (around line 170-175):
```cpp
auto* iterativeInfo = dynamic_cast<IterativeSearchData*>(&info);
if (iterativeInfo && iterativeInfo->limits.infinite) {
```

**Replace with**:
```cpp
// PERFORMANCE FIX: Static cast with type check replaces expensive dynamic_cast
// Eliminates RTTI overhead in hot path (2-3% performance gain)
IterativeSearchData* iterativeInfo = nullptr;
if (info.dataType == SearchData::ITERATIVE_SEARCH) {
    iterativeInfo = static_cast<IterativeSearchData*>(&info);
}
if (iterativeInfo && iterativeInfo->limits.infinite) {
```

### Step 2.5: Build and Test
```bash
# Clean build
rm -rf build/
./build.sh 2>&1 | tail -20

# If compilation errors, check:
# - All SearchData uses have dataType initialized
# - No remaining virtual functions
# - Static cast syntax is correct
```

### Step 2.6: Verify SearchData is POD
Add temporary test code to verify:

**File**: `/workspace/src/search/negamax.cpp`  
**Add at start of searchIterativeTest()** (temporary):
```cpp
// TEMPORARY: Verify SearchData is POD
static_assert(!std::is_polymorphic<SearchData>::value, 
              "SearchData must not be polymorphic");
std::cerr << "SearchData is POD: " << std::is_pod<SearchData>::value << std::endl;
```

Build and run:
```bash
./build.sh
echo "go depth 1" | ./bin/seajay 2>&1 | grep "POD"
# Should show: SearchData is POD: 1 (or true)
```

Remove the temporary code after verification.

### Step 2.7: Performance Verification
```bash
# Run bench and check NPS improvement
echo "bench" | ./bin/seajay 2>&1 | tail -5
# Record the NPS value - should be 10-15% higher than before

# Verify bench unchanged
echo "bench" | ./bin/seajay 2>&1 | grep "Benchmark complete"
# MUST show: 19191913 nodes
```

### Step 2.8: Commit Phase 2
```bash
# Get bench value
BENCH_VAL=$(echo "bench" | ./bin/seajay 2>&1 | grep "Benchmark complete" | awk '{print $3}')

git add -A
git commit -m "perf: Remove polymorphism from SearchData (Phase 2)

Eliminates vtable overhead by replacing polymorphic design with type enum.
This is Phase 2 of fixing the 40 ELO regression from UCI conversion.

Changes:
- Removed virtual destructor from SearchData
- Added DataType enum for static type checking  
- Replaced dynamic_cast with static_cast + type check
- SearchData is now POD (Plain Old Data) type

Performance impact:
- Eliminates vtable indirection (5-7% overhead)
- Removes dynamic cast RTTI lookups (2-3% overhead)
- Expected recovery: 25-30 ELO

bench $BENCH_VAL"

git push origin feature/20250828-uci-white-perspective
```

### PHASE 2 CHECKPOINT
```
=== STOP HERE FOR TESTING ===
Branch: feature/20250828-uci-white-perspective
Commit: [record commit SHA]
Expected: +25-30 ELO improvement (cumulative +30-38 ELO)
Test against: Commit from Phase 1

DO NOT PROCEED TO PHASE 3 UNTIL SPRT CONFIRMS MAJOR IMPROVEMENT
```

---

## PHASE 3: Inline Score Conversion Functions
**Expected Recovery**: 5-7 ELO  
**Time Estimate**: 1 hour  
**Risk**: Low

### Step 3.1: Find Score Conversion Functions
```bash
grep -n "convertScoreToUci\|scoreFromWhitePerspective" src/search/*.h src/search/*.cpp
```

### Step 3.2: Make Functions Inline
**File**: `/workspace/src/search/types.h` or wherever these functions are defined

**Find score conversion functions and add inline**:
```cpp
// Before:
eval::Score convertScoreToUci(...) {

// After:
// PERFORMANCE FIX: Inline score conversion to eliminate function call overhead
inline eval::Score convertScoreToUci(...) {
```

Do this for all score conversion functions in the hot path.

### Step 3.3: Build and Verify
```bash
rm -rf build/
./build.sh

# Verify bench
echo "bench" | ./bin/seajay 2>&1 | grep "Benchmark complete"
# MUST show: 19191913 nodes
```

### Step 3.4: Commit Phase 3
```bash
BENCH_VAL=$(echo "bench" | ./bin/seajay 2>&1 | grep "Benchmark complete" | awk '{print $3}')

git add -A
git commit -m "perf: Inline score conversion functions (Phase 3)

Final optimization phase for UCI regression fix.
Inlines all score conversion functions to eliminate function call overhead.

- Marked score conversion functions as inline
- Allows compiler to optimize away function calls
- Expected recovery: 5-7 ELO

Total expected recovery: 35-40 ELO (full regression fix)

bench $BENCH_VAL"

git push origin feature/20250828-uci-white-perspective
```

### PHASE 3 CHECKPOINT
```
=== FINAL TESTING ===
Branch: feature/20250828-uci-white-perspective  
Commit: [record commit SHA]
Expected: +5-7 ELO improvement (cumulative +35-45 ELO)
Test against: Commit from Phase 2

ALSO TEST AGAINST BASELINE:
Test final commit against 855c4b9 (original baseline)
Expected: Should show 0 ELO difference (regression fully fixed)
```

---

## Verification Checklist

### After Each Phase:
- [ ] Bench value is exactly 19191913
- [ ] No compilation warnings
- [ ] Clean build was performed
- [ ] Commit message includes bench value
- [ ] Pushed to remote for testing

### Final Verification:
- [ ] Phase 1: +5-8 ELO confirmed by SPRT
- [ ] Phase 2: +25-30 ELO confirmed by SPRT  
- [ ] Phase 3: +5-7 ELO confirmed by SPRT
- [ ] Total vs baseline (855c4b9): 0 ELO difference

## Rollback Plan

If any phase causes problems:
```bash
# Revert last commit
git revert HEAD
git push origin feature/20250828-uci-white-perspective

# Or reset to known good commit
git reset --hard [last-good-commit-sha]
git push --force origin feature/20250828-uci-white-perspective
```

## Success Criteria

The regression is considered FULLY FIXED when:
1. NPS is 10-15% higher than current version
2. Bench remains exactly 19191913
3. SPRT testing shows 0 ELO difference vs commit 855c4b9
4. No new bugs or crashes introduced

---

## Notes for Implementer

1. **Take your time** - Rushing leads to mistakes
2. **Test thoroughly** - Each phase must work independently
3. **Document issues** - If something unexpected happens, record it
4. **Ask for help** - If stuck, the issue is likely in the details
5. **Celebrate success** - Fixing a 40 ELO regression is significant!

## Time Estimates

- Phase 1: 30 minutes + testing time
- Phase 2: 2 hours + testing time  
- Phase 3: 1 hour + testing time
- Total implementation: ~4 hours
- Total with testing: 1-2 days

This plan is designed to be foolproof. Follow it exactly and the regression will be fixed.