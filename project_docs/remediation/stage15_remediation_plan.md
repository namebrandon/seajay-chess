# Stage 15 SEE Remediation Plan

**Document Version:** 1.0  
**Date:** August 19, 2025  
**Stage:** Phase 3, Stage 15 - Static Exchange Evaluation (SEE)  
**Status:** READY FOR REVIEW  
**Priority:** HIGH - Critical algorithm bug found  

## Executive Summary

Stage 15 SEE implementation is largely complete with UCI runtime control, but contains a **CRITICAL BUG** in the swap list evaluation that causes incorrect SEE values. Additionally, compile-time flags from earlier stages (USE_MAGIC_BITBOARDS, ENABLE_QUIESCENCE) still exist and must be removed.

**Expected Time:** 4-6 hours implementation + SPRT testing  
**Risk Level:** HIGH - Algorithm bug affects move ordering accuracy  
**Confidence:** HIGH - Clear fixes identified  

## Phase Structure Overview

Following our remediation plan, this will be executed in distinct phases:

### Phase 1: Comprehensive Audit ✅ COMPLETE
- Reviewed implementation: SEE properly uses UCI options
- Found CRITICAL BUG: Swap list evaluation using min instead of max
- Found remnant compile flags from Stages 10 and 14
- Identified 3 test utilities for validation

### Phase 2: Critical Bug Fixes (1-2 hours)
- Fix swap list evaluation bug (min→max)
- Remove USE_MAGIC_BITBOARDS ifdefs
- Remove ENABLE_QUIESCENCE from CMake
- Validate fixes with test positions
- **⚠️ CRITICAL: Get bench count and commit with "bench <node-count>" in message**
- **REQUEST HUMAN OPENBENCH VALIDATION**

### Phase 3: OpenBench Testing Round 1
- Human submits to OpenBench
- Monitor SPRT results
- Fix any issues found
- Document results

### Phase 4: Testing & Validation (2-3 hours)
- Cross-validate with Stockfish SEE
- Run existing test suite
- Performance benchmarking
- Create validation report
- **⚠️ CRITICAL: Get bench count and commit with "bench <node-count>" in message**
- **REQUEST HUMAN OPENBENCH VALIDATION**

### Phase 5: OpenBench Testing Round 2
- Human submits to OpenBench
- Monitor SPRT results
- Fix any issues found
- Document results

### Phase 6: UCI Default Optimization (2-3 hours + OpenBench time)
- Test different UCI option combinations via OpenBench
- Determine optimal defaults through SPRT testing
- Document performance impact of each setting
- **⚠️ CRITICAL: Get bench count and commit with "bench <node-count>" in message**
- **REQUEST HUMAN OPENBENCH VALIDATION**

### Phase 7: OpenBench Default Testing
- Human runs OpenBench tests for UCI defaults:
  - Test 1: SEEMode off vs testing vs shadow vs production
  - Test 2: SEEPruning off vs conservative vs aggressive
  - Test 3: Best SEEMode + best SEEPruning combination
- Document results and ELO gains
- Update defaults based on test results

### Phase 8: Final Configuration & Documentation (1 hour)
- Update UCI defaults to optimal values from Phase 7
- Document final UCI options and their impact
- Update CLAUDE.md with final settings
- Create completion report
- **⚠️ CRITICAL: Get bench count and commit with "bench <node-count>" in message**

### Phase 9: Final SPRT Testing & Merge
- Test complete Stage 15 with optimal defaults against Stage 14 baseline
- Await human approval
- Create reference branch
- Merge to main

## Detailed Findings

### ✅ What's Working Well

1. **UCI Runtime Control**: SEE properly configured with UCI options
   - SEEMode: off/testing/shadow/production
   - SEEPruning: off/conservative/aggressive
   - No compile-time SEE flags (good!)

