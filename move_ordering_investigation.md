# Move Ordering Efficiency Investigation

**Created:** August 27, 2025  
**Branch:** feature/analysis/20250826-pruning-aggressiveness  
**Status:** INVESTIGATION IN PROGRESS  
**Parent Analysis:** master_plan.md  

## Executive Summary

SeaJay's move ordering efficiency is currently at **76.8%**, significantly below the target of **85%+** seen in competitive engines. This means that beta cutoffs occur on the first move only 76.8% of the time, indicating that the best moves are often not being searched first. This inefficiency causes:

- **20-30% more nodes searched than necessary**
- **1-2 ply shallower searches** at the same time control
- **Reduced effectiveness** of pruning techniques like LMR

## Current Statistics

From the original game analysis (seajay_investigation_report.md):
```
Move Ordering Efficiency: 76.8%
Beta Cutoffs on First Move: 153,946 / 200,512 = 76.8%
```

Target for competitive engines: **85-90%** efficiency

## Investigation Goals

1. **Identify** why move ordering is underperforming
2. **Compare** with successful implementations (Laser, Stockfish)
3. **Quantify** the impact of each ordering component
4. **Create** actionable improvement plan with expected gains

## Current Move Ordering Implementation

### Components Currently Implemented:

1. **Transposition Table Move** (Stage 12)
   - Should be tried first when available
   - Status: ✅ Implemented

2. **MVV-LVA for Captures** (Basic)
   - Most Valuable Victim - Least Valuable Attacker
   - Status: ✅ Implemented

3. **Killer Moves** (Stage 19)
   - Two killer moves per ply
   - Status: ✅ Implemented (+31.42 Elo when added)

4. **History Heuristic** (Stage 20)
   - Butterfly table tracking successful moves
   - Status: ✅ Implemented (initial implementation had issues)

5. **Countermove Heuristic** (Stage 23)
   - Response to previous move
   - Status: ✅ Implemented (+8.31 nELO)

### Order of Move Selection (Expected):
1. TT move (if available)
2. Good captures (MVV-LVA positive SEE)
3. Killer moves (2 per ply)
4. Countermove
5. Quiet moves ordered by history
6. Bad captures (negative SEE)

## Initial Analysis

### Hypothesis H1: TT Move Not Being Tried First
**Theory:** The TT move might not always be searched first
**Test:** Add statistics to track when TT move causes cutoff
**Expected:** TT move should cause cutoff 40-50% of the time when present

### Hypothesis H2: Killer Moves Not Updated Correctly
**Theory:** Killer moves might not be properly maintained/updated
**Test:** Track killer move hit rate and update frequency
**Expected:** Killer moves should cause cutoff 15-20% of the time

### Hypothesis H3: History Heuristic Decay Issues
**Theory:** History scores might grow unbounded or decay incorrectly
**Test:** Monitor history score distribution over time
**Expected:** Scores should remain in reasonable range with proper aging

### Hypothesis H4: Move Ordering Function Incorrect
**Theory:** The actual sorting/ordering logic might have bugs
**Test:** Log move order for positions and verify correct sequencing
**Expected:** Moves should follow the prescribed order

### Hypothesis H5: SEE Implementation Issues
**Theory:** Static Exchange Evaluation might mis-order captures
**Test:** Verify SEE scores for captures match expected values
**Expected:** Winning captures before losing captures

## Code Investigation - FINDINGS

### Current Implementation Analysis:

#### 1. Move Ordering Sequence (negamax.cpp):
```cpp
1. MVV-LVA ordering for captures (move_ordering.cpp)
2. Killer moves placed after captures (orderMovesWithHistory)
3. Countermove placed after killers (if bonus > 0)
4. Quiet moves sorted by history score
5. TT move moved to front AFTER all other ordering
```

#### 2. Key Observations:

**✅ TT Move Handling:** 
- TT move IS moved to first position after ordering (line 87-97 negamax.cpp)
- This happens correctly as the last step

**⚠️ Potential Issue #1: Killer Move Placement**
- Killers are placed at the START of quiet moves
- But they come AFTER all captures (even bad captures!)
- In strong engines, killers often come before bad captures

**⚠️ Potential Issue #2: No SEE for Capture Ordering**
- All captures ordered by MVV-LVA only
- No Static Exchange Evaluation to put bad captures last
- This means QxP defended by pawn is searched before killer moves!

**⚠️ Potential Issue #3: History Decay**
- No obvious history aging/decay mechanism found
- History scores could grow unbounded over time
- This would make history ordering less effective

**❌ CRITICAL ISSUE FOUND: No SEE Being Used!**
- SEE (Static Exchange Evaluation) code EXISTS but is NOT being used
- The `SEEMoveOrdering` class is implemented but never called
- All captures are ordered by MVV-LVA only
- This means losing captures (QxP defended by pawn) are searched before killer moves!

**✅ History Aging:** 
- History DOES have proper aging (divides by 2 when values get too high)
- Only ages when >10% of entries are high (prevents premature aging)
- This is working correctly

### Statistics to Add:
```cpp
struct MoveOrderingStats {
    uint64_t ttMoveFirst = 0;      // TT move searched first
    uint64_t ttMoveCutoff = 0;     // TT move caused cutoff
    uint64_t killerFirst = 0;      // Killer searched first (no TT)
    uint64_t killerCutoff = 0;     // Killer caused cutoff
    uint64_t historyFirst = 0;     // History best searched first
    uint64_t historyCutoff = 0;    // History best caused cutoff
    uint64_t randomFirst = 0;      // No ordering info available
};
```

