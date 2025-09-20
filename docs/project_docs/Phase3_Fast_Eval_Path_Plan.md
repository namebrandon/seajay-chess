Title: Phase 3 – Fast Evaluation Path (qsearch + Pruning Decisions)

Objective
- Reduce time spent in evaluation during quiescence and static pruning checks without degrading tactical strength.
- Net effect: 5–20% time reduction in qsearch-heavy positions; +0.2–0.5 depth at 1s/move in mixed suites; OpenBench non-regression or small gains.

Non‑Goals
- No functional chess rule changes. No eval term additions. No multi-threading behavior changes beyond per-thread ownership of new data.

High-Level Design
- fast_eval(board) = material (phase‑interpolated) + PST (phase‑interpolated) + optional pawn-structure cache.
- Use fast_eval only where bounds math is safe: qsearch stand‑pat, static null‑move margin check, futility/razoring prechecks, and delta pruning comparisons.
- Keep full eval for TT stores that rely on exact scores and for principal search nodes (unless explicitly A/B’d in later phases).

Thread‑Safety / LazySMP Readiness
- All caches and scratch used by fast_eval must be per‑thread (worker‑owned) objects; passed by reference down the call chain.
- No mutable singletons. No global state beyond readonly tables (PST). Align per‑thread structures to cache lines to avoid false sharing (alignas(64)).
- Use benign data races only where explicitly stated (not expected here). Avoid atomics in hot path; sample counters if needed.
- Ensure fast_eval is const w.r.t. Board public API; it may read Board’s incremental material/PST/pawn keys but must not mutate them.

Interfaces (proposed)
- files: `src/evaluation/fast_evaluate.h`, `src/evaluation/fast_evaluate.cpp`.
- API: `namespace eval { Score fastEvaluate(const Board& board); }`
- Gating UCI options (default off initially):
  - `UseFastEvalForQsearch` (bool)
  - `UseFastEvalForPruning` (bool) // gates futility/razor/delta/null‑move static checks separately

Instrumentation
- Counters: qsearchNodes, standPatReturns, deltaPruneSkips already exist; add optional sampling timers for eval vs qsearch body time (DEBUG/DEV only).
- Add aggregated counters for: fastEvalCalls, fastEvalUsedInStandPat, fastEvalUsedInPruning (sampled or behind DEBUG to avoid overhead).

SPRT Guidance (nELO bounds)
- Infrastructure phases (no behavior change): [-5.00, 3.00].
- Minor gains expected phases: [0.00, 5.00] (or [2.00, 8.00] if clearly impactful).
- Target TC: 10+0.1, Hash=128MB, Threads=1 for initial A/B; re‑run at Threads=NN post‑Phase 3F to confirm LazySMP neutrality.

Sub‑Phases (small, self‑contained, SPRT‑testable)

Phase 3A: Scaffolding & UCI toggles (0 ELO expected)
- Deliverables
  - Add `fast_evaluate.{h,cpp}` with a stub calling full `evaluate(board)` internally (no behavior change yet).
  - Wire UCI: `UseFastEvalForQsearch=false`, `UseFastEvalForPruning=false` (parsed, printed, stored; not used).
  - Add DEBUG counters (compiled out in Release) and minimal hooks in qsearch/negamax, guarded behind toggles but returning full eval.
- Tests
  - Build + bench identical. Unit/perft unchanged.
- SPRT
  - Bounds: [-5.00, 3.00]. Expect PASS (neutral).
- Commit msg example
  - feat: Phase 3A – Fast eval scaffolding (toggles only)
  - bench 19191913

Phase 3B: Material-only fast_eval (qsearch stand‑pat only)
- Deliverables
  - Implement `fastEvaluate` using material only (Board already tracks material; if not, compute fast path cheaply).
  - Use in qsearch stand‑pat when `UseFastEvalForQsearch=true`.
- Risk controls
  - No use in pruning yet; stand‑pat must remain a safe lower bound for side-to-move.
