# Move Picking Optimization Plan

## Overview

Refactor SeaJay's move ordering so that nodes pick the next best move incrementally instead of repeatedly invoking `std::stable_sort`. The goal is to eliminate large temporary buffers, reduce per-node overhead, and unlock an expected +8–12% NPS gain while keeping each change small, testable, and reversible.

## Success Criteria

- Replace the hot `stable_sort`/`stable_partition_adaptive` paths with in-place selection logic while preserving current search behaviour.
- Bench regression threshold: ≤0.5% NPS delta per phase (e.g., ±7,400 nps on 1.48 Mnps baseline).
- No SPRT failure: every phase runs OpenBench (10+0.1) vs `main` with bounds `[-3.00, 3.00]` unless otherwise specified.
- Profiling (Callgrind) shows the `std::__merge_sort_with_buffer` symbol reduced by ≥70% after Phase MP3.

## Baseline Measurements (2025-10-02)

- Branch: `feature/20251002-move-picking`
- Base commit: `963efe7d65dfae994848afcbb285ec0e797190ca`
- Bench: `echo "bench" | ./bin/seajay` → **1,482,403 nps** (2,508,823 nodes in 1.689s)
- Callgrind: `__merge_sort_with_buffer` = 2.86% Ir, `std::stable_sort` visible in multiple search paths.

## Phase Breakdown

### Phase MP0 – Instrumentation & Safety Net (Infrastructure)

- **Objective:** Add lightweight monitoring so later phases can verify ordering behaviour and cutoff distribution.
- **Changes:**
  - Add counters in move ordering to track selection counts and compare against existing totals (guarded by `SEAJAY_DEBUG_MOVE_PICKING`).
  - Emit optional info strings when `SearchStats` is enabled (ensure default-off to avoid runtime noise).
- **Testing:**
  - Bench (Release) — expect identical NPS.
  - `go depth 9` with `SearchStats` to confirm counters remain zeroed when disabled.
- **Commit Template:**
  - `feat: instrument move ordering baseline (Phase MP0)

    bench <nodes>`
  - SPRT: Skip (no behaviour change).

### Phase MP1 – Capture Scoring Cache

- **Objective:** Precompute capture scores once per node to avoid repeated MVV-LVA evaluation during selection.
- **Changes:**
  - Introduce `CaptureScoreBuffer` (stack array) populated alongside `MoveList` creation.
  - Replace repeated `scoreMove` calls in the hottest `stable_sort` invocation with cached values.
  - Prototype a `MovePickerStage` enum that models the “TT → good captures → killers → quiets → bad captures” flow (inspired by Publius). MP1 will add the scaffolding but keep current behaviour.
- **Testing:**
  - Bench — expect ≤0.5% NPS change.
  - Callgrind spot-check: `scoreMove` share should drop in the sorted section.
  - SPRT vs `main`, bounds `[-3.00, 3.00]` (expect neutral).
- **Commit Message:** `refactor: cache capture scores before ordering (Phase MP1)

bench <nodes>`

### Phase MP2 – Incremental Selection for Captures

- **Objective:** Replace capture `stable_sort` with in-place “pick-next-best” selection while maintaining quiet move order.
- **Changes:**
  - Implement `selectNextCapture(...)` within the stage machine so `NextMove()` can yield good captures one-by-one.
  - Split captures into “good” and “bad” buckets (SEE-based guard) so bad captures defer to a later stage.
- **Testing:**
  - Bench — expect up to +5% NPS gain (verify). Any drop triggers rollback.
  - `perft 6` (parity check).
  - SPRT bounds `[0.00, 5.00]` (targeting positive gain).
- **Commit Message:** `feat: incremental capture selection in move ordering (Phase MP2)

bench <nodes>`

### Phase MP3 – Quiet Move Selection Integration

- **Objective:** Extend incremental selection to quiet moves (killers/history/countermoves) removing remaining `stable_sort` usage.
- **Changes:**
  - Promote the stage machine to handle quiet move buckets (killers → counter-move → general quiets) with duplicate suppression so TT/killer moves never repeat.
  - Add mode hooks so quiescence can request “checks only” while still injecting TT/killer quiets when they exist.
