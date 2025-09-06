# Null Move Refinement (NMR) Implementation Plan

## Executive Summary

This document outlines a staged implementation plan to address critical correctness and performance issues in SeaJay's Null Move Pruning (NMP) and Static Null (reverse futility) pruning implementations. Following SeaJay's strict phased development methodology, we will implement fixes across multiple tested phases to ensure stability and measurable improvement.

## Issues Identified

### Critical Issues (Causing ELO Loss)
1. **Static-null lacks endgame/zugzwang guard** - Main source of residual -5 to -15 nELO loss
2. **TT replacement policy flaws** - Heuristic NO_MOVE entries can overwrite valuable move entries
3. **Static-null depth range too aggressive** - depth <= 8 is too deep for reverse futility

### Minor Issues (Performance/Stability)
4. **Thread-safety concerns** - TT needs atomic operations for future LazySMP
5. **Efficiency issues** - Redundant zobrist key calculations in hot paths

## Implementation Phases

Following SeaJay's feature guidelines, we will implement fixes in small, testable phases with full OpenBench validation between each phase.

---

## Phase NMR-1: Static-Null Endgame Guard (Infrastructure)

### Objective
Add endgame detection infrastructure without changing behavior (0 ELO expected)

### Changes
1. Add endgame detection helper function to Board class
2. Add NPM threshold constants
3. Add UCI option for endgame threshold (default: disabled/very high)

### Implementation Details
```cpp
// In board.h
bool isEndgame(eval::Score npmThreshold = eval::Score(1300)) const;

// In board.cpp  
bool Board::isEndgame(eval::Score npmThreshold) const {
    eval::Score npmUs = nonPawnMaterial(sideToMove());
    eval::Score npmThem = nonPawnMaterial(~sideToMove());
    return npmUs < npmThreshold || npmThem < npmThreshold;
}

// In engine_config.h - add UCI option
int staticNullEndgameThreshold = 9999;  // Effectively disabled
```

### Expected Results
- 0 ELO change (infrastructure only)
- Clean compilation
- Bench count unchanged

### SPRT Bounds
`[-5.00, 3.00]` - Detect any regression from infrastructure

---

## Phase NMR-2: Static-Null Endgame Guard (Activation)

### Objective
Enable endgame guard for static-null pruning to prevent zugzwang issues

### Changes
1. Activate endgame guard in static-null condition
2. Set default threshold to 1300 cp (matching razoring)

### Implementation Details
```cpp
// In negamax.cpp - modify static-null condition (line ~455)
if (!isPvNode && depth <= 8 && depth > 0 && !weAreInCheck 
    && std::abs(beta.value()) < MATE_BOUND - MAX_PLY
    && !board.isEndgame(eval::Score(limits.staticNullEndgameThreshold))) {
    // ... existing static-null code
}

// Update default in engine_config
staticNullEndgameThreshold = 1300;  // Active by default
```

### Expected Results
- +5 to +10 nELO from reduced tactical blindness
- Improved stability at deeper iterations
- Slightly reduced node count in endgames

### SPRT Bounds
`[2.00, 8.00]` - Expect solid gain from bugfix

---

## Phase NMR-3: Static-Null Depth Reduction

### Objective
Reduce maximum depth for static-null from 8 to 4 (standard for reverse futility)

### Changes
1. Change depth condition from `depth <= 8` to `depth <= 4`
2. Add UCI option for max depth (default: 4)

### Implementation Details
```cpp
// In engine_config.h
int staticNullMaxDepth = 4;  // UCI configurable

// In negamax.cpp
if (!isPvNode && depth <= limits.staticNullMaxDepth && depth > 0 ...
```

### Expected Results
- +2 to +5 nELO from reduced over-pruning
- Better tactical accuracy at medium depths
- Small increase in node count

### SPRT Bounds
`[0.00, 5.00]` - Small positive expected

---

## Phase NMR-4: TT Replacement Policy (NO_MOVE Protection)

### Objective
Prevent heuristic NO_MOVE entries from overwriting valuable move entries

### Changes
1. Modify TT replacement logic to protect entries with moves
2. Add generation age factor to replacement decision

### Implementation Details
```cpp
// In transposition_table.cpp::store() - line ~140
if (entry->key32 == key32) {
    // Same position - protect move entries from NO_MOVE overwrites
    if (move == NO_MOVE && entry->move != NO_MOVE) {
        if (depth <= entry->depth) {
            canReplace = false;  // Don't overwrite move with heuristic
        } else if (depth <= entry->depth + 2) {
            // Only replace if significantly deeper
            canReplace = (entry->generation() != m_generation);
        } else {
            canReplace = true;  // Much deeper, replace
        }
    } else if (entry->generation() != m_generation) {
        canReplace = (depth >= entry->depth - 2);  // Grace for old entries
    } else {
        canReplace = (depth >= entry->depth);  // Same gen, need equal/deeper
    }
}
```

### Expected Results
- +3 to +6 nELO from improved TT move availability
- Higher first-move cutoff rate
- Better move ordering quality

