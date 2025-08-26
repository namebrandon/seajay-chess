# SeaJay Engine Master Development Plan

**Created:** August 25, 2025  
**Purpose:** Organize and prioritize all investigation findings and development work  
**Status:** ACTIVE - Guiding development priorities

## Document Hierarchy

```
master_plan.md (THIS DOCUMENT - Top Level)
├── seajay_investigation_report.md (Original game analysis - 29 games)
│   └── depth_deficit_investigation_plan.md (Deep dive into Issue #1)
│       ├── depth_deficit_investigation_results.md (Test results)
│       └── search_extensions_analysis.md (Reference engine comparison)
├── pruning_aggressiveness_analysis.md (Deep dive into Issue #3)
│   └── pruning_plan.md (Implementation plan with phases)
├── docs/project_docs/deferred_items_tracker.md (Long-term tracking)
└── Future investigation docs...
```

## Current Status Summary

### ✅ Completed
1. **Game Analysis** - Identified 3 critical issues from 29 games
2. **Check Extensions** - Implemented and verified (+28 Elo)
3. **Initial Depth Investigation** - Root causes identified
4. **Pruning Analysis** - Comprehensive comparison with Laser engine completed

### 🔄 In Progress
1. **Depth Deficit Resolution** - Partially addressed, more work needed
2. **Search Architecture Issues** - Documented, needs refactoring
3. **Pruning Improvements** - Analysis complete, implementation planned

### ❌ Not Started
1. **Move Ordering Improvements** - 76.8% efficiency needs fixing
2. **LMR Tuning** - Currently too aggressive
3. **Time Management Fixes** - Still has 40% early exit

---

## PHASE 1: Critical Issues from Game Analysis (CURRENT)

From `seajay_investigation_report.md`, we identified three critical issues:

### Issue #1: Search Depth Deficit [10-12 ply behind]
**Status:** PARTIALLY ADDRESSED  
**Document:** `depth_deficit_investigation_plan.md`

#### Completed:
- [x] H1: Check extensions implemented (+28 Elo) ✅
- [x] H2-H5: Other hypotheses tested and documented

#### Remaining Work:
- [ ] **Singular Extensions** - Blocked by architecture (see below)
- [ ] **Move Ordering Fix** - 76.8% efficiency (Target: >85%)
- [ ] **LMR Tuning** - Less aggressive reductions needed
- [ ] **Time Management** - Remove 40% early exit

### Issue #2: Tactical Blindness [171 huge blunders]
**Status:** PARTIALLY IMPROVED (check extensions help)
**Next Steps:** 
- [ ] Complete depth deficit fixes first (will improve tactics)
- [ ] Add tactical test suite validation
- [ ] Consider tactical extensions (pins, forks)

### Issue #3: Overly Aggressive Pruning [20% fewer nodes]
**Status:** ANALYZED - Root causes identified  
**Document:** `pruning_aggressiveness_analysis.md` & `pruning_plan.md`  
**Priority:** HIGH (causing tactical blindness)

#### Key Findings:
- **Missing Techniques:** No futility pruning, move count pruning, razoring
- **Overly Aggressive:** Null move margin 71% higher than Laser (120cp vs 70cp)
- **No Safety Checks:** Missing null move verification at high depths

#### Implementation Plan:
- [ ] **Phase 1:** Add missing futility pruning (biggest impact)
- [ ] **Phase 2:** Reduce aggressive parameters to conservative values
- [ ] **Phase 3:** Implement move count pruning
- [ ] **Phase 4:** Add razoring and advanced techniques
**Expected Gain:** Reduce blunders from 171 to <50, +15-20% nodes

---

## PHASE 2: Search Architecture Refactoring (BLOCKING)

**Critical Discovery:** Our excluded move pattern blocks multiple features  
**Document:** `docs/project_docs/deferred_items_tracker.md` (Section: Search Architecture Refactoring)

### The Problem:
- SeaJay uses recursive exclusion (different from all successful engines)
- Blocks: Singular extensions, Multi-cut, Probcut, IID
- Breaks move counting and statistics

### Required Work:
1. [ ] Create specialized search functions (searchSingular, searchProbcut, etc.)
2. [ ] Implement explicit move iteration pattern (like Stockfish/Laser)
3. [ ] Remove excluded move mechanism entirely
4. [ ] Test and validate refactored search

**Estimated Effort:** 10-15 hours  
**Priority:** HIGH - Blocking multiple improvements

---

## PHASE 3: Immediate Improvements (Can Do Now)

These don't require architecture changes:

### 3.1 Time Management Fixes
**File:** `src/search/time_management.cpp`
- [ ] Remove 40% early exit rule (line 1123-1125 in negamax.cpp)
- [ ] Test more aggressive time usage
- [ ] Implement soft/hard limit properly
**Expected Gain:** 2-3 ply deeper searches

