# SeaJay Phase 4 Review (Codex Notes)

These notes summarize a code-grounded review of Phase 4 (Search-Specific Optimizations), with confirmations from the codebase, corrections to the plan where it’s out of date, and concrete recommendations. SeaJay uses negamax with side-to-move scoring.

Last reviewed: 2025-08-30

## Quick Summary

- TT move ordering is already TT-first after MVV-LVA ordering. The plan’s “TT ordered last” bug does not match current code.
- LMR already uses a logarithmic reduction table. Treat plan’s “switch to logarithmic” as done; focus on tuning.
- Clear wins remaining in Phase 4:
  - Depth-preferred TT replacement policy.
  - TT prefetching at probe sites.
  - Fast-path killer validation before pseudo-legal validation.
  - Store true static eval in TT entries (separate from search score) and reuse.
- Treat futility > depth 4 cautiously; current engine intentionally caps at 4 based on prior tests.
- Singular extensions are disabled; a proper reimplementation is a larger task and should follow TT improvements.

## Code Confirmations and Pointers

### TT Move Ordering (Already Correct)
- File: `src/search/negamax.cpp`
- Function: internal helpers + `orderMoves(...)` and TT-first adjustment.
- The TT move is moved to the front after MVV-LVA/killer/history/countermove ordering:
  - `orderMoves` (templated local helper) calls MVV-LVA ordering variants then moves `ttMove` to the front when present.
  - Calls:
    - Ordering: `orderMoves(board, moves, ttMove, &info, ply, prevMove, info.countermoveBonus);`
    - Then TT-first move adjustment inside `orderMoves`.
- Conclusion: No change needed; update plan text to reflect current behavior.

### Transposition Table Implementation (Gaps)
- Files: `src/core/transposition_table.h/.cpp`
- Current behavior:
  - Single-entry indexing with multiplicative hash mix: `index(key) = (key * 0x9E3779B97F4A7C15ULL) & m_mask;`
  - `store(...)`: always-replace policy (overwrites entry unconditionally).
  - `probe(...)`: single-slot probe; checks high 32 bits (`key32`).
  - `TTEntry` (16B) packs: `key32`, `move`, `score`, `evalScore`, `depth`, `genBound` (6-bit gen + 2-bit bound), padding.
- Missing/Recommended:
  1) Depth-preferred replacement: Only replace when new depth ≥ stored depth, or when generation changes. Pseudocode:
     ```cpp
     TTEntry* e = &m_entries[idx];
     const uint32_t k32 = key >> 32;
     const bool differentPos = !e->isEmpty() && e->key32 != k32;
     const bool staleGen = e->generation() != m_generation;
     if (e->isEmpty() || staleGen || depth >= e->depth) {
         e->save(k32, move, score, evalScore, depth, bound, m_generation);
     } else if (differentPos) {
         // Optional: replace if new depth much deeper than existing
         if (depth + 2 >= e->depth) e->save(...);
     }
     ```
  2) Prefetch at probe sites: In search hot path(s):
     - Before probing: `__builtin_prefetch(&tt->m_entries[tt->index(zobristKey)], 0, 1);`
     - Add in `negamax.cpp` (and `quiescence.cpp`) where `tt->probe` is called, guarded by `if (tt && tt->isEnabled())`.
  3) Store true static eval: Currently `evalScore` is set to the same value as the search score. Store the node’s static evaluation instead to enable better reuse in pruning and improving detection.
  4) Hash indexing change (optional): The current multiplicative mix is fine on modern CPUs; the plan’s XOR-folding is not guaranteed faster. Benchmark before changing.
  5) Buckets (advanced): 4-way buckets can reduce collisions and improve strength; larger refactor—do after (1)-(3).

### Move Ordering: Killer Validation Fast-Path (Small Win)
- Files: `src/search/move_ordering.cpp`
- Current: Killers are validated with `MoveGenerator::isPseudoLegal` before being moved forward (good to prevent pollution).
- Improvement: Add a quick precheck before the heavier pseudo-legal validator:
  - If `killer != NO_MOVE` and not a capture/promotion, first check `board.pieceAt(from) != NO_PIECE` and `colorOf(piece) == board.sideToMove()`; only then call `isPseudoLegal`. This reduces validator calls on stale killers.

### Pruning
- Futility pruning: In `src/search/negamax.cpp` SeaJay intentionally caps futility at depth ≤ 4 (prior regressions beyond 4). Keep as-is unless SPRT shows gains with carefully tuned margins and “improving” logic.
- Null-move: Present with a static null-move check (reverse futility) path that uses cached static eval when available.
- Move-count pruning: Implemented conservatively with UCI tunables and safeguards for killers and countermoves.

### LMR (Already Logarithmic)
- Files: `src/search/lmr.h/.cpp`
- Uses a log(depth)*log(moves) reduction table with PV/improving adjustments and parameterization. Plan item to “switch to logarithmic” is done. Focus on parameter tuning (bounds and decay) via UCI.

### Singular Extensions
- Disabled in current code (explicitly guarded and commented). If re-introduced, follow the standard pattern:
  - Verify TT suggests a likely singular move.
  - Search other moves at reduced depth with narrow window to confirm singularity.
  - Extend only on confirmation.
  - Do this after TT and move ordering improvements for signal quality.