- Tests
  - Tactical suite unchanged; qsearch node/time profile shows small drop in eval time.
- SPRT
  - Bounds: [0.00, 5.00]. Expect small/no change; PASS required.
- Commit
  - feat: Phase 3B – Use material fast_eval for qsearch stand‑pat (gated)
  - bench <count>

Phase 3C: Add PST to fast_eval (qsearch stand‑pat)
- Deliverables
  - Incorporate phase‑interpolated PST via Board’s incremental PST score (`Board::m_pstScore`), avoiding recomputation.
  - Keep identical interpolation logic as full eval to preserve bounds relation.
- Tests
  - Unit: equality with full eval on positions where only material+PST terms are non‑zero (mobility/safety disabled scenarios).
  - Depth/time on 10–20 FEN suite; check stand‑pat return rate and qsearch nodes.
- SPRT
  - Bounds: [0.00, 8.00]. Expect positive or neutral.
- Commit
  - feat: Phase 3C – Add PST to fast_eval for stand‑pat (gated)
  - bench <count>

Sub‑stages (recommended)
- 3C.0 Shadow parity (no behavior change):
  - Compute fast_eval(PST+material) alongside full evaluate() on a 1/N sample of stand‑pat nodes.
  - Compare only when non‑PST terms are provably zero; log histogram of differences in DEBUG.
  - SPRT: [-5.00, 3.00].
- 3C.1 Implement PST path but still return full eval:
  - Integrate Board’s incremental PST + material and exact mg/eg interpolation used by full eval.
  - Keep behavior unchanged (stand‑pat still returns full eval); record divergence stats.
  - SPRT: [-5.00, 3.00].
- 3C.2 Switch stand‑pat to fast_eval (gated):
  - Under `UseFastEvalForQsearch=true`, stand‑pat returns fast_eval; retain dev toggle to revert.
  - SPRT: [0.00, 8.00].

Phase 3D: Pawn‑structure cache (thread‑local) [optional]
- Deliverables
  - Use `Board::pawnZobristKey` as lookup key; small per‑thread cache for pawn eval term needed by PST phase interpolation if applicable.
  - Start with direct compute fallback; enable cache with `UseFastEvalForQsearch=true` only.
- Thread‑safety
  - Per‑thread direct‑mapped or 4‑way set cache (1–2 KB), alignas(64). No sharing across threads.
- Tests
  - Microbench: cache hit rate on pawn-heavy suites; ensure correctness via asserts comparing fallback occasionally in DEBUG.
- SPRT
  - Bounds: [0.00, 5.00]. Expect minor improvements.
- Commit
  - feat: Phase 3D – Add thread‑local pawn cache for fast_eval (gated)
  - bench <count>

Sub‑stages (recommended)
- 3D.0 Baseline timing (no cache):
  - Add DEBUG timers/counters for pawn term within fast_eval to estimate potential win.
  - SPRT: [-5.00, 3.00].
- 3D.1 Cache API (shadow fill):
  - Implement per‑thread cache keyed by `pawnZobristKey`; on miss, compute and fill, but always use the freshly computed value (don’t read cache).
  - SPRT: [-5.00, 3.00].
- 3D.2 Enable cache reads for fast_eval:
  - On hit, use cached value; on miss, compute+fill. Add sampled cross‑checks in DEBUG.
  - SPRT: [0.00, 5.00].
- 3D.3 Size/alignment sweep:
  - Evaluate 64/128/256 entries, direct‑mapped vs 4‑way set; ensure `alignas(64)` and padding to avoid false sharing.
  - SPRT: [0.00, 5.00].

Phase 3E: Use fast_eval for futility/razoring prechecks
- Deliverables
  - In main search, replace full eval reads in precheck comparisons with fast_eval when `UseFastEvalForPruning=true`.
  - Keep margins conservative; do not change existing margins initially.
- Risk controls
  - Maintain existing guards: no futility when in check, near mate bounds, or when TT suggests fail‑high.
