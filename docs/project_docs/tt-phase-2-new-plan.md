# Phase 2 – Ranked MovePicker: Revised Plan (Lessons‑Learned + Granular Steps)

## Executive Summary
We redesign Phase 2 around a ranked MovePicker with strict, incremental steps and a test‑after‑each‑change workflow. The goal is to improve early decision quality without destabilizing search. Each sub‑phase is a single functional change gated by SPRT against the Phase 1 baseline.

## Design Principles
- Single‑pass O(n), no quadratic work or repeated sorts
- No dynamic allocations or hidden copies; fixed‑size stack arrays
- In‑check parity: generate check evasions when in check (never iterate non‑evasions)
- Clean fallback: feature behind UCI toggle; legacy path intact
- Root and QS safety in 2a: keep ranked picker off at root and in quiescence
- Deterministic ordering with clear tie‑breaks

## Critical Lessons Learned (Do/Don’t)

Do
- Use check‑evasion generator when in check.
- Include non‑capture promotions and top quiets in the shortlist.
- MVV‑LVA order captures; don’t return captures in generator order.
- After shortlist, hand off the remainder to the legacy orderer (captures + killers/history quiets).
- Dedup TT move by exact encoding (including promotion piece).
- Add bound checks/asserts around fixed arrays (shortlist, QS arrays) in non‑Release.
 - Preserve captures‑first discipline: when mixing classes in the shortlist, ensure captures/promotions are yielded before quiets (or build the shortlist directly from the legacy‑ordered capture/promotion prefix).
 - Consider depth‑gating quiet shortlist (e.g., enable quiets only at depth ≥ 6 to align with CMH gating) to avoid shallow‑depth noise.
 - Keep quiet scoring weights conservative so they don’t swamp capture scores (history≈×1, killer≈300–500, countermove≈200–300, CMH scaled down).
 - For parity: it’s safe and effective to take the shortlist directly from the already legacy‑ordered list (TT=NO_MOVE) so early yields don’t change relative ordering.

Don’t
- Don’t enable ranked picker in QS or at root in 2a.
- Don’t rely on legal move generation for ranking (avoid tryMakeMove in ordering).
- Don’t sort full lists or re‑sort per next(); rank once per node.
- Don’t bury promotions or good quiets; don’t skip evasions when in check.
 - Don’t allow quiets to outrank strong captures at shallow depth; avoid large static bonuses that destabilize early move order.

## Core Components

### 1. RankedMovePicker Class Design

#### Class Structure
```cpp
class RankedMovePicker {
    // No owning pointers, trivially copyable
    // References to existing tables only
    // Stack-based storage for shortlist
};
```

#### Key Design Decisions
- **No RAII/copy issues**: Explicitly delete copy/move or make trivially copyable
- **No ownership**: Only references/raw pointers to existing tables
- **Iterator-like API**: `next()` method for move yielding
- **Pre-rank once**: Compute shortlist during construction for Phase 2a simplicity

### 2. Scoring System

#### Capture Scoring (Lazy SEE)
```
1. Pre-score with MVV-LVA (cheap)
2. Compute SEE only when:
   - Comparing for shortlist boundary
   - MVV-LVA suggests it's "good" (victim_value >= attacker_value)
3. Add promotion bonus (queen > rook > bishop/knight)
4. Small check bonus (+50)
```

#### Quiet Move Scoring
```
score = history_score * 2 +
        killer_bonus * 1000 +
        countermove_bonus * 500 +
        refutation_bonus * 300 +
        check_bonus * 50
```
- Use int16_t for scores (sufficient range)
- Weights are initial estimates, not tuned

#### Tie-Breaking
For deterministic ordering:
1. Score (primary)
2. MVV-LVA value (secondary)
3. Raw move encoding (tertiary)

### 3. Top‑K Shortlist Mechanics

#### Implementation
- **Fixed size**: K=10 initially (stack array)
- **Simple insertion**: Track minimum score in shortlist
- **No heap/vector**: Direct array manipulation
- **Deduplication**: Check against TT move during insertion
 - **Class priority (stability)**: When shortlist mixes classes, yield order caps/promotions → quiets (or build shortlist from the legacy‑ordered capture/promotion prefix, then append quiets).
 - **Depth gating (quiets)**: Admit quiets in shortlist only from depth ≥ 6 to match CMH usefulness.
 - **Parity‑preserving option**: To avoid drift, derive shortlist from the already legacy‑ordered list (call orderMoves with ttMove=NO_MOVE) and take the first K of the relevant class.

#### Future Enhancement (not in 2a)
- Depth-aware K: `K = min(12, 8 + depth/2)`

### 4. Move Yield Order (2a target behavior)

