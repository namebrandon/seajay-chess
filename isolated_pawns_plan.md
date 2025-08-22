# Isolated Pawns Implementation Plan for SeaJay

## Overview
This document outlines the phased implementation of isolated pawn evaluation for SeaJay chess engine. Each phase will be tested via OpenBench before proceeding to the next phase.

## What Are Isolated Pawns?
An isolated pawn is a pawn that has no friendly pawns on either adjacent file. These pawns are considered weak because:
- Cannot be protected by other pawns
- The square in front becomes a weakness
- Opponent can blockade the pawn
- Become targets for enemy pieces

## Critical Requirements
- **STOP after each phase for OpenBench testing**
- **Every commit MUST include "bench <node-count>" in the message**
- **Return branch name, commit hash, and test expectations after each phase**
- **Wait for human confirmation of test results before proceeding**

## Implementation Phases

### Phase IP1: Infrastructure & Detection (0 ELO expected)

#### Objectives:
- Add isolated pawn detection to pawn structure code
- Integrate with existing pawn hash table
- NO evaluation impact yet

#### Implementation Details:
1. Add to `pawn_structure.h`:
   - `isIsolated()` detection function
   - Isolated pawns bitboard in PawnEntry

2. Add to `pawn_structure.cpp`:
   - Detection logic (check adjacent files for friendly pawns)
   - Store results in pawn hash

3. Add unit tests for detection correctness

#### Key Code Structure:
```cpp
// Detection function
bool isIsolated(Square sq, Bitboard ourPawns) {
    int file = fileOf(sq);
    Bitboard adjacentFiles = 0ULL;
    if (file > 0) adjacentFiles |= FILE_A_BB << (file - 1);
    if (file < 7) adjacentFiles |= FILE_A_BB << (file + 1);
    return !(ourPawns & adjacentFiles);
}
```

#### Test Positions for Validation:
```
4k3/8/8/3p4/8/8/8/4K3 w - - 0 1  // Black d5 pawn is isolated
4k3/p1p3p1/8/8/8/8/P1P3P1/4K3 w - - 0 1  // All b and f pawns isolated
```

#### Commit Process:
```bash
echo "bench" | ./bin/seajay | grep "Benchmark complete"
git add -A
git commit -m "feat: Add isolated pawn detection infrastructure (Phase IP1)

No evaluation changes yet, just detection logic.

bench <node-count>"
git push
```

#### OpenBench Test Configuration:
- **Branch:** feature/20250822-isolated-pawns
- **Base:** main
- **Expected Result:** 0 ELO ± 10 (no regression)
- **Time Control:** 10+0.1
- **Test Type:** SPRT [-10, 10]

---

### Phase IP2: Basic Integration (+10-15 ELO expected)

#### Objectives:
- Apply basic penalties for isolated pawns
- Implement phase scaling
- Cache in pawn hash

#### Implementation Details:
1. Add to `evaluate.cpp`:
   - Basic penalty: -12 centipawns base (more conservative per expert review)
   - Rank-based scaling (weaker pawns on lower ranks)
   - Phase scaling (less penalty in endgame)

2. Penalty structure:
```cpp
// Base penalties by rank (white perspective)
// More conservative values based on engine analysis
static constexpr int ISOLATED_PAWN_PENALTY[8] = {
    0,   // Rank 1 - no pawns here
    15,  // Rank 2 - most vulnerable (reduced from 20)
    14,  // Rank 3
    12,  // Rank 4 - standard
    12,  // Rank 5
    10,  // Rank 6
    8,   // Rank 7 - less weak when advanced
    0    // Rank 8 - promoted
};
```

3. Phase scaling:
   - Opening: 100% penalty
   - Middlegame: 100% penalty  
   - Endgame: 50% penalty (isolated pawns less weak)
   
4. Store in pawn hash:
   - Cache both detection AND evaluation score

#### Commit Process:
```bash
echo "bench" | ./bin/seajay | grep "Benchmark complete"
git add -A
git commit -m "feat: Add basic isolated pawn penalties (Phase IP2)

Implements rank-based penalties with phase scaling.
Base penalty ~15cp, reduced in endgame.

bench <node-count>"
git push
```

#### OpenBench Test Configuration:
- **Branch:** feature/20250822-isolated-pawns
- **Base:** Previous IP1 commit
- **Expected Result:** +8-12 ELO (revised based on expert analysis)
- **Time Control:** 10+0.1
- **Test Type:** SPRT [5, 15]

---

### Phase IP3: Refinements (+3-7 ELO expected)