- Tests
  - Node explosion stats: monitor futilityAttempts/prunes and qsearch entry rate.
- SPRT
  - Bounds: [0.00, 8.00]. Expect small positive from reduced eval cost and earlier prunes.
- Commit
  - feat: Phase 3E – Fast eval in futility/razor prechecks (gated)
  - bench <count>

Sub‑stages (recommended)
- 3E.0 Shadow audit (no behavior change):
  - Compute fast_eval and compare to full eval versus existing margins; log “would‑change decision” counts by depth.
  - SPRT: [-5.00, 3.00].
- 3E.1 Depth=1 futility only:
  - Replace static value with fast_eval at depth 1 when not in check and far from mate; keep razor unchanged.
  - SPRT: [0.00, 5.00].
- 3E.2 Extend to depth=2:
  - Same guards; still no razor. Monitor futilityPrunes %.
  - SPRT: [0.00, 5.00].
- 3E.3 Razor gating with fast_eval:
  - Replace static check for razoring where currently allowed; keep TT/near‑mate/endgame guards.
  - SPRT: [0.00, 8.00].

Phase 3F: Null‑move static margin check via fast_eval
- Deliverables
  - Replace static eval used for null‑move precheck with fast_eval when `UseFastEvalForPruning=true`.
- Risk controls
  - Keep R reduction conservative; retain existing zugzwang/endgame guards; verification search unchanged.
- Tests
  - Track null‑move attempts, passes, verification rates; ensure fail‑high/verification ratios remain sane.
- SPRT
  - Bounds: [0.00, 8.00]. Expect neutral/positive.
- Commit
  - feat: Phase 3F – Fast eval in null‑move static check (gated)
  - bench <count>

Sub‑stages (recommended)
- 3F.0 Shadow audit (no behavior change):
  - Compare fast_eval vs full eval against the null‑move margin; log cases where decision would flip.
  - SPRT: [-5.00, 3.00].
- 3F.1 Shallow depths (verification path only):
  - Use fast_eval for static check at shallower depths where verification search still runs.
  - SPRT: [0.00, 5.00].
- 3F.2 Deep/trust paths:
  - Apply to trusted null‑move paths while retaining existing zugzwang/endgame guards.
  - SPRT: [0.00, 8.00].

Phase 3G: Delta pruning in qsearch using fast_eval
- Deliverables
  - Use fast_eval as the baseline for delta pruning comparisons (capture margins still computed via SEE/material deltas).
- Risk controls
  - Keep existing tactical guards; skip delta pruning if giving check or near mate.
- Tests
  - qsearch delta prune skip counts; tactical pass rate unchanged.
- SPRT
  - Bounds: [0.00, 8.00].
- Commit
  - feat: Phase 3G – Use fast_eval for delta pruning baseline (gated)
  - bench <count>

Sub‑stages (recommended)
- 3G.0 Shadow audit (no behavior change):
  - Compare pruning decisions using full‑eval baseline vs fast‑eval baseline; count flips and categorize by margin.
  - SPRT: [-5.00, 3.00].
- 3G.1 Large‑delta captures only:
  - Enable fast_eval baseline where SEE/material delta >> margin (safe zone).
  - SPRT: [0.00, 5.00].
- 3G.2 Full delta pruning set:
  - Use existing margins with fast_eval baseline; preserve “gives check” and near‑mate guards.
  - SPRT: [0.00, 8.00].

Phase 3H: Micro‑optimizations & cold‑path cleanup
- Deliverables
  - Force‑inline fast_eval; branchless accumulation; reduce sign flips; minimize parameter passing.
  - Mark debug counters as cold; compile out in Release; consider sampling (1/N calls) for timings.
- Tests
  - CPU profile before/after; bench stability check.
- SPRT
  - Bounds: [0.00, 5.00].
- Commit
  - perf: Phase 3H – Fast eval micro‑optimizations
  - bench <count>