#### Main Search (negamax)
1. TT move (if present)
2. Top‑K shortlist (captures + quiets + promotions; sorted)
3. Remainder via legacy orderer:
   - Captures: MVV‑LVA ordered
   - Quiets: killers → history (existing path)

#### Quiescence Search (2a)
- Ranked picker disabled; legacy captures/promotions only (no quiets)

### 5. TT Move Handling

#### Validation Protocol
```cpp
if (ttMove != MOVE_NONE) {
    if (isLegal(ttMove)) {
        // Yield first, mark as used
        ttMoveUsed = true;
    } else {
        // Ignore illegal TT move
        ttMove = MOVE_NONE;
    }
}
```

#### Deduplication
- Full move encoding comparison (including promotion piece)
- Skip TT move if already in shortlist

## Integration Points

### Negamax Integration
```cpp
if (UseRankedMovePicker && ply > 0) {  // Skip root for safety
    RankedMovePicker picker(...);
    while (Move move = picker.next()) {
        // existing search logic
    }
} else {
    // Legacy orderMoves() path
}
```

### Quiescence Integration
- 2a: keep legacy path only; do not enable ranked QS

### UCI Toggle
- `UseRankedMovePicker` (default: false)
- No runtime behavior change until wired
- Keep `UseRankedMovePickerAtRoot` internal or implicit (ply > 0 check)

## Telemetry (Low Overhead)

### Metrics to Track
1. **Best-move rank distribution**
   - Where does eventual PV move appear in ordering?
   - Track via histogram: rank_1, rank_2_5, rank_6_10, rank_11+

2. **First-move cutoff rate**
   - Already tracked in existing stats
   - Compare before/after

3. **Shortlist coverage**
   - Fraction of quiet PV moves in top-K
   - Helps tune K value

4. **SEE call efficiency**
   - Count SEE evaluations vs total captures
   - Verify lazy SEE is working

### Implementation
```cpp
#ifdef SEARCH_STATS
    struct MovePickerStats {
        uint64_t bestMoveRank[4];  // [1], [2-5], [6-10], [11+]
        uint64_t shortlistHits;
        uint64_t seeCallsLazy;
        uint64_t capturesTotal;
    };
#endif
```

## Risk Mitigation

### Performance Risks
- **SEE overhead**: Mitigated by lazy evaluation
- **Shortlist overhead**: Fixed-size, O(n) single pass
- **Cache misses**: Keep data structures compact

### Correctness Risks
- **TT move illegality**: Explicit validation
- **Move deduplication**: Full encoding comparison
- **Promotion handling**: Include piece in all comparisons
 - **Quiet shortlist instability**: If quiets cause regression, disable quiets and/or enforce class priority and depth gating; reduce quiet weights to prevent overshadowing capture scores.

### Regression Risks
- Root ordering: keep legacy at ply 0 for 2a
- Qsearch: ranked picker off; keep captures‑only ordering
- Bench deviation: acceptable within noise; document

### UB/LTO Risks (OpenBench)
- Fixed arrays: hard bound checks in debug; never write past end
- Consistent ifdefs across TUs for stats/debug to avoid ODR issues under LTO
- Avoid relying on undefined iteration over uninitialized slots

## Testing Protocol

### Build Discipline
```bash
# Clean rebuild for each test
make clean
rm -rf build
./build.sh Release
```

### Validation Steps
1. **Unit tests**: All must pass
2. **Perft**: Unchanged (move gen correctness)
3. **Bench**: Document any change (ordering affects nodes)
4. **Tactical suite**: ≥ baseline pass rate
5. **Depth comparison**: Similar depth at fixed time

### A/B Testing
```
Branch: feature/20250908-ranked-movepicker-2a
Base: current HEAD (Phase 1 complete)
Time Control: 10+0.1
Bounds: [-3.00, 3.00] (non-regression)
Book: UHO_4060_v2.epd
```

## Implementation Checklist

### Phase 2a Core Tasks (granular, one change per SPRT)
Use the companion checklist before each sub‑phase: `docs/project_docs/scaffolds/Phase2a_Checklist.md`.
- [ ] 2a.0: Scaffolds only (UCI toggle, no behavior change); verify build parity
- [ ] 2a.1: TT‑only sanity (yield TT first; no other change)
- [ ] 2a.2: Captures‑only shortlist (K small), remainder legacy; SPRT
- [ ] 2a.3: Add quiets + promotions to shortlist; remainder legacy; SPRT
- [ ] 2a.4: In‑check parity (use check evasions in picker ctor); SPRT
- [ ] 2a.5: Deterministic tie‑breaks; assert/guards around fixed arrays; SPRT
- [ ] 2a.6: Minimal telemetry hooks (compiled‑out in Release); SPRT
- [ ] 2a.7: QS remains legacy; confirm no ranked QS paths via asserts/log once; SPRT
- [ ] 2a.8: Evasion ordering (class‑priority): captures of checker → blocks → king moves; stable, no history/CMH; SPRT