### 3.2 Move Ordering Improvements
**Current:** 76.8% efficiency (poor)
- [ ] Fix killer move implementation
- [ ] Verify history heuristic decay
- [ ] Add countermove heuristic
**Expected Gain:** 20-30% fewer nodes, +1-2 ply

### 3.3 LMR Parameter Tuning
**File:** `src/search/lmr.cpp`
- [ ] Less aggressive base reduction
- [ ] Don't reduce killers/good history moves
- [ ] Test logarithmic vs linear formula
**Expected Gain:** Better move selection, +15-20 Elo

---

## PHASE 4: Advanced Features (After Architecture Fix)

These require the search refactoring from Phase 2:

### 4.1 Singular Extensions (HIGH PRIORITY)
- [ ] Implement proper move-by-move testing
- [ ] Add verification search
- [ ] Track singular extension statistics
**Expected Gain:** +20-40 Elo

### 4.2 Multi-cut Pruning
- [ ] Test multiple moves for cutoff
- [ ] Implement early pruning decision
**Expected Gain:** +10-15 Elo

### 4.3 Probcut
- [ ] Shallow search to predict deep cutoff
- [ ] Requires proper search separation
**Expected Gain:** +10-15 Elo

---

## Priority Order (What to Do Next)

### Immediate (This Week):
1. **Pruning Fixes Phase 1** - Reduce aggressive parameters (4 hours)
2. **Futility Pruning** - Implement missing technique (6 hours)
3. **Time Management Fix** - Remove 40% rule (2 hours)

### Short Term (Next 2 Weeks):
1. **Move Count Pruning** - Implement with depth tables (5 hours)
2. **Null Move Verification** - Add safety checks (3 hours)
3. **Move Ordering Analysis** - Why only 76.8%? (4 hours)
4. **LMR Tuning** - Less aggressive (2 hours)

### Medium Term (Next Month):
1. **Complete Depth Deficit Resolution** - Reach 15+ ply consistently
2. **Tactical Test Suite** - Validate improvements
3. **Advanced Pruning Techniques** - Multi-cut, Probcut

---

## Success Metrics

### Target Improvements:
- **Search Depth:** From 10-11 ply → 15+ ply minimum
- **Move Ordering:** From 76.8% → 85%+ efficiency  
- **Tactical Blunders:** From 171 → <50 per 29 games
- **ELO Gain:** +100-150 total from all improvements

### Validation Methods:
1. **Depth Tests:** Same positions, compare depth reached
2. **Tactical Suite:** Standard test positions
3. **Self-play:** SPRT testing for each change
4. **Game Analysis:** Re-run vs 4ku/Laser after fixes

---

## Risk Management

### High Risk Items:
1. **Search Refactoring** - Could break existing functionality
   - Mitigation: Extensive testing, keep old code available
2. **LMR Changes** - Could miss tactics
   - Mitigation: Incremental changes with SPRT validation

### Dependencies:
- Singular extensions BLOCKED until search refactoring
- Advanced pruning BLOCKED until singular extensions work
- Full depth improvement needs ALL components

---

## Working Documents Reference

### Investigation Phase:
- `seajay_investigation_report.md` - Original 29-game analysis
- `depth_deficit_investigation_plan.md` - Deep dive planning
- `depth_deficit_investigation_results.md` - Test results
- `search_extensions_analysis.md` - Reference engine comparison
- `pruning_aggressiveness_analysis.md` - Pruning comparison with Laser
- `pruning_plan.md` - Detailed implementation plan for pruning fixes

### Implementation Tracking:
- `docs/project_docs/deferred_items_tracker.md` - Long-term items
- `feature_status.md` - Current feature branch status (when active)

### Code Locations:
- Time Management: `src/search/time_management.cpp`, `negamax.cpp:1123-1125`
- Move Ordering: `src/search/move_ordering.cpp`, killer/history in `negamax.cpp`
- LMR: `src/search/lmr.cpp`
- Search Extensions: `src/search/negamax.cpp:198-203` (check extension)

---

## Next Actions (TODO Right Now)

1. [ ] Fix time management 40% rule
2. [ ] Run depth comparison test with fixed time management
3. [ ] Analyze move ordering efficiency in detail
4. [ ] Create test position set for validation
5. [ ] Plan search architecture refactoring in detail

---

## Notes

- Check extensions alone gave +28 Elo - other fixes should give similar gains
- The 10-12 ply deficit is our #1 priority - everything else is secondary
- Don't implement advanced features until architecture is fixed
- Each fix should be tested independently with SPRT

This master plan will be updated as work progresses. All subordinate documents should reference back to this plan for context and priority.