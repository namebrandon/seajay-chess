Null Move Pruning (NMP) and Static-Null Analysis
================================================

Owner: search/negamax.cpp (primary), search/quiescence.cpp (context)
Last reviewed: 2025-09-06

Goal: Identify and fix correctness and performance issues in Null Move Pruning (NMP) and Static Null (reverse futility) pruning, and outline low‑risk improvements for thread‑safety and LazySMP readiness.

---

TL;DR — Bugs Found
------------------

- Static‑null endgame guard is missing (and was previously “backwards” per TODO). This enables static‑null in zugzwang‑prone endgames, causing tactical blindness and instability.
- Previously fixed: TT store for static‑null used wrong bound (UPPER) and wrong depth; this was a major regression. Now correct (LOWER, depth 0), but see additional safeguards below to avoid table churn.
- Replacement policy can let heuristic `NO_MOVE` entries overwrite more useful entries at equal depth; this reduces TT move ordering effectiveness and hurts pruning.
- Minor NMP implementation risks: overly permissive activation at shallow material; verification search is correct, but some guards can still be tightened.

---

Current Implementation Snapshot (Key Points)
-------------------------------------------

- Static‑Null (reverse futility) check (negamax.cpp):
  - Condition: `!isPvNode && depth <= 8 && depth > 0 && !weAreInCheck && abs(beta) < MATE_BOUND - MAX_PLY`.
  - Margin: depth‑scaled (aggressive for shallow, slows after 3).
  - Return: if `staticEval - margin >= beta`, returns early with `staticEval - margin`.
  - TT store: LOWER, score=`beta`, depth 0, `move=NO_MOVE` (fixed in 58b9d14).

- Regular Null Move:
  - Gating: `!isPvNode && !weAreInCheck && depth >= minDepth && ply > 0 && !consecutiveNull && nonPawnMaterial(sideToMove) > ZUGZWANG_THRESHOLD`.
  - Reduction R: UCI‑configurable, +1 if `(staticEval - beta) > margin`.
  - On fail‑high, optional verification search at depth `depth - R` from same node.

- Endgame guards exist for razoring, but not for static‑null.

---

Bugs and Fixes
---------------

1) Static‑Null lacks endgame/zugzwang guard (root cause of residual ELO loss)

- Symptoms:
  - Static‑null triggers in positions where null‑like reasoning is unsafe: low non‑pawn material for one or both sides (zugzwang prone), endgames with only pawns, or positions with very tight margins.
  - Leads to premature cutoffs and degraded TT content (even with LOWER@depth0 store). This shows up as instability at deeper iterations and lingering −5 to −15 nELO.

- Fix:
  - Add a material/phase guard identical (or slightly stricter) than razoring’s:
    - Disable static‑null if either side’s non‑pawn material is below a threshold (e.g., 1200–1400 cp), or if both sides are “endgame” per your existing detection.
    - Optional: Disable when side‑to‑move NPM is below R+B (~830 cp) or when both sides’ NPM are below the endgame threshold.

- Concrete guard (conceptual):
  - `npmUs  = board.nonPawnMaterial(board.sideToMove()).value()`
  - `npmThem= board.nonPawnMaterial(~board.sideToMove()).value()`
  - If `npmUs < 1300 || npmThem < 1300` then skip static‑null.

- Rationale: You already apply this guard for razoring; static‑null has similar failure modes in endgames. Enabling it yields more stable deep search and improves TT reuse.

2) Heuristic TT entries can overwrite more useful entries (equal‑depth same‑key)

- Issue:
  - `TranspositionTable::store` updates same‑key entries when `depth >= entry->depth`. Heuristic stores (static‑null, depth 0, `NO_MOVE`) can overwrite a depth‑0 quiescence entry that has a valid move. This harms ordering and cutoffs.

- Fix (low‑risk):
  - On same‑key replacement, prefer entries with a move:
    - If incoming `move == NO_MOVE` and existing `move != NO_MOVE` and `depth <= entry->depth`, skip replacement.
    - Also avoid bound downgrades at equal depth.
  - On collision (different key), be conservative replacing with `NO_MOVE` unless clearly deeper or victim is old gen.