2. **Architecture**: 
   - Swap algorithm implementation (correct approach)
   - Cache with 16K entries
   - X-ray detection for sliding pieces
   - Thread-safe with atomics and thread-local storage

3. **Integration**:
   - Move ordering has multiple modes for gradual rollout
   - Quiescence pruning with configurable thresholds
   - Statistics tracking for monitoring

### ❌ Critical Issues Found

#### 1. **CRITICAL ALGORITHM BUG: Swap List Evaluation**

**Location:** `/workspace/src/core/see.cpp`, lines 454-456  
**Issue:** Using `std::min` instead of `std::max` for negamax evaluation  

```cpp
// CURRENT (WRONG):
m_swapList.gains[m_swapList.depth - 1] = 
    std::min(m_swapList.gains[m_swapList.depth - 1], 
            -m_swapList.gains[m_swapList.depth]);

// SHOULD BE:
m_swapList.gains[m_swapList.depth - 1] = 
    std::max(m_swapList.gains[m_swapList.depth - 1], 
            -m_swapList.gains[m_swapList.depth]);
```

**Impact:** Causes incorrect SEE values, leading to poor move ordering  
**Priority:** IMMEDIATE FIX REQUIRED  

#### 2. **Compile-Time Flags Still Present**

**USE_MAGIC_BITBOARDS** (Stage 10 remnant):
- Location: `/workspace/src/core/move_generation.cpp`, lines 532-558
- Fix: Remove ifdefs, always use magic bitboards

**ENABLE_QUIESCENCE** (Stage 14 remnant):
- Location: `/workspace/CMakeLists.txt`, line 52
- Fix: Remove compile definition

#### 3. **Default Configuration Issue**

- Current default: SEEMode = "off"
- Should be: "testing" or "shadow" for gradual validation
- Allows monitoring before full production use

## Remediation Phases

### Phase 1: Branch Setup ✅ COMPLETE
```bash
git checkout main
git pull origin main
git bugfix stage15-see-remediation
# Creates: bugfix/20250819-stage15-see-remediation
```

### Phase 2: Critical Bug Fixes

**⚠️ OPENBENCH REQUIREMENT:**
Before committing after this phase, MUST get bench count:
```bash
echo "bench" | ./bin/seajay | grep "Benchmark complete" | awk '{print $4}'
```
Commit message MUST include: "bench <node-count>" for OpenBench compatibility!

#### Task 2.1: Fix Swap List Evaluation Bug
```cpp
// File: /workspace/src/core/see.cpp, line 454-456
// Change min to max for correct negamax evaluation
m_swapList.gains[m_swapList.depth - 1] = 
    std::max(m_swapList.gains[m_swapList.depth - 1], 
            -m_swapList.gains[m_swapList.depth]);
```

#### Task 2.2: Remove USE_MAGIC_BITBOARDS ifdefs
```cpp
// File: /workspace/src/core/move_generation.cpp
// Remove all #ifdef USE_MAGIC_BITBOARDS blocks
// Always use magic bitboard functions
```

#### Task 2.3: Remove ENABLE_QUIESCENCE from CMake
```cmake
# File: /workspace/CMakeLists.txt
# Remove line: add_compile_definitions(ENABLE_QUIESCENCE)
```

#### Task 2.4: Update Default SEE Mode
```cpp
// File: /workspace/src/uci/uci.cpp
// Change default from "off" to "testing"
m_seeMode = "testing";  // Was "off"
```

### Phase 4: Testing & Validation

**⚠️ OPENBENCH REQUIREMENT:**
Before committing after this phase, MUST get bench count:
```bash
echo "bench" | ./bin/seajay | grep "Benchmark complete" | awk '{print $4}'
```
Commit message MUST include: "bench <node-count>" for OpenBench compatibility!

#### Test Suite Execution
```bash
# Run SEE-specific tests
./bin/test_see_basic
./bin/test_see_special
./bin/see_demo

# Performance benchmark
./bin/seajay bench
```

