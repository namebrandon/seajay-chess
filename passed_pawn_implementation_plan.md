# Passed Pawn Evaluation Implementation Plan for SeaJay

## Overview
This document outlines the phased implementation of passed pawn evaluation for SeaJay chess engine. Each phase will be tested via OpenBench before proceeding to the next phase.

## Critical Requirements
- **STOP after each phase for OpenBench testing**
- **Every commit MUST include "bench <node-count>" in the message**
- **Return branch name, commit hash, and test expectations after each phase**
- **Wait for human confirmation of test results before proceeding**

## Implementation Phases

### Phase PP1: Infrastructure & Detection (0 ELO expected)

#### Objectives:
- Create pawn structure evaluation framework
- Implement passed pawn detection using bitboards
- Set up pawn hash caching infrastructure
- NO integration with evaluation yet

#### Implementation Details:
1. Create `src/evaluation/pawn_structure.h` and `.cpp` files
2. Implement:
   - `relativeRank()` helper function to avoid rank confusion
   - Passed pawn mask generation at program startup
   - `isPassed()` detection function
   - Pawn hash structure for caching results
3. Add unit tests for detection correctness

#### Key Code Structure:
```cpp
// Precomputed masks for performance
Bitboard passedPawnMask[2][64];  // [Color][Square]

// Detection function
bool isPassed(Color us, Square sq, Bitboard theirPawns) {
    return !(theirPawns & passedPawnMask[us][sq]);
}
```

#### Test Positions for Validation:
```
8/8/1p6/1P6/8/8/8/8 w - - 0 1  // Both sides have passer
8/pp6/8/PP6/8/8/8/8 w - - 0 1  // Connected vs isolated
8/k7/P7/8/8/8/8/K7 w - - 0 1  // Advanced passer
```

#### Commit Process:
```bash
# After implementation
echo "bench" | ./bin/seajay | grep "Benchmark complete"
git add -A
git commit -m "feat: Add passed pawn detection infrastructure (Phase PP1) - bench <node-count>"
git push
```

#### OpenBench Test Configuration:
- **Branch:** feature/20250821-passed-pawns
- **Base:** main
- **Expected Result:** 0 ELO ± 10 (no regression)
- **UCI Options:** None
- **Time Control:** 10+0.1
- **Test Type:** SPRT [0, 10]

#### Deliverables for Human Review:
- Branch name: `feature/20250821-passed-pawns`
- Commit hash: (will be provided after implementation)
- Expected ELO: 0 ± 10
- Status: AWAITING OPENBENCH TEST

---

### Phase PP1.5: Candidate Passed Pawns (5-10 ELO expected)

#### Objectives:
- Detect pawns that can become passed with one push
- Apply 30-50% of passed pawn value
- Cache in pawn hash structure

#### Implementation Details:
1. Add candidate passer detection
2. Store candidate passers in pawn hash
3. Still NO evaluation integration

#### Commit Process:
```bash
echo "bench" | ./bin/seajay | grep "Benchmark complete"
git add -A
git commit -m "feat: Add candidate passed pawn detection (Phase PP1.5) - bench <node-count>"
git push
```

#### OpenBench Test Configuration:
- **Branch:** feature/20250821-passed-pawns
- **Base:** Previous PP1 commit
- **Expected Result:** 0-5 ELO (slight improvement or neutral)
- **UCI Options:** None
- **Time Control:** 10+0.1
- **Test Type:** SPRT [0, 10]

#### Deliverables for Human Review:
- Branch name: `feature/20250821-passed-pawns`
- Commit hash: (will be provided)
- Expected ELO: 0-5
- Status: AWAITING OPENBENCH TEST

---

### Phase PP2: Basic Integration (0 to -5 ELO expected)

#### Objectives:
- Hook passed pawn detection into evaluation system
- Apply basic rank-based bonuses
- Implement phase scaling
- May show regression due to overhead

#### Implementation Details:
1. Integrate with `evaluate.cpp`
2. Apply rank bonuses: `{0, 10, 17, 30, 60, 120, 180, 0}`
3. Phase scaling: more valuable in endgame
4. Ensure no double-counting with pawn hash

#### Critical Checks:
- Verify phase scaling sign (endgame should INCREASE value)
- Check pawn hash integration doesn't double-count
- Validate rank indexing is relative to pawn color

#### Commit Process:
```bash
echo "bench" | ./bin/seajay | grep "Benchmark complete"
git add -A
git commit -m "feat: Integrate passed pawns with basic bonuses (Phase PP2) - bench <node-count>"
git push
```

#### OpenBench Test Configuration:
- **Branch:** feature/20250821-passed-pawns
- **Base:** Previous PP1.5 commit
- **Expected Result:** -5 to +5 ELO (overhead may cause small regression)
- **UCI Options:** None
- **Time Control:** 10+0.1
- **Test Type:** SPRT [-10, 10]

#### Deliverables for Human Review:
- Branch name: `feature/20250821-passed-pawns`
- Commit hash: (will be provided)
- Expected ELO: -5 to +5
- Status: AWAITING OPENBENCH TEST

---

### Phase PP3: Full Implementation (+40-60 ELO expected)

#### Objectives:
- Add all major passed pawn evaluation features
- This is where the main ELO gain is realized

#### Implementation Details:
1. King proximity factors (friendly and enemy)
2. Rook behind passed pawn (both sides)
3. Blockader evaluation:
   - Knight: smallest penalty
   - Rook: medium penalty  
   - Queen: medium penalty
   - Bishop: largest penalty