- Impact: Preserves TT move availability, improves first‑move cutoff rate, reduces node counts.

3) Static‑Null depth range likely too large (depth <= 8)

- Issue:
  - Reverse futility is typically a shallow heuristic. Allowing it up to depth 8 risks tactical misses and over‑pruning at medium depths.

- Fix (tunable):
  - Tighten to `depth <= 3` or `<= 4` initially; expose as UCI option for SPSA if desired. Combine with endgame guard.

4) Minor: dynamic R tweak sign is OK, but ensure consistency

- Current increment condition `if (staticEvalComputed && staticEval - beta > evalMargin) R++` is reasonable (larger margin above beta → bigger R). Keep it, but document the intent.

---

Thread‑Safety and LazySMP Readiness (TT)
----------------------------------------

The current TT is single‑slot (direct‑mapped) with a 16‑byte `TTEntry`. For LazySMP / multi‑threading, consider:

- Write ordering: `save()` writes fields then `genBound`. Without fences, other threads might observe torn entries. Minimal change:
  - Make `genBound` an `std::atomic<uint8_t>`.
  - In `save()`: write all fields, then `genBound.store(value, std::memory_order_release)`.
  - In `probe()`: read `auto gb = genBound.load(std::memory_order_acquire)`, check `gb != 0` before reading other fields.
  - This ensures other fields are visible after `genBound` indicates “valid”.

- Wider safety: If you want to stay POD, at least place a compiler barrier before/after writing `genBound` and when reading it. Atomics are preferable.

- Set‑associative buckets (2–4 way):
  - Improves hit rate and reduces destructive collisions (critical under SMP churn).
  - Victim selection by depth+age.
  - Still lock‑free; per‑slot writes use the same release/acquire protocol.

- Stats atomics: use `memory_order_relaxed`, or compile out in Release for lower overhead.

- Prefetch: already present. Keep.

---

Efficiency Improvements (Hot‑Path)
---------------------------------

- Avoid repeated `zobristKey()` calls within the same block; cache per node.
- Use relaxed atomics for TT stats (`fetch_add(1, std::memory_order_relaxed)`), or no‑ops in Release.
- Consider gating very shallow static‑null/razor stores to reduce TT churn (e.g., skip stores at depth <= 1).

---

Proposed Code Changes (Summary)
-------------------------------

1) Static‑Null Guards (negamax.cpp):
- Add endgame/zugzwang guard mirroring razoring’s NPM checks (both sides). Optionally make the threshold configurable.
- Consider lowering `depth <= 8` → `depth <= 3/4`.

2) TT Replacement Policy (transposition_table.cpp):
- Same‑key: avoid replacing entries with a move by heuristic `NO_MOVE` entries at equal or lower depth.
- Bound quality: avoid downgrades at equal depth.
- Collision: require clearly deeper or old‑gen if incoming `move == NO_MOVE`.

3) Thread‑Safety (transposition_table.h/cpp):
- Make `genBound` atomic; release on write, acquire on read.
- Change stats increments to relaxed.

---

Validation Plan
---------------

- Unit/bench A/B:
  - Positions where static‑null previously triggered in endgames: confirm it is now gated.
  - Measure TT move first‑move cutoff % and TT hit rate trend vs depth (should increase or stay stable).
  - Node counts and NPS (expect slight improvement from reduced TT churn).

- SPRT vs parent commit:
  - Expect recovery of remaining −10 nELO and small positive due to stability.

---

Appendix — Rationale Notes
--------------------------

- Static‑null is powerful but dangerous in zugzwang/endgames. The missing guard is the primary source of residual loss after the TT bound/depth fix.
- Protecting TT entries that carry moves retains ordering quality; heuristic `NO_MOVE` entries shouldn’t evict them at equal depth.
- Minimal atomic discipline (release/acquire on the validity byte) provides cheap safety for future LazySMP.