## Evaluation Function Review

- Files: `src/evaluation/evaluate.cpp/.h`, `src/core/board.cpp`
- STM scoring: `evaluate()` computes white-perspective components and flips by `board.sideToMove()`. `Board::evaluate()` memoizes per position and invalidates on STM changes. This matches negamax STM scoring throughout search.
- Pawn structure: Hashed (`board.pawnZobristKey()`), cached via `g_pawnStructure`, and features are reused.
- Mobility and safety:
  - Mobility: computed per side and combined.
  - King safety: implemented hook returns 0 in current phase; consider gating safety by phase (skip in simple endgames) and enabling incrementally when stable.
- Suggested improvements:
  1) Store true static eval into TT (`TTEntry::evalScore`) on store; reuse on probe to avoid recomputation and improve improving/non-improving logic.
  2) Phase gating: Continue leveraging `detectGamePhase(board)` to skip expensive features where irrelevant (e.g., skip king safety late; skip passed-pawn race logic early).
  3) Keep lazy evaluation disabled unless A/B shows strength parity (prior results showed losses); if revisited, expose via UCI and keep default OFF.

## Minor Bugs / Nits

- `TTEntry::isEmpty()` and perft TT: `isEmpty()` checks `(key32 == 0 && move == 0)`. In perft TT usage, entries are stored with `move=0`. If a position hashes to `key32==0`, an occupied slot may be miscounted as empty in sampling (e.g., `fillRate()`, `hashfull()`). Safer emptiness indicator:
  - Treat `genBound == 0` as empty (reserve generation 0 as empty), or set a dedicated “occupied” marker.
- `AlignedBuffer`: `std::aligned_alloc(64, size)` requires `size` be a multiple of 64. Today `m_numEntries * sizeof(TTEntry)` will generally be aligned, but add a debug assert `bufferSize % 64 == 0` to prevent future breakage if entry size or alignment changes.
- Plan vs code: Plan says “check mate/stalemate first”; with lazy legality in search, we only know stalemate/checkmate after trying all moves. Keep current order: do draw checks (periodically), then TT probe, and determine mate/stalemate after move loop.

## Patch Sketches (Recommended Next Steps)

1) Depth-preferred TT replacement (`TranspositionTable::store`):
```cpp
void TranspositionTable::store(Hash key, Move move, int16_t score, int16_t evalScore,
                               uint8_t depth, Bound bound) {
    if (!m_enabled) return;
    m_stats.stores++;
    size_t idx = index(key);
    TTEntry* e = &m_entries[idx];
    uint32_t k32 = static_cast<uint32_t>(key >> 32);

    if (!e->isEmpty() && e->key32 != k32) m_stats.collisions++;

    const bool canReplace = e->isEmpty()
                         || e->generation() != m_generation
                         || depth >= e->depth;  // depth-preferred
    if (canReplace) {
        e->save(k32, move, score, evalScore, depth, bound, m_generation);
    }
}
```

2) TT prefetch (probe sites):
```cpp
if (tt && tt->isEnabled()) {
    Hash key = board.zobristKey();
    __builtin_prefetch(&tt->m_entries[tt->index(key)], 0, 1); // consider adding a friend or accessor
    TTEntry* ttEntry = tt->probe(key);
    // ...
}
```

3) Store true static eval in TT (negamax store):
```cpp
// compute/retain staticEval for this node; then on store:
tt->store(zobristKey, bestMove, scoreToStore.value(), staticEval.value(),
          static_cast<uint8_t>(depth), bound);
```

4) Killer fast-path precheck (in `move_ordering.cpp` before pseudo-legal):
```cpp
if (killer != NO_MOVE && !isCapture(killer) && !isPromotion(killer)) {
    Square kf = moveFrom(killer);
    Piece p = board.pieceAt(kf);
    if (p != NO_PIECE && colorOf(p) == board.sideToMove()) {
        if (seajay::MoveGenerator::isPseudoLegal(board, killer)) {
            // promote killer in quiet section
        }
    }
}
```

## Recommended Plan Edits

- Replace Phase 4.1.a description with “Verify TT-first ordering (already in place); no change required.”
- Replace Phase 4.4.a description with “LMR already logarithmic; proceed to tune parameters via UCI/SPSA.”
- For Phase 4.2 keep: depth-preferred replacement (4.2.a), TT prefetch (4.2.b). Treat hash indexing tweak as experimental; do only if microbenchmarks justify.
- For 4.4.b futility: keep current cap at 4 unless SPRT validates depth 5–6 with position-improving logic and safe margins.
- Singular extensions (4.4.d): schedule after TT rework, using a standard verification-search method.

## Next Steps (Priority)

1) Implement TT prefetch at probe sites.
2) Implement depth-preferred TT replacement.
3) Store true static eval in TT; use it to avoid recomputation and to stabilize pruning decisions.
4) Add killer fast-path validation to reduce pseudo-legal checks.
5) Benchmark. If positive, consider bucketed TT later (with clear memory/strength trade-offs).

## Notes on Side-to-Move Evaluation

- Evaluation returns STM-oriented scores; search wraps negamax consistently. Mate score adjustments (store vs probe) account for ply distance. Keep this invariant intact when introducing TT/static eval changes.

---

Prepared by: Codex (Phase 4 review)
