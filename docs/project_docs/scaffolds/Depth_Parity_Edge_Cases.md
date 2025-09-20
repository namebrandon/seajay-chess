Title: Depth Parity – Edge Cases and Gotchas (Keep Top of Mind)

General
- TT size and collisions: Ensure UCI Hash is consistent across A/B; tiny TT sizes can reverse expected gains.
- Hash resize mid-search: Disallow or defer until next ‘ucinewgame’; resizing during search invalidates pointers.
- Mate score adjustments: Always convert to/from ply-relative before TT store/probe; check both main and quiescence paths.
- Root vs non-root TT usage: Never early-return from TT at root; still compute bestMove/PV.
- Repetition detection: Consistent use of search stack; multi-thread designs must keep per-thread stacks.

MovePicker
- Duplicate moves: TT move may also appear in captures/quiets; ensure de-duplication to avoid double search.
- Illegal pseudo-legal moves: Maintain tryMakeMove() guard before counting as legal.
- PV nodes: Only first legal child is PV; MovePicker must not break this assumption.
- Quiescence: Capture‑only mode must include promotions and en passant; preserving special ordering (queen promos front).
- Killer staleness: Skip killers not present in generator; avoid probing board for wrong-side killers.

SEE & Ordering
- SEE cost: Ensure SEE evaluation is lightweight and noexcept; avoid repeated board recomputation.
- Promotions as captures: SEE logic must account for promotion semantics (attacker is pawn pre‑promotion).

Transposition Table
- NO_MOVE pollution: Don’t let heuristic NO_MOVE writes evict deeper move-carrying entries.
- Bound correctness: EXACT/LOWER/UPPER classification must use original alpha; avoid mis-stores.
- Generation wrap: 6-bit generation wraps; replacement policy must remain sane across wrap.
- Cluster scan: Always scan all entries in cluster before declaring miss; track mismatches.

Quiescence & Pruning
- Check extensions: Respect maxCheckPly; panic modes must not bypass.
- Delta pruning: Ensure captured piece values and margins align with SEE decisions; avoid double-counting.
- Razoring/NMP interaction: Be careful adding fast-eval; guard aggressively on TT/LMP contexts.

Time Management & Info
- Iterative info cadence: Don’t spam UCI; ensure per-thread info consolidated if moving to SMP.
- Stability factor: In SMP, stability metrics may differ per thread; compute at root controller.

Threading / LazySMP
- Per-thread history/killers/countermoves: no sharing without locks; merge only at iteration boundaries if desired.
- TT sharing: Shared across threads; ensure memory alignment and avoid false sharing.
- Stop flag: Use atomic stopFlag consistently; check at safe intervals.

Testing & Reproducibility
- Seeded behavior: If random tie-breakers exist in MovePicker, seed per-thread deterministically to keep reproducibility.
- Cross-platform alignment: 64-byte alignment assumptions must hold on all targets (Windows/MSVC included).