4. Protected passed pawns (+30% bonus)
5. Connected passed pawns (+50% bonus)
6. Unstoppable passer detection (huge bonus)

#### Test Suite Validation:
- Run STS (Strategic Test Suite) passed pawn section
- Validate against Arasan test positions
- Check specific endgame positions

#### Commit Process:
```bash
echo "bench" | ./bin/seajay | grep "Benchmark complete"
git add -A
git commit -m "feat: Complete passed pawn evaluation with all factors (Phase PP3) - bench <node-count>"
git push
```

#### OpenBench Test Configuration:
- **Branch:** feature/20250821-passed-pawns
- **Base:** Previous PP2 commit
- **Expected Result:** +40-60 ELO
- **UCI Options:** None
- **Time Control:** 60+0.6 (longer TC for accuracy)
- **Test Type:** SPRT [30, 70]

#### Deliverables for Human Review:
- Branch name: `feature/20250821-passed-pawns`
- Commit hash: (will be provided)
- Expected ELO: +40-60
- Status: AWAITING OPENBENCH TEST

---

### Phase PP4: Tuning & Refinement (+5-10 ELO expected)

#### Objectives:
- SPSA tuning of all parameters
- Fine-tune bonuses for optimal strength

#### Implementation Details:
1. Make all bonuses UCI-configurable for SPSA
2. Tune:
   - Rank bonus values
   - Phase scaling factors
   - King distance multipliers
   - Connected/protected bonuses
   - Blockader penalties
3. Add outside passed pawn logic
4. Optimize masks if beneficial

#### UCI Options to Add:
```cpp
option name PassedRank2 type spin default 10 min 5 max 30
option name PassedRank3 type spin default 17 min 10 max 40
option name PassedRank4 type spin default 30 min 20 max 60
option name PassedRank5 type spin default 60 min 40 max 100
option name PassedRank6 type spin default 120 min 80 max 180
option name PassedRank7 type spin default 180 min 120 max 250
```

#### Commit Process:
```bash
echo "bench" | ./bin/seajay | grep "Benchmark complete"
git add -A
git commit -m "feat: Add SPSA tuning for passed pawn parameters (Phase PP4) - bench <node-count>"
git push
```

#### OpenBench Test Configuration:
- **Branch:** feature/20250821-passed-pawns
- **Base:** Previous PP3 commit
- **Expected Result:** +5-10 ELO
- **UCI Options:** Default values (pre-tuning)
- **Time Control:** 60+0.6
- **Test Type:** SPRT [0, 15]
- **Note:** After SPSA tuning completes, update defaults and retest

#### Deliverables for Human Review:
- Branch name: `feature/20250821-passed-pawns`
- Commit hash: (will be provided)
- Expected ELO: +5-10
- Status: AWAITING OPENBENCH TEST

---

## Risk Mitigation Checklist

### Before Each Commit:
- [ ] Run bench command and record node count
- [ ] Include "bench <node-count>" in commit message
- [ ] Verify no compilation warnings
- [ ] Test specific positions for correctness
- [ ] Push to feature branch

### Common Bugs to Check:
- [ ] Rank indexing uses relative rank to pawn color
- [ ] Bitboard operations handle A/H file edges correctly
- [ ] Phase scaling makes passed pawns MORE valuable in endgame
- [ ] No double-counting in pawn hash
- [ ] Blockader penalties have correct sign

## Testing Protocol

### After Each Phase:
1. **Stop development** - Do not proceed to next phase
2. **Report to human**:
   - Branch name
   - Commit hash (short form)
   - Expected ELO change
   - UCI options needed (if any)
3. **Wait for OpenBench results**
4. **Only proceed after human confirmation**

### OpenBench Submission Template:
```
Dev Branch: feature/20250821-passed-pawns
Base Branch: main (or previous phase commit)
Time Control: 10+0.1 (or 60+0.6 for later phases)
Test Bounds: [lower, upper] based on phase expectations
Book: UHO_4060_v2.epd
Description: Phase PPX - <brief description>
```

## Success Criteria

### Phase Validation:
- PP1: No regression (within ± 10 ELO)
- PP1.5: Neutral to slight positive (0-5 ELO)
- PP2: Small regression acceptable (-5 to +5 ELO)
- PP3: Significant gain required (+40-60 ELO)
- PP4: Incremental improvement (+5-10 ELO)

### Final Merge Criteria:
- All phases tested via OpenBench
- Total ELO gain: +50-75 from baseline
- No significant regression in tactical positions
- Clean compilation, no warnings
- Perft tests still pass

## Communication Protocol

After implementing each phase, the AI assistant will provide:

```
=== PHASE PP[X] COMPLETE ===
Branch: feature/20250821-passed-pawns
Commit: [short hash]
Bench: [node count]
Expected ELO: [range]
UCI Options: [if any]
Ready for OpenBench: YES

AWAITING HUMAN TEST CONFIRMATION
Do not proceed to Phase PP[X+1] until test results are reviewed.
```

## Final Notes

This implementation follows the successful phased approach used for killer moves and history heuristic in SeaJay. The key is patience - waiting for test results between phases ensures we catch issues early and understand the impact of each change.

Total expected ELO gain: +50-75 points
Total implementation time: 4-5 days (including testing waits)
Risk level: Low (with phased approach)