# Check Extensions Implementation Plan (2025-09-19)

## Objectives
- Restore tactical coverage on forcing check sequences, starting with WAC.237 (`r5k1/pQp2qpp/8/4pbN1/3P4/6P1/PPr4P/1K1R3R b - - bm Rc1+; id "WAC.237";`).
- Introduce give-check aware extensions without triggering search explosions or runaway runtimes.
- Break work into individually testable increments, each ending with commit + push + OpenBench SPRT gating before the next phase.

## Guardrails
- **Timeouts**: All local harness invocations (`go movetime`, scripts, unit tests) must run under explicit wall-clock timeouts (CLI `timeout` wrapper or script-level guard) to prevent infinite searches.
- **Telemetry**: Log extension counts, seldepth, and node growth after every change; store traces under `docs/project_docs/telemetry/tactical/` with timestamps.
- **SPRT Discipline**: After each functional change, push branch and await human-triggered SPRT validation; no further coding until results reviewed.

## Stage Breakdown

### Stage 0 – Instrumentation & Safety Harness
**Goal**: Ensure we can observe extension usage and abort runaway searches.
- Add counters to `SearchInfo` for per-node extension totals; expose in UCI `info string` when `DebugTrackedMoves` is active.
- Update `tools/run_tactical_investigation.py` to accept `--timeout-ms` (default 120000) and kill the engine process on expiry.
- Create helper script `tools/run_wac_237_check.sh` that runs WAC.237 at `movetime {100, 850, 2000}` with `timeout` wrappers and logs to telemetry directory.
- **Validation**: Run helper script twice; confirm engine exits normally and telemetry captures extension counters.
- **Gate**: Bench + unit tests + tactical harness; commit/push (message `chore: add check extension telemetry (Stage 0)` + bench number); await SPRT ack (expect neutral change).

### Stage 1 – Discovered/Double-Check Detection Fixes
**Goal**: Correct `isDiscoveredCheck()` and add double-check helper tested on WAC.237.
- Replace heuristic with bitboard-based uncover test; handle sliders on files, ranks, diagonals.
- Add `isDoubleCheckAfterMove()` that verifies both moving piece and uncovered slider attack the enemy king on the post-move board.
- Build unit test file (`tests/unit/search/discovered_check_tests.cpp`) covering WAC.237 and regression motifs.
- **Validation**: Run new unit tests + WAC.237 harness (Stage 0 tool) + bench.
- **Gate**: Commit/push (`feat: fix discovered check detection (Stage 1) - bench XXXXXXX`); wait for SPRT before Stage 2.

### Stage 2 – Generate Give-Check Flags During Move Generation
**Goal**: Cache give-check status cheaply for ordering and pruning.
- Extend move representation or side array to carry `gives_check` flag set during generation.
- Update move picker, LMR/LMP logic, and search stack to use cached flag instead of recomputing via `inCheck(board)`.
- Ensure quiescence uses the same flag when available (fallback to runtime check otherwise).
- **Validation**: Bench + WAC.237 harness + targeted WAC edge-file subset at 850 ms (timeout enforced).
- **Gate**: Commit/push (`feat: cache give-check flags (Stage 2) - bench XXXXXXX`); hold for SPRT.

### Stage 3 – Conservative Give-Check Extension
**Goal**: Introduce first extension increment with strict limits.
- Add per-node extension budget (e.g., max 2 plies) and per-branch total (e.g., 4) tracked via `SearchStack`.
- Extend non-capture checking moves when depth ∈ [3,6], move rank ≤ 3, and budgets available.
- Skip extension if parent already gave check to avoid chains; allow for TT move override when flagged.
- **Validation**: WAC.237 harness, WAC edge subset, full 300-position tactical suite at 850 ms (timeout). Record extension counts before/after.
- **Gate**: Commit/push (`feat: add limited give-check extension (Stage 3) - bench XXXXXXX`); run SPRT with conservative bounds (e.g., [-5, 5]); await results.

### Stage 4 – Double-Check Priority & Ranking Adjustments
**Goal**: Improve ordering/extension for discovered double checks without broadening scope.
- If Stage 1 flags double-check, allow one extra extension slot (still within total cap) and bump move ordering score to shove into top-3.
- Update telemetry to separately track double-check extensions vs single-check.
- **Validation**: WAC.237 harness (expect `Rc1+` selected at ≤ depth 12), double-check regression set (add at least three FENs), tactical suite at 850 ms.
- **Gate**: Commit/push (`feat: prioritize double checks (Stage 4) - bench XXXXXXX`); hold for SPRT.

### Stage 5 – Broader Tactical Regression & Cleanup
**Goal**: Verify improvements generalize and document outcomes.
- Run `tools/run_wac_test.sh` full 300 positions with timeout and compare solved count to Stage 0 baseline; append row to `tools/tactical_test_history.csv`.
- Execute focused OpenBench job once SPRT signals positive/neutral to confirm long-run stability (same TC/bounds as earlier investigative runs).
- Update `docs/project_docs/investigation_20250918_tactical_failures.md` with new stats and lessons learned.
- **Validation**: Bench + WAC harness + full tactical suite logs.
- **Gate**: Commit/push (`chore: document check extension results (Stage 5) - bench XXXXXXX`); after SPRT + documentation, conclude feature.

## Contingencies
- If any stage triggers timeout breaches or seldepth spikes (>256), pause, revert the stage commit, and open a remediation doc before retrying.
- If SPRT signals regression, branch off `integration/` sub-branch with the failing stage for deeper diagnostics while main investigation waits.

## Communication
- After every stage, post summary in `feature_status.md` (kept local) and update investigation doc (once Stage 5 reaches SPRT pass) so human reviewers have context before approving next SPRT run.
- Use commit template with `bench <nodes>` per feature guidelines to maintain OpenBench compatibility.
