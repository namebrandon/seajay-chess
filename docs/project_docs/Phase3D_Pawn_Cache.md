Title: Phase 3D – Fast Eval Pawn-Structure Cache

## Objective
Establish a lightweight, thread-local pawn-structure cache that allows `eval::fastEvaluate` to reuse the full evaluator’s pawn term without recomputing it, reducing qsearch and pruning overhead while preserving accuracy. Target: neutral or positive nELO at 10+0.1 with measurable qsearch time reductions once fully enabled.

## Current Context
- Branch: `feature/20250913-phase3-fast-eval`
- Base plan reference: `docs/project_docs/Phase3_Fast_Eval_Path_Plan.md`
- Relevant code:
  - `src/evaluation/fast_evaluate.cpp/.h`
  - `src/evaluation/pawn_eval.cpp/.h`
  - `src/evaluation/evaluate.cpp`

## Completed to Date (Phase 3D.0–3D.2 Shadow Work)
- Extracted pawn evaluation logic into shared helpers (`pawn_eval.{cpp,h}`) so both full and fast paths use identical scoring.
- Added per-thread, 4 KB pawn cache (`FastEvalPawnCache`) in `fast_evaluate.cpp` keyed by `Board::pawnZobristKey` with shadow-fill instrumentation (Phase 3D.1).
- Fast eval currently computes the pawn term and stores it in the cache (shadow mode); it still returns only material+PST.
- DEBUG counters track shadow store/compute counts (`pawnCacheShadowStores`, `pawnCacheShadowComputes`).
- Phase 3D.2: Added DEBUG parity sampling comparing cached pawn scores against freshly recomputed values, tracking histograms, mismatch counts, and max deviation with 1/64 sampling to keep overhead negligible.

## Remaining Phases & Tasks
### 3D.1 – Shadow Fill (DONE)
- ✅ Populate cache with fresh pawn scores on every call.
- ✅ Record hit/miss stats without altering behavior.

### 3D.2 – Shadow Compare (DONE)
- ✅ DEBUG-only parity checks compare cached pawn score vs freshly computed value (1/64 sampling).
- ✅ Histogram, mismatch count, and max-diff telemetry added to `FastEvalStats` for diagnostics.
- ✅ Acceptance: tooling in place to flag any divergence beyond rounding noise during upcoming runs.

### 3D.3 – Enable Cache Reads (Gated)
- Under `UseFastEvalForQsearch=true`, read pawn score from cache when hit; fall back to compute+store on miss.
- Maintain debug sampling to confirm correctness when using cached value.
- Measure local benchmarks (bench, depth_vs_time) before SPRT.

### 3D.4 – Pruning Integration (Optional/Gated)
- When `UseFastEvalForPruning=true`, reuse cached pawn score inside pruning contexts that invoke `fastEvaluate`.
- Ensure null-move audit continues to sample/validate against full eval.

### 3D.5 – Telemetry & Validation
- Extend `FastEvalStats` dump (DEBUG) to show cache hit rate, miss rate, average reuse per search.
- Update `uci debug fast-eval` (if available) to print cache telemetry.
- Run targeted suites:
  - `./tools/depth_vs_time.py --time-ms 1000 ...`
  - `./tools/tactical_test.py ...`
  - Node explosion diagnostics
- Compare qsearch node/time ratios with and without cache.

### 3D.6 – SPRT Testing
- Configuration: `UseFastEvalForQsearch=true`, `UseFastEvalForPruning=false` initially.
- Bounds: `[0.00, 5.00]` per main plan.
- After PASS, consider enabling pruning flag and rerun SPRT.

### 3D.7 – Documentation & Cleanup
- Update `docs/project_docs/Phase3_Fast_Eval_Path_Plan.md` progress log once phases complete.
- Ensure comments/DEBUG code compiled out in Release.
- Decide on default toggle state (likely remain off until later consolidation).

## Open Questions / Risks
- Cache size: current 256-entry direct-mapped table may need tuning (measure hit rate under heavy Pawn structure churn).
- Multi-thread safety: thread-local cache avoids contention; confirm no accidental sharing in worker management.
- Accuracy: ensure no divergence between cached and freshly computed scores; rely on DEBUG parity tooling.

## Next Immediate Actions
1. Gather local cache hit/miss + parity telemetry on representative suites to confirm instrumentation coverage.
2. Implement 3D.3 cache-read path gated by `UseFastEvalForQsearch`, preserving DEBUG parity sampling on hits.
3. Decide gating strategy (depth/qply guards if needed) based on observed reuse patterns before enabling pruning integration.