#### SEE Validation Strategy

**Note on Stockfish Comparison:**
We CANNOT directly compare SEE values with Stockfish because:
1. Stockfish uses different piece values optimized for its evaluation
2. Stockfish's SEE is tuned for its specific search depths and pruning margins
3. Stockfish includes advanced features we don't have (pins, discovered attacks in some contexts)

**What We CAN Validate:**
1. **Basic Exchange Sequences** - Both engines should agree on simple captures:
   - Undefended pieces (SEE > 0)
   - Equal exchanges (SEE = 0)
   - Bad captures like QxP defended by pawn (SEE < 0)

2. **Move Ordering Agreement** - Compare if both engines order captures similarly:
   - Best captures first (winning exchanges)
   - Equal exchanges in the middle
   - Losing captures last

3. **Sign Agreement** - Even if values differ, the sign should usually match:
   - Positive SEE = good capture (both engines agree)
   - Negative SEE = bad capture (both engines agree)
   - Zero SEE = equal exchange (both engines agree)

#### Critical Test Positions
```
// BASIC VALIDATION POSITIONS (should match signs with any engine):

// 1. Undefended piece (SEE should be positive)
8/8/8/4p3/3P4/8/8/8 w - - 0 1
// dxe5: SEE = +100 (pawn value)

// 2. Equal exchange (SEE should be zero)
8/8/3p4/4P3/8/8/8/8 w - - 0 1  
// exd6: SEE = 0 (pawn for pawn)

// 3. Bad capture - Queen takes defended pawn (SEE should be negative)
8/3p4/4p3/4P3/3Q4/8/8/8 w - - 0 1
// Qxe6: SEE = -850 (lose queen for pawn)

// 4. Knight fork position (test piece values)
8/2p5/3p4/4N3/8/8/8/8 w - - 0 1
// Nxc6 or Nxd6: SEE = +100 (undefended pawns)

// 5. X-ray test - Rook behind rook
8/8/3p4/8/3R4/8/3R4/8 w - - 0 1
// Rxd6: SEE = +100 (pawn value, second rook backs up)
```

#### Validation Script
```bash
# validate_see_basic.sh - Test basic SEE agreement on sign
#!/bin/bash
# Tests positions where ALL engines should agree on SEE sign
# Not comparing exact values, just positive/negative/zero

positions=(
  "8/8/8/4p3/3P4/8/8/8 w - - 0 1:d4e5:positive"
  "8/8/3p4/4P3/8/8/8/8 w - - 0 1:e5d6:zero"
  "8/3p4/4p3/4P3/3Q4/8/8/8 w - - 0 1:d4e6:negative"
)

for pos in "${positions[@]}"; do
  fen="${pos%%:*}"
  move="${pos#*:}"
  move="${move%%:*}"
  expected="${pos##*:}"
  
  # Get our SEE value
  our_see=$(./bin/see_demo "$fen" "$move" | grep "Final SEE")
  
  echo "Position: $fen"
  echo "Move: $move"
  echo "Expected: $expected"
  echo "Our SEE: $our_see"
done
```

### Phase 6: Configuration & Documentation

**⚠️ OPENBENCH REQUIREMENT:**
Before committing after this phase, MUST get bench count:
```bash
echo "bench" | ./bin/seajay | grep "Benchmark complete" | awk '{print $4}'
```
Commit message MUST include: "bench <node-count>" for OpenBench compatibility!

#### UCI Option Testing Strategy

**Phase 7 OpenBench Testing Matrix:**