Acceptance Gates per Sub‑Phase
- All unit/perft tests pass.
- Local depth‑vs‑time shows neutral or improved depth at 1s/move on 10–20 FEN suite.
- Node explosion diagnostics neutral/improved.
- OpenBench SPRT PASS with configured bounds.

Best Practices
- Keep toggles default off until a phase passes OpenBench; flip defaults only after the final 3G/3H pass.
- Do not store fast_eval values in TT (avoid pollution); TT store remains based on search scores.
- Maintain monotonicity of comparisons (alpha/beta bounds) when substituting fast_eval to avoid behavioral flips.
- Use Board’s incremental PST/material; avoid recomputing from scratch.
- Prefer per‑thread state accessed via SearchData/ThreadState rather than TLS to aid testing.

Pitfalls & Watch‑outs
- Bias in stand‑pat if PST phase weighting diverges from full eval’s interpolation.
- Double‑counting or sign errors (White vs Black perspective) when using incremental PST; add assertions in DEBUG.
- False sharing on per‑thread caches; pad/align and avoid placing hot counters adjacent to caches.
- Atomics in hot path for counters – compile out or use memory_order_relaxed and sampling.
- Null‑move over‑prune in zugzwang‑like endgames – keep existing endgame guards.
- Razoring/futility near mate scores – ensure margins leave safe buffer from MATE/−MATE.

Validation & Tooling
- Build: `make` (OpenBench), `./build.sh Release` (local). See docs/BUILD_SYSTEM.md.
- Benchmark: `echo "bench" | ./bin/seajay` (capture node count for commit message).
- Depth vs time: `./tools/depth_vs_time.py --time-ms 1000 --engine ./bin/seajay --fen startpos --fen "<FEN>" --out depth_vs_time.csv`.
- Tactical: `./tools/tactical_test.py ./bin/seajay external/UHO_4060_v2.epd 2000`.
- Diagnostics: node explosion report, qsearch ratio and stand‑pat rate from existing stats.

Branching & Commits
- Branch: `feature/DEPTHP-fast-eval` (see docs/project_docs/Git_Strategy_for_SeaJay.txt).
- Commit messages must include exact bench line:
  - Example: `bench 19191913`
- Push after each sub‑phase; run OpenBench A/B vs `main`.

Roll‑Forward / Roll‑Back
- Each change gated by UCI flags for rapid A/B and rollback.
- Keep prior binaries for quick manual A/B (`bin/seajay.prev`).
- If any phase fails: decompose further (3E→3Ea/3Eb for futility vs razor), or revert toggle default and proceed with next safe micro‑phase.

Post‑Phase Checks (Threaded Readiness)
- Run smoke with `Threads=N>1` to ensure no data races (TSAN/ASAN builds if available).
- Verify no cross‑thread writes in caches by construction; confirm equal strength single vs multi‑thread (normalized nELO neutrality within noise).

Ready‑to‑Implement Checklist
- [ ] UCI toggles parsed, printed, and stored in `uci.h/.cpp` (default false).
- [ ] `fast_evaluate.{h,cpp}` created; uses Board incremental fields; no globals.
- [ ] qsearch stand‑pat path guarded by `UseFastEvalForQsearch`.
- [ ] Pruning checks (futility/razor/null/delta) guarded by `UseFastEvalForPruning`.
- [ ] DEBUG assertions for monotonicity and sign.
- [ ] Instrumentation compiled out in Release.

SPRT Summary Table (suggested)
- 3A: [-5.00, 3.00]
- 3B: [0.00, 5.00]
- 3C: [0.00, 8.00]
- 3D: [0.00, 5.00]
- 3E: [0.00, 8.00]
- 3F: [0.00, 8.00]
- 3G: [0.00, 8.00]
- 3H: [0.00, 5.00]

Notes
- Keep changes minimal and local per sub‑phase (≤2–3 files touched) to isolate regressions.
- Do not modify TT storage semantics in this phase.
- Defer using fast_eval for TT store or non‑qsearch principal nodes to a future investigation.