## Comparison with Laser

### Laser's Move Ordering:
1. **TT move** (always first if valid)
2. **Good captures** (SEE >= 0, ordered by MVV-LVA)
3. **Killer moves** (2 per ply, verified pseudo-legal)
4. **Countermove** (if not already killer)
5. **Quiet moves** (ordered by history score)
6. **Bad captures** (SEE < 0, ordered by SEE value)

### Key Differences to Investigate:
- Laser updates killers only on quiet moves causing beta cutoff
- Laser uses history aging every 2048 entries
- Laser has separate capture history table
- Laser uses continuation history (2-ply history)

## Root Cause Analysis

### PRIMARY CAUSE: No SEE for Capture Ordering (Est. -8% efficiency)
- **Issue:** All captures ordered by MVV-LVA regardless of tactical soundness
- **Impact:** Bad captures (like QxP defended) searched before killer moves
- **Example:** In a position with QxP defended and a killer move, the losing capture is searched first
- **Fix:** Use SEE to put bad captures (SEE < 0) after killer moves

### SECONDARY CAUSE: Poor Move Type Ordering (Est. -5% efficiency)
Current order:
1. TT move (good ✅)
2. ALL captures by MVV-LVA (bad ❌)
3. Killer moves (too late!)
4. Countermove
5. Quiet moves by history

Should be:
1. TT move
2. **Good captures (SEE ≥ 0)** by MVV-LVA
3. Killer moves
4. Countermove  
5. Quiet moves by history
6. **Bad captures (SEE < 0)** by SEE value

### MINOR ISSUES:
- No capture history table (tracks which captures work)
- No continuation history (2-ply patterns)
- Basic history table only (no piece-specific history)

## Testing Methodology

### Phase 1: Instrumentation
Add detailed statistics collection to understand current behavior:
- Track cutoff rates by move type
- Monitor ordering decisions
- Log anomalies

### Phase 2: Component Testing
Test each ordering component in isolation:
- Disable all but TT move ordering → measure impact
- Add back killer moves → measure improvement
- Add back history → measure improvement
- Enable all → should sum to 76.8%

### Phase 3: Fix Implementation
Based on findings, fix the most impactful issues:
- Correct any ordering bugs
- Tune parameters (killer slots, history aging)
- Add missing features (capture history)

### Phase 4: Validation
- Measure new efficiency percentage
- Run SPRT tests for Elo gain
- Verify no tactical regressions

## Expected Gains

Based on move ordering importance:
- **Fixing to 85% efficiency:** +20-30 Elo
- **Reaching 90% efficiency:** +40-50 Elo
- **Depth improvement:** +1-2 ply average
- **Node reduction:** 20-30% fewer nodes

## Implementation Plan

### Phase 1: Enable SEE for Capture Ordering [HIGHEST PRIORITY]
**Expected Gain:** +8-10% efficiency, +15-20 Elo

#### Step 1.1: Activate SEE in Move Ordering
- Change from MVV-LVA to SEE-based ordering
- Use the existing `SEEMoveOrdering` class
- Put bad captures (SEE < 0) after killers

#### Step 1.2: Proper Move Type Ordering
```cpp
1. TT move (if valid)
2. Good captures (SEE >= 0) ordered by MVV-LVA
3. Killer moves (2 slots)
4. Countermove (if different from killers)
5. Quiet moves (ordered by history)
6. Bad captures (SEE < 0) ordered by SEE value
```

#### Step 1.3: Testing & Validation
- Add statistics to track improvement
- Run SPRT test for Elo gain
- Target: 85%+ move ordering efficiency

### Phase 2: Add Capture History [MEDIUM PRIORITY]
**Expected Gain:** +2-3% efficiency, +5-10 Elo

- Track success of specific captures
- Order good captures by history within MVV-LVA groups
- Separate from quiet move history

### Phase 3: Continuation History [LOWER PRIORITY]
**Expected Gain:** +1-2% efficiency, +5 Elo

- Track 2-ply move patterns
- Improve quiet move ordering
- Requires more memory but better patterns

## Next Steps

1. **Immediate:** Add instrumentation to collect detailed statistics
2. **Today:** Run test games with statistics enabled
3. **Tomorrow:** Analyze results and identify biggest issue
4. **This Week:** Implement fixes for top issues
5. **Next Week:** SPRT testing of improvements

## Success Criteria

- [ ] Move ordering efficiency reaches 85%+
- [ ] Beta cutoff on first move in 85%+ of positions
- [ ] No regression in tactical strength
- [ ] Measurable Elo gain (+20-30 expected)
- [ ] 20-30% reduction in nodes searched

## Risk Factors

1. **Complex interactions:** Fixing one component might affect others
2. **Tuning required:** Optimal parameters might differ from other engines
3. **Testing time:** Each change needs SPRT validation

## References

- Laser source: https://github.com/jeffreyan11/laser-chess-engine
- Stockfish source: https://github.com/official-stockfish/Stockfish
- Chess Programming Wiki: https://www.chessprogramming.org/Move_Ordering

---

**Document Status:** Initial investigation framework created. Next step: Add instrumentation to code.