For each sub‑phase:
- Clean rebuild; bench in commit; short local sanity; then OpenBench SPRT vs Phase 1 end.

### Success Criteria
- Non‑regression SPRT vs Phase 1 end at 10+0.1 (bounds [-3.00, 3.00])
- First‑move cutoff and PV stability not worse than baseline
- Bench within expected variance; no unexplained perf cliffs
- No sanitizer or bounds violations in Makefile/OB build

## Notes for Implementation

### Code Organization
```
src/search/
  ranked_move_picker.h       # Main class
  ranked_move_picker.cpp     # Implementation
  ranked_move_picker_qs.h    # Quiescence variant
  ranked_move_picker_qs.cpp  # QS implementation
```

### Memory Layout
```cpp
struct RankedMovePicker {
    // Fixed arrays on stack
    Move shortlist[10];
    int16_t scores[10];
    int shortlistSize;
    
    // References to tables
    const Board& board;
    const HistoryHeuristic& history;
    const KillerMoves& killers;
    // ...
};
```

### Performance Notes
- Inline hot functions (scoring)
- Minimize branches in inner loops
- Use __builtin_expect for likely paths
- Consider prefetch for history table access

## OpenBench Build Parity & Sanitizers
- OpenBench uses Makefile with -O3 -flto -march=native. Reproduce locally for parity.
- Add ASan/UBSan CI target (disable LTO) for quick overflow/UB detection.
- If OB shows regressions not seen locally, run the Makefile build locally and check with minimal logging.

## Conclusion
Phase 2 now proceeds in granular sub‑phases with an SPRT after each functional change. The shortlist improves early decisions; all remainder ordering stays legacy to preserve strength. Only after 2a passes do we introduce rank‑aware pruning gates in 2b.

## Phase 2b — Rank‑Aware Pruning Gates (Plan)

Goal
- Use move rank (from RankedMovePicker yield index) to scale pruning/reduction heuristics, lowering nodes and improving effective depth without harming tactics.

Guardrails
- Non‑PV nodes only to start; depth gate (≥ 4).
- Never gate: TT move, checks, promotions, recaptures; protect killers/countermoves.
- No reordering; only adjust thresholds/reductions. No changes in quiescence.
- Behind a single UCI toggle: `UseRankAwareGates` (default false).

Telemetry
- Track searched/pruned/reduced by rank bucket; PVS re‑search frequency; first‑move cutoff; nodes per depth.

Sub‑Phases (SPRT‑gated)
1) 2b.0 — Scaffolds (no behavior change)
   - Add `UseRankAwareGates` toggle; thread rank from picker to negamax; add telemetry structs (compiled‑out in Release).
   - Commit: "feat: 2b.0 scaffolds; bench <nodes>". SPRT: none.

2) 2b.1 — Rank capture + telemetry (no behavior change)
   - Compute rank once per move; record buckets for searched/pruned/reduced. No gating.
   - Commit: "feat: 2b.1 rank capture + telemetry". SPRT: none.

3) 2b.2 — LMR scaling by rank (conservative)
   - Non‑PV, depth ≥ 4. Buckets: rank 1..K → minimal/no reduction; K+1..10 → normal; 11+ → +1 step reduction (capped). Exempt TT/checks/promotions/recaptures/killers/countermoves.
   - Commit: "feat: 2b.2 LMR rank scaling". SPRT: [-3.00, 3.00].

4) 2b.3 — Move Count Pruning (LMP) rank gating
   - Non‑PV, quiets only, depth 4–8. Earlier pruning for late‑rank quiets; disable for top‑rank moves; keep exceptions.
   - Commit: "feat: 2b.3 LMP rank gating". SPRT: [-3.00, 3.00].

5) 2b.4 — Futility margin scaling by rank (quiets)
   - Non‑PV quiets; modestly increase margins for late ranks; leave top‑rank unchanged. Depth ≥ 3; guard recaptures/refutations.
   - Commit: "feat: 2b.4 futility rank margins". SPRT: [-3.00, 3.00].

6) 2b.5 — Capture SEE gating by rank
   - For late‑rank captures, require SEE ≥ 0 (or small threshold). Always allow recaptures/checks/promotions. If SEEMode=OFF, skip or gate more conservatively.
   - Commit: "feat: 2b.5 capture SEE gating by rank". SPRT: [-3.00, 3.00].