- **Testing:**
  - Bench — expect additional +3–5% NPS.
  - Callgrind — confirm `__merge_sort_with_buffer` falls below 1% Ir.
  - SPRT bounds `[0.00, 5.00]`.
- **Commit Message:** `feat: incremental quiet move picking for alpha-beta (Phase MP3)

bench <nodes>`

### Phase MP4 – Cleanup & Telemetry Removal

- **Objective:** Remove temporary debug code and finalise instrumentation.
- **Changes:**
  - Strip `SEAJAY_DEBUG_MOVE_PICKING` scaffolding and simplify the stage machine API for production use.
  - Update docs (`external/performance_analysis_2025-10-02.txt`) with new profiling numbers.
- **Testing:**
  - Bench.
  - SPRT optional (skip if MP3 already stable).
- **Commit Message:** `chore: cleanup move picking instrumentation (Phase MP4)

bench <nodes>`

## Validation Checklist per Phase

- [ ] `./build.sh Release`
- [ ] `echo "bench" | ./bin/seajay`
- [ ] `go depth 9` sanity (if behaviour change expected)
- [ ] Optional: `valgrind --tool=callgrind ./bin/seajay bench` (MP2 & MP3)
- [ ] SPRT submitted & results recorded in feature tracker
- [ ] Update plan with actual bench/SPRT outcomes

## Rollback Strategy

- Maintain a checkpoint tag per phase (e.g., `git tag move-picking-MP1`).
- If SPRT fails, revert to previous phase tag and spin a `bugfix/` branch to investigate before resuming.
- Document failures and mitigations in this plan (append notes at the end).

## Open Questions

- Should we pipeline incremental selection into qsearch pruning helpers (SEE) simultaneously or defer to a follow-up feature?
- Is additional instrumentation required for `killers`/`history` hit rates once selection changes?

## 2025-10-03 Rollback Notes

- SPRT 771 (`a0beed684257c008e1985b7809c367b25f579abf` vs `main`) failed at −39 ± 14 Elo, so the staged picker rewrite was reverted.
- A control SPRT on the same commit with `UseRankedMovePicker=false` vs `true` produced +22 ± 10 Elo, confirming the regression is isolated to the new picker code path.
- Next iteration should restart from `16152a47c5e77570ea3d8c22dfff86b85d0cbd3b`, reintroducing incremental stages gradually while preserving feature parity and validating each change with telemetry.

## 2025-10-04 Telemetry & Diagnostics

- Extended `MovePickerStats` output to capture first-cutoff buckets and TT availability (see `docs/project_docs/move_ordering_telemetry.md`).
- Added UCI option `UseUnorderedMovePicker` to bypass move ordering for diagnostic baselines.
- Ordered vs unordered depth-10 comparison on `r3k2r/...`: ~127k nodes vs ~3.19M nodes (≈96% reduction with ordering); first-cutoff TT usage ≈65%.
- Next focus area: improve first-move fail-high rate (current average ~87%) using the new bucket telemetry.

## 2025-10-05 TT Coverage Instrumentation

- Instrumented the transposition table with per-ply coverage buckets split by PV/non-PV/quiescence. `debug tt` now prints `Coverage` lines (≤32 plies) so we can spot where probes stop hitting.
- Added UCI toggle `LogRootTTStores` to emit `info string RootTTProbe/RootTTStore` lines for each depth. This makes it easy to correlate stored zobrist keys with subsequent misses in the harness logs under `logs/tt_probe/`.
- Search and quiescence both feed the new counters; instrumentation is on by default while ENABLE_TT_STATS remains active.
- Next steps:
  - Run the WAC.049/WAC.002 depth-10 harness (ordered + SEE-off controls) with `LogRootTTStores=true` to capture coverage traces.
  - Chart PV vs non-PV coverage from the `debug tt` snapshots to confirm where availability collapses.
  - Evaluate mitigation ideas (earlier root stores, TT size, cluster size) once the drop-off depth is confirmed.

## 2025-10-05 Aspiration Guardrails

