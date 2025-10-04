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