#### Objectives:
- Add file-based adjustments
- Consider if isolani is opposed (enemy pawn on same file)
- Fine-tune penalties

#### Implementation Details:
1. File considerations (based on expert engine analysis):
```cpp
static constexpr int FILE_ADJUSTMENT[8] = {
    120,  // a-file (edge pawn penalty)
    105,  // b-file
    100,  // c-file (standard)
    80,   // d-file (central bonus)
    80,   // e-file (central bonus)
    100,  // f-file (standard)
    105,  // g-file
    120   // h-file (edge pawn penalty)
};
```

2. Additional factors:
   - If opposed (enemy pawn on same file): +20% penalty
   - If blockaded by enemy piece: +15% penalty
   - Avoid double-counting with backward pawns

3. Make penalties tunable via UCI options for future SPSA

#### UCI Options to Add:
```cpp
option name IsolatedPawnPenalty type spin default 15 min 5 max 30
option name IsolatedPawnEndgameScale type spin default 50 min 25 max 100
```

#### Commit Process:
```bash
echo "bench" | ./bin/seajay | grep "Benchmark complete"
git add -A
git commit -m "feat: Refine isolated pawn evaluation (Phase IP3)

Adds file-based and blockade adjustments.
Central isolanis get reduced penalty.

bench <node-count>"
git push
```

#### OpenBench Test Configuration:
- **Branch:** feature/20250822-isolated-pawns
- **Base:** Previous IP2 commit
- **Expected Result:** +3-7 ELO (revised based on expert analysis)
- **Time Control:** 60+0.6
- **Test Type:** SPRT [0, 10]

---

## Risk Mitigation Checklist

### Before Each Commit:
- [ ] Run bench command and record node count
- [ ] Include "bench <node-count>" in commit message
- [ ] Verify no compilation warnings
- [ ] Test specific positions for correctness
- [ ] Push to feature branch

### Common Bugs to Check:
- [ ] File boundary checks (a and h files) - CRITICAL!
- [ ] Correct phase scaling (less penalty in endgame, not more)
- [ ] No double-counting in pawn hash
- [ ] No double-counting with backward pawns
- [ ] Proper integration with existing pawn structure code
- [ ] Cache both detection AND evaluation in pawn hash
- [ ] Opposed pawn detection (enemy pawn on same file)

## Testing Protocol

### After Each Phase:
1. **Stop development**
2. **Report to human**:
   - Branch name
   - Commit hash
   - Expected ELO change
3. **Wait for OpenBench results**
4. **Only proceed after confirmation**

## Success Criteria

### Phase Validation:
- IP1: No regression (within ± 10 ELO)
- IP2: Main gain (+10-15 ELO)
- IP3: Incremental improvement (+5-10 ELO)

### Final Merge Criteria:
- All phases tested via OpenBench
- Total ELO gain: +11-19 from baseline (revised expectation)
- No regression in other positions
- Clean compilation
- Integrates with pawn hash table

## Key Implementation Insights

1. **Why Isolated Pawns Are Weak:**
   - Cannot be defended by pawns
   - Square in front becomes outpost for enemy
   - Easily blockaded
   - Require piece defense

2. **Why Less Penalty in Endgame:**
   - Fewer pieces to attack them
   - King can defend them
   - Pawn breaks more feasible
   - Less time for exploitation

3. **Central vs Wing Isolanis:**
   - Central control more important
   - d4/e4 pawns control key squares
   - Wing pawns less influential

## Expert Insights from Other Engines

Based on analysis of Stockfish, Ethereal, Obsidian, Berserk, and Stash-bot:

1. **Start Conservative:** Most engines use 10-15cp base penalty
2. **File Matters:** Edge pawns (a/h) should get extra penalty (120%), central (d/e) reduced (80%)
3. **Opposed Isolanis:** If enemy pawn on same file, +20% weaker
4. **Pawn Hash Critical:** Cache both detection AND evaluation
5. **Avoid Double-Counting:** Don't also count as backward pawns
6. **Simple Works:** Stash-bot reaches 2800+ Elo with just 10cp flat penalty

## Critical Implementation Notes

**File Boundary Bug Prevention:**
```cpp
// ALWAYS check bounds before file operations
if (file > 0) adjacentFiles |= FILE_A_BB << (file - 1);
if (file < 7) adjacentFiles |= FILE_A_BB << (file + 1);
```

Total expected ELO gain: +11-19 points (revised from 15-25)
Implementation time: 2-3 days (including testing)
Risk level: Low (simple feature)