```
Test 1: SEEMode Comparison (SEEPruning=off for all)
- Base: SEEMode=off
- Test A: SEEMode=testing
- Test B: SEEMode=shadow  
- Test C: SEEMode=production
Goal: Determine best SEEMode setting

Test 2: SEEPruning Comparison (using best SEEMode from Test 1)
- Base: SEEPruning=off
- Test A: SEEPruning=conservative (-100 threshold)
- Test B: SEEPruning=aggressive (-50 to -75 threshold)
Goal: Determine if pruning helps and which threshold

Test 3: Combined Optimization
- Base: SEEMode=off, SEEPruning=off
- Test: Best SEEMode + Best SEEPruning from above
Goal: Verify combined improvement

Test 4: Safe Defaults vs Optimal (if different)
- Base: Safe defaults (SEEMode=testing, SEEPruning=off)
- Test: Optimal from Test 3
Goal: Determine if aggressive defaults are stable
```

**Current UCI Options (to be optimized):**
```
SEEMode:
  - off: Use MVV-LVA only (fallback)
  - testing: Use SEE with extensive logging
  - shadow: Calculate both, use MVV-LVA (A/B testing)
  - production: Full SEE usage
  Default: TBD after Phase 7 testing (currently off)

SEEPruning:
  - off: No SEE-based pruning
  - conservative: Prune captures with SEE < -100
  - aggressive: Prune with SEE < -50 to -75
  Default: TBD after Phase 7 testing (currently off)
```

**Expected Outcomes:**
- SEEMode: Likely "production" for maximum strength
- SEEPruning: Likely "conservative" for safety/strength balance
- Combined: +30-50 ELO expected from proper SEE integration

### Phase 9: Final SPRT Testing & Merge

#### OpenBench Configuration
```json
{
  "base_engine": {
    "repository": "https://github.com/namebrandon/seajay-chess",
    "commit": "openbench/remediated-stage14"
  },
  "dev_engine": {
    "repository": "https://github.com/namebrandon/seajay-chess",
    "commit": "[stage15-remediation-sha]"
  },
  "time_control": "10+0.1",
  "book": "8moves_v3.pgn"
}
```

## Risk Assessment

| Risk | Impact | Likelihood | Mitigation |
|------|--------|------------|------------|
| Swap list bug causes incorrect SEE | HIGH | CONFIRMED | Fix immediately |
| Performance regression | MEDIUM | LOW | Benchmark before/after |
| Integration issues | MEDIUM | LOW | Gradual rollout via modes |
| Test validation failures | LOW | MEDIUM | Cross-check with Stockfish |

## Success Criteria

### Must Have (Phase Gate)
- [ ] Swap list bug fixed and verified
- [ ] All compile-time flags removed
- [ ] Perft tests still pass
- [ ] SEE test suite passes
- [ ] Bench command works
- [ ] UCI defaults determined through OpenBench testing
- [ ] Final configuration shows positive ELO gain

### Should Have
- [ ] Performance within 10% of baseline
- [ ] Sign validation passes on basic positions
- [ ] Optimal UCI defaults identified and set
- [ ] Documentation updated with performance data

### Nice to Have
- [ ] SEE cache hit rate > 30%
- [ ] Tuned piece values
- [ ] Additional test positions

## Validation Checklist

### Pre-Implementation
- [x] Review original Stage 15 plan
- [x] Audit current implementation
- [x] Consult chess-engine-expert
- [x] Document all findings

### Implementation
- [ ] Fix swap list evaluation bug
- [ ] Remove USE_MAGIC_BITBOARDS ifdefs
- [ ] Remove ENABLE_QUIESCENCE from CMake
- [ ] Update default SEE mode
- [ ] Build and test locally

### Testing
- [ ] Run test_see_basic
- [ ] Run test_see_special
- [ ] Cross-validate with Stockfish
- [ ] Benchmark performance
- [ ] Test UCI options work

### Documentation
- [ ] Update CLAUDE.md
- [ ] Document test utilities
- [ ] Create completion report
- [ ] Update OpenBench Testing Index

