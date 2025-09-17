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

## Completed to Date (Phase 3D.0–3D.3)
- Extracted pawn evaluation logic into shared helpers (`pawn_eval.{cpp,h}`) so both full and fast paths use identical scoring.
- Added per-thread, 8 KB pawn cache (`FastEvalPawnCache`) in `fast_evaluate.cpp` keyed by `Board::pawnZobristKey` with shadow-fill instrumentation (Phase 3D.1).
- Fast eval stores pawn totals in the cache and, when toggled on, reuses them so stand-pat/pruning paths see material + PST + cached pawns; toggles off keeps legacy mat+PST behavior.
- DEBUG counters track shadow store/compute counts (`pawnCacheShadowStores`, `pawnCacheShadowComputes`).
- Phase 3D.2: Added DEBUG parity sampling comparing cached pawn scores against freshly recomputed values, tracking histograms, mismatch counts, and max deviation with 1/64 sampling to keep overhead negligible.
- Phase 3D.3: Enabled pawn cache consumption when `UseFastEvalForQsearch/Pruning` toggles are set. Fast eval now reuses cached pawn totals (miss → recompute/store, hit → reuse + sampled parity check), global config mirrors UCI toggles, and parity telemetry accounts for the pawn term.
- Added board-context validation (side to move, king squares, game phase, and per-passer blocker fingerprints) to the cache probe so stale entries are rejected. Parity sampling now reports zero divergence once the context matches.
- Added `debug fast-eval` UCI command to snapshot/reset telemetry so we can monitor cache hit rate, parity histogram, and usage counters during local testing (Phase 3D.5 groundwork).

## Remaining Phases & Tasks
### 3D.1 – Shadow Fill (DONE)
- ✅ Populate cache with fresh pawn scores on every call.
- ✅ Record hit/miss stats without altering behavior.

### 3D.2 – Shadow Compare (DONE)
- ✅ DEBUG-only parity checks compare cached pawn score vs freshly computed value (1/64 sampling).
- ✅ Histogram, mismatch count, and max-diff telemetry added to `FastEvalStats` for diagnostics.
- ✅ Acceptance: tooling in place to flag any divergence beyond rounding noise during upcoming runs.

### 3D.3 – Enable Cache Reads (DONE)
- ✅ Cache hits now reuse stored pawn totals when fast-eval toggles are active; misses recompute + store.
- ✅ DEBUG parity sampling remains in place (1/64 hits) with histogram + max-abs tracking, and cache hit/miss counters added for telemetry.
- ✅ Global engine config mirrors UCI toggles so parity checks adjust expected baselines.

### 3D.4 – Pruning Integration (Optional/Gated)
- When `UseFastEvalForPruning=true`, reuse cached pawn score inside pruning contexts that invoke `fastEvaluate`.
- Ensure null-move audit continues to sample/validate against full eval.

### 3D.5 – Telemetry & Validation (IN PROGRESS)
- ✅ `debug fast-eval` now prints aggregated counters/histogram and supports `reset`.
- ✅ Debug build restored; two 5-FEN telemetry rounds (750 ms and 500 ms movetime) show 0/23 730 and 0/9 375 parity mismatches respectively, max diff 0 cp. Hit rate ~29% with context guards (23 730 hits vs 58 470 misses in the longer run) prior to cache sizing change; 50-position WAC sample (500 ms) with the 512-entry table yields 76 850 hits / 177 100 misses (~30.3%) with parity still clean.
- 🔜 Expand telemetry to depth_vs_time and tactical suites to assess node/time impact and refine cache sizing now that correctness holds.
- 🔜 Compare qsearch timing with cache enabled/disabled once broader telemetry is captured.

### 3D.6 – SPRT Testing
- Configuration: `UseFastEvalForQsearch=true`, `UseFastEvalForPruning=false` initially.
- Bounds: `[0.00, 5.00]` per main plan.
- After PASS, consider enabling pruning flag and rerun SPRT.

### 3D.7 – Documentation & Cleanup
- Update `docs/project_docs/Phase3_Fast_Eval_Path_Plan.md` progress log once phases complete.
- Ensure comments/DEBUG code compiled out in Release.
- Decide on default toggle state (likely remain off until later consolidation).

## Open Questions / Risks
- Cache size: current 256-entry direct-mapped table may need tuning (measure hit rate under heavy pawn-structure churn now that context gating reduces reuse).
- Multi-thread safety: thread-local cache avoids contention; confirm no accidental sharing in worker management.
- Hit rate vs. correctness trade-off: context gating ensures accuracy but lowered hit rate; evaluate whether additional metadata (e.g., clustered sets) or larger table improves reuse.

## Next Immediate Actions
1. Capture extended telemetry (depth_vs_time, tactical suite, node-explosion) to quantify hit rate and performance impact with context-aware caching.
2. Review cache sizing and potential associativity tweaks if hit rate remains low on larger suites.
3. Decide gating strategy (depth/qply guards if needed) before enabling pruning-side consumption (Phase 3D.4).
4. Once telemetry looks stable, stage `UseFastEvalForPruning` audits, then prep SPRT configuration per 3D.6.
