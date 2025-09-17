Title: Phase 4 – Selective Search Micro-Tuning

## Objective
Reduce search tree size while preserving tactical strength by refining selective search heuristics (history/countermove gating, LMR reductions, null-move aggressiveness). Target: neutral or positive nELO with measurable depth or node improvements at 1s/move.

## Current Context
- Integration base: `integration/20250912-depth-and-search-speed`
- Phase predecessors: TT clustering (Phase 1), staged move picker (Phase 2), fast-eval path (Phase 3 – on hold)
- Branch: `feature/20250917-phase4-selective-tuning`

## Planned Sub-Phases
### 4.0 Baseline Telemetry (No Behavior Change)
- Capture current selective-search metrics (LMR re-search rate, null-move verification, history hit rate) for comparison.
- Verify tactical suite, depth-vs-time, node explosion reports to anchor future regressions.
- Commit (docs/telemetry only). No SPRT required.

### 4.1 Relax Quiet-Heuristic Gating (History/Countermoves)
- Reduce depth gate for history/countermoves from ≥6 to ≥2 (as planned in Phase 4 outline) with existing safety checks.
- Instrument history hit counts vs. re-search rate.
- Tests: quick tactical suite (≥80%), depth-vs-time (expect minor depth gain).
- SPRT: 10+0.1, bounds [0.0, 5.0]. Commit after SPRT results (include bench).

### 4.2 Null-Move Reduction Tuning
- Add conservative extra reduction step at depth ≥10 when eval - beta is large (guarded by TT bound context, no change to near-mate/endgame guards).
- Shadow audit counters (attempts, reductions, verification rate) before enabling.
- Enable change behind UCI flag `UseAggressiveNullMove` (default false) for A/B.
- SPRT: [0.0, 5.0]. Commit after SPRT (bench, final toggle decision).

### 4.3 LMR Refinement
- Use existing LMR table but compute "improving" via stack evals already tracked (no new eval work).
- Test incremental adjustments: begin with PV/non-PV guard tweaks and capture thresholds.
- Measure re-search rate, move-order cutoffs, tactical pass rate.
- SPRT: [0.0, 5.0]. Commit after PASS (bench).

### 4.4 Consolidation & Defaults
- If sub-phases PASS, decide whether to flip UCI defaults on integration branch.
- Update documentation, telemetry snapshots, and progress logs.
- Final SPRT with all Phase 4 changes enabled to confirm combined effect.

## Instrumentation & Telemetry Checklist
- `debug search` (existing) for null-move/LMR counters.
- Add `search::SelectiveStats` summary (calls, re-search, reductions) if needed.
- `tools/depth_vs_time.py` – maintain before/after CSVs.
- `tools/node_explosion_diagnostic.sh` – rerun after each major change.
- `./tools/tactical_test.py` – quick (500ms) before SPRT, full suite if regressions suspected.

## Testing & Validation Gates (per sub-phase)
- All unit/perft tests (already part of CI) pass.
- Depth-vs-time: neutral or improved depth/nodes at 1s/move (startpos + 4 FENs).
- Tactical suite: ≥baseline pass rate (80–85%).
- Node explosion: neutral or improved cutoff ratios.
- SPRT PASS at 10+0.1 (bounds noted above) before merging.

## Commit & Push Protocol
1. Implement change → local tests/telemetry.
2. `bench` run after Release build; include `bench <count>` in commit message.
3. Push to `feature/20250917-phase4-selective-tuning` immediately after commit for OpenBench access.
4. Initiate SPRT once change appears neutral/positive locally.
5. Record results (test ID, outcome) in docs and progress log.

## Progress Log (append-only)
- 2025-09-17: Phase 4 plan initialized. Baseline telemetry pending.