- Iterative deepening now disables aspiration for one depth whenever the root PV changes. That single widened iteration keeps non-PV coverage ≥40 % about one ply deeper on WAC.049 (ply 9 vs 8) without touching picker heuristics.
- Full aspiration disable still pushes the cliff to ply 10–11, and unordered move picking keeps coverage high everywhere, confirming the gap is caused by root oscillation rather than TT replacement pressure.
- Final tuned defaults (Oct 5 2025 SPSA): `AspirationWindow=9`, `AspirationMaxAttempts=6`, `StabilityThreshold=5`.
- Suggested diagnostic order: (1) leave the guard in place, (2) if needed, explore `UseAspirationWindows`, `AspirationGrowth`, or root-order heuristics, and (3) use `LogRootTTStores=true` during every run so the coverage tables verify each tweak before we commit.

## 2025-10-06 SPRT Result

- OpenBench test #784 (`feature/20251002-move-picking` vs `main`, 10+0.1, Hash=128MB, 1 thread) with the new aspiration defaults produced **−44.47 ± 14.85 nELO** (LLR −2.99).
- Despite improved TT coverage depth (logs in `docs/project_docs/telemetry/TTCoverage_WAC_PostSPSA.md`), search strength regressed.
- Immediate follow-up actions:
  1. Analyse why better coverage is not translating to Elo – inspect root PV oscillation, aspiration growth mode, and TT fill rate (hashfull only ~0.3%).
  2. Run targeted experiments (WAC harness, specific tactical suites) toggling aspiration growth / root ordering before altering move-picker heuristics.
  3. Only schedule another SPRT once a mitigation shows neutral or positive performance in diagnostics.

## Reboot Checklist (TT Coverage & Aspiration Guard)

If resuming after a long pause:

1. **Branch / Commit**
   - Branch: `feature/20251002-move-picking`
   - Last commit: `bee16a0 feat: improve TT diagnostics and aspiration guard` (`bench 2549006`)

2. **Instrumentation**
   - TT coverage buckets are exposed via `debug tt` (`Coverage PV/NonPV/Q`).
   - Root probe/store tracing is available with `setoption name LogRootTTStores value true`.
   - Coverage logs for baseline diagnostics live in `logs/tt_probe/` (`wac049_*_ttcoverage.log`, `wac002_*`).

3. **Aspiration Guard**
   - Iterative deepening now disables aspiration for one iteration whenever the root PV changes (`src/search/negamax.cpp` around the iterative deepening loop).
   - This keeps non-PV coverage ≥40 % to ply 9 on WAC.049 (ordered) without touching move ordering.

4. **Telemetry Commands**
   - Ordered harness: `printf 'uci\nsetoption name LogRootTTStores value true\nposition fen <FEN>\ngo depth 10\ndebug tt\nquit\n' | ./bin/seajay > logs/tt_probe/<name>.log`
   - SEE-off control: add `setoption name SEEPruning value off` and `setoption name QSEEPruning value off` before `go`.

5. **Current Defaults (post-SPSA)**
   - `AspirationWindow = 9`
   - `AspirationMaxAttempts = 6`
   - `StabilityThreshold = 5`
   - These values came from the Oct 5 2025 directional SPSA run. Keep `LogRootTTStores=true` when validating future adjustments.

6. **Next Experiments**
   - If coverage still collapses at ply 9, evaluate root-order heuristics before touching deeper move-picker heuristics.

7. **Key Files**
   - Coverage data structures: `src/core/transposition_table.h/.cpp`
   - Search integration: `src/search/negamax.cpp`, `src/search/quiescence*.cpp`, `src/search/negamax_legacy.inc`
   - UCI options & telemetry: `src/uci/uci.cpp/.h`

8. **Telemetry Summary**
   - Latest ordered vs SEE-off coverage logs: `logs/tt_probe/wac049_*_ttcoverage*.log`, `logs/tt_probe/wac002_*_ttcoverage*.log`
   - Aspiration-off / unordered control logs present for comparison.

Re-run the WAC depth-10 harness with the above commands after any tuning or code change to confirm TT coverage and first-move fail-high metrics remain stable before proceeding to SPRT.