### SPRT Bounds
`[0.00, 5.00]` - Modest gain expected

---

## Phase NMR-5: TT Collision Handling

### Objective
Improve replacement policy for hash collisions (different keys)

### Changes
1. Be more conservative with NO_MOVE entries on collisions
2. Always allow replacing very old entries to prevent lockup

### Implementation Details
```cpp
// In transposition_table.cpp - collision case
else {  // Different position (collision)
    if (move == NO_MOVE) {
        // Heuristic entries need significant depth advantage
        if (depth > entry->depth + 3) {
            canReplace = true;
        } else if (entry->generation() < m_generation - 1) {
            canReplace = true;  // Very old, replace
        } else {
            canReplace = false;  // Keep existing
        }
    } else {
        // Regular entries use standard replacement
        if (depth > entry->depth + 2) {
            canReplace = true;
        } else if (entry->generation() != m_generation && depth >= entry->depth) {
            canReplace = true;
        } else if (entry->generation() < m_generation - 1) {
            canReplace = true;  // Prevent lockup
        }
    }
}
```

### Expected Results
- +2 to +4 nELO from better TT utilization
- Reduced TT pollution
- Prevention of TT lockup at high depths

### SPRT Bounds
`[0.00, 5.00]` - Small positive expected

---

## Phase NMR-6: Thread-Safety Preparation (Optional)

### Objective
Add atomic operations for future LazySMP support (infrastructure only)

### Changes
1. Make TTEntry::genBound atomic
2. Use release/acquire memory ordering
3. Add relaxed atomics for stats

### Implementation Details
```cpp
// In transposition_table.h
struct TTEntry {
    std::atomic<uint8_t> genBound;  // Make atomic
    // ...
    
    void save(...) {
        // Write all fields first
        key32 = k;
        move = m;
        score = s;
        evalScore = e;
        depth = d;
        // Release fence ensures visibility
        genBound.store((gen << 2) | bound, std::memory_order_release);
    }
    
    bool isValid() const {
        return genBound.load(std::memory_order_acquire) != 0;
    }
};
```

### Expected Results
- 0 ELO change (infrastructure only)
- Slight NPS decrease (~1-2%) from atomic overhead
- Future-proofs for multi-threading

### SPRT Bounds
`[-3.00, 3.00]` - Verify no regression

---

## Testing Protocol

### For Each Phase:
1. Create feature branch: `git bugfix nmr-phase-X`
2. Implement changes
3. Build and get bench: `./build.sh Release && echo "bench" | ./bin/seajay`
4. Commit with bench: `git commit -m "fix: NMR Phase X - <description>\n\nbench <count>"`
5. Push immediately: `git push -u origin bugfix/YYYYMMDD-nmr-phase-X`
6. Submit to OpenBench with recommended SPRT bounds
7. **FULL STOP** - Wait for test results
8. Only proceed to next phase after PASS

### Failure Recovery
If any phase fails:
1. Create sub-phases (e.g., NMR-2a, NMR-2b)
2. Use binary search to isolate issue
3. Test each sub-component independently
4. Document failure in feature_status.md

## Success Metrics

### Overall Target
- Recover -10 to -15 nELO loss from bugs
- Achieve +5 to +10 nELO net improvement
- Improve stability at deep searches
- Reduce node explosion in endgames

### Phase-by-Phase Expectations
- NMR-1: 0 ELO (infrastructure)
- NMR-2: +5 to +10 nELO (main fix)
- NMR-3: +2 to +5 nELO (refinement)
- NMR-4: +3 to +6 nELO (TT improvement)
- NMR-5: +2 to +4 nELO (TT refinement)
- NMR-6: 0 ELO (future-proofing)

**Total Expected: +12 to +25 nELO**

## Risk Mitigation

### Rollback Points
- After each phase, tag the commit for easy rollback
- Keep main branch stable - only merge after full validation
- Document all changes in feature_status.md

### Critical Validation
- Endgame positions (K+P vs K, etc.)
- Zugzwang positions
- Deep tactical positions
- TT stress tests

## Documentation Requirements

Create and maintain `/workspace/feature_status.md` with:
- Phase completion status
- Bench counts for each phase
- OpenBench test results
- Issues encountered
- Learning points

## Timeline Estimate

Assuming 24-hour OpenBench tests per phase:
- Phase NMR-1: Day 1
- Phase NMR-2: Day 2 (critical phase)
- Phase NMR-3: Day 3
- Phase NMR-4: Day 4
- Phase NMR-5: Day 5
- Phase NMR-6: Day 6 (optional)

**Total: 5-6 days for complete implementation**

## Conclusion

This phased approach ensures:
1. Each change is independently validated
2. Bugs are caught early and isolated
3. Progress is measurable and reversible
4. Main branch remains stable
5. Full OpenBench validation at each step

The primary fix (Phase NMR-2) addresses the root cause of the residual ELO loss. Subsequent phases provide incremental improvements to maximize the gain while maintaining stability.