### Deployment
- [ ] **GET BENCH COUNT**: `echo "bench" | ./bin/seajay | grep "Benchmark complete" | awk '{print $4}'`
- [ ] **COMMIT WITH BENCH**: Include "bench <node-count>" in commit message
- [ ] Push to GitHub
- [ ] Request human to submit SPRT test
- [ ] Await human approval
- [ ] Create reference branch
- [ ] Merge to main

## Timeline Estimate

- **Phase 1**: Branch Setup - ✅ Complete
- **Phase 2**: Bug Fixes - 1-2 hours
- **Phase 3**: OpenBench Round 1 - 2-4 hours
- **Phase 4**: Testing & Validation - 2-3 hours
- **Phase 5**: OpenBench Round 2 - 2-4 hours
- **Phase 6**: UCI Default Optimization - 2-3 hours
- **Phase 7**: OpenBench Default Testing - 4-8 hours (multiple tests)
- **Phase 8**: Final Configuration - 1 hour
- **Phase 9**: Final SPRT Testing - 4+ hours
- **Total**: 18-28 hours + SPRT wait time

## Key Commands

```bash
# Build
make clean && make -j4 seajay

# Test SEE directly
./bin/test_see_basic
./bin/test_see_special

# Get bench count for OpenBench (CRITICAL FOR EVERY COMMIT!)
echo "bench" | ./bin/seajay | grep "Benchmark complete" | awk '{print $4}'

# Example commit with bench count:
git commit -m "fix: SEE swap list evaluation bug - bench 1234567"

# Test UCI options
echo -e "uci\\nsetoption name SEEMode value testing\\nisready\\nquit" | ./bin/seajay
```

## Expert Recommendations

From chess-engine-expert review:
1. **CRITICAL**: Fix swap list bug immediately
2. **HIGH**: Remove all compile flags for consistent behavior
3. **MEDIUM**: Validate against Stockfish SEE
4. **LOW**: Consider tuning piece values after validation

## Utilities Documentation

### see_demo
**Purpose**: Interactive SEE demonstration
**Usage**: `./bin/see_demo`
**Stage**: 15
**Notes**: Shows SEE calculation step-by-step

### test_see_basic
**Purpose**: Basic SEE functionality tests
**Usage**: `./bin/test_see_basic`
**Stage**: 15
**Notes**: Tests simple exchanges

### test_see_special
**Purpose**: Special cases (en passant, promotions)
**Usage**: `./bin/test_see_special`
**Stage**: 15
**Notes**: Edge case validation

## Lessons from Previous Remediations

1. **Always check for remnant flags** from earlier stages
2. **Cross-validate algorithms** with reference engines
3. **Use gradual rollout** for complex features
4. **Document everything** for future reference
5. **Test comprehensively** before SPRT

## Final Notes

Stage 15 SEE is well-architected with proper UCI control, but the swap list evaluation bug is CRITICAL and must be fixed immediately. The gradual rollout strategy (testing→shadow→production) is excellent for safe deployment.

**Critical Success Factor**: The UCI defaults MUST be determined through systematic OpenBench testing, not guesswork. Phase 7 testing will determine:
1. Which SEEMode provides best ELO (likely "production")
2. Whether SEEPruning helps (likely "conservative" is best)
3. The combined effect of optimal settings

Only after Phase 7 OpenBench testing will we know the correct defaults to ship. After fixing the identified issues and optimizing defaults, SEE should provide the expected +30-50 ELO improvement through better move ordering.

**Priority Actions**:
1. Fix the swap list bug immediately
2. Remove compile-time flags
3. Systematically test UCI combinations via OpenBench
4. Set defaults based on test results, not assumptions

## Approval Gate

**DO NOT PROCEED TO IMPLEMENTATION WITHOUT:**
- [ ] Human review of this plan
- [ ] Confirmation that swap list bug fix is correct
- [ ] Agreement on default mode change
- [ ] SPRT testing strategy approved

---

*Remember: NO MERGE TO MAIN WITHOUT HUMAN APPROVAL after SPRT testing completes*