7) 2b.6 — Depth‑aware protected rank window R(depth)
   - R(depth) = clamp(4 + depth/2, 6..12). Moves with rank ≤ R bypass pruning and use minimal reduction.
   - Commit: "feat: 2b.6 protected ranks by depth". SPRT: [-3.00, 3.00].

8) 2b.7 — Re‑search smoothing (PVS)
   - If late‑rank moves frequently trigger re‑search, reduce aggressiveness one notch at that depth; track PVS re‑search freq by rank.
   - Commit: "feat: 2b.7 re‑search smoothing". SPRT: [-3.00, 3.00].

9) 2b.8 — Safety + DEBUG asserts
   - DEBUG asserts ensure gates do not apply to PV/TT/check/promo/recap/killers/countermoves; optional one‑shot DEV log of rank buckets.
   - Commit: "chore: 2b.8 debug asserts/logs". SPRT: N/A.

SPRT Configuration (default)
- Dev: `UseClusteredTT=true`, `UseRankedMovePicker=true`, `UseInCheckClassOrdering=true`, `UseRankAwareGates=true`.
- Base: same, but `UseRankAwareGates=false`.
- Common: Hash=128, Threads=1, TC 10+0.1, Book UHO_4060_v2, bounds [-3.00, 3.00].

## Progress Log (2a Postmortem)

- 2025-09-10: 2a.3 regression identified in OpenBench (test 498). Expanding shortlist to include quiets caused a large negative result (≈ −200 nELO): Elo | −209.06 ± 29.49; LLR −3.13. Root cause analysis indicates quiet shortlist over‑prioritization at shallow depth (killer/countermove/history/CMH weights allowed quiets to outrank strong captures), lack of class priority (captures/promotions vs quiets), and no depth gating for quiets.
  - 2a.3a (disable quiets in shortlist; keep captures + non‑capture promotions) recovered ~90% of the loss vs original base; residual small negative remained.
  - A (promotions‑only toggle) and B (reduce K from 10→8) independently showed small negatives (≈ −10 to −12 nELO over ~3k games).
  - Combined A+B plus parity‑preserving capture shortlist (derive shortlist captures from legacy order with ttMove=NO_MOVE; shortlist K=8) produced effectively neutral results (≈ −3 nELO within ±9 after 3k games). Conclusion: issue primarily due to quiet shortlist; remaining drift addressed by aligning shortlist order with legacy.
  - Lessons recorded above: enforce captures‑first discipline when mixing classes, depth‑gate quiet shortlist (≥ depth 6), keep quiet weights conservative, and use parity‑preserving shortlist derivation from legacy ordering when needed.

## Progress Log (2b Implementation Status - 2025-09-13)

### Completed Phases:
- **2b.0-2b.1**: Scaffolds and rank telemetry - Successfully implemented
- **2b.2**: LMR scaling by rank - Initially failed SPRT (LLR -3.05), fixed with more conservative approach:
  - Added !weAreInCheck guard to avoid reducing check evasions
  - Removed +1 reduction tier for ranks 11+ (was over-reducing at shallow depths)
  - Now only protects top ranks: Rank 1 (no reduction), Ranks 2-5 (minimal reduction), Ranks 6+ (unchanged)
- **2b.3**: LMP rank gating (non-PV, depth 4-8) - Successfully implemented
- **2b.4**: Futility margin scaling by rank (non-PV quiets, depth≥3) - Successfully implemented
- **2b.5**: Capture SEE gating by rank (non-PV, depth≥4) - Successfully implemented
- **2b.7**: PVS re-search smoothing - Successfully implemented with conservative approach:
  - Tracks re-search rates per depth×rank bucket
  - Applies smoothing when rate exceeds 20% after 32 attempts
  - Effects: LMR reduction -1, LMP uses baseline (no rank adjustment)

### Abandoned Phases:
- **2b.6**: Depth-aware protected rank window R(depth) - ABANDONED due to regressions
  - Concept: R(depth) = clamp(4 + depth/2, 6..12) with moves rank ≤ R bypassing pruning
  - Result: Caused performance regressions in testing
  - Decision: Skipped entirely, moved directly to 2b.7

- **2b.8**: Safety + DEBUG asserts - Implemented but rolled back
  - Initially added comprehensive DEBUG assertions and one-shot logging
  - Decision: Rolled back to keep branch clean for SPRT testing
  - Can be re-added later if needed for debugging

### Final Branch State:
Branch `feature/20250912-phase-2b-rank-aware-gates` ends at commit f7e7ebb with Phase 2b.7 as the final implementation. All rank-aware gates are behind `UseRankAwareGates` UCI toggle (default false for safety).
