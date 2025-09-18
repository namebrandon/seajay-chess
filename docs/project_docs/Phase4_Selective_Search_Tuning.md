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
- 2025-09-17: Completed Phase 4.0 baseline telemetry (Release build). Depth-vs-time (startpos + 4 FENs), node explosion comparison, and WAC tactical suite (500ms) recorded under `docs/project_docs/telemetry/phase4/`. Baseline results: depth 12–17 at 1s, avg node ratio 6.47× vs Stash, tactical hit rate 78.3%.
- 2025-09-17: Phase 4.1 (Relax history/CMH gate to depth ≥2) implemented. Added `SearchStats` history telemetry (`hist(...)`), reran depth-vs-time, node explosion, and WAC tactical suite. Tactical: 236/300 (78.7%), CMH activations visible at shallow depth (apps=970, cmh=279 on depth-6 startpos run). Data stored in `docs/project_docs/telemetry/phase4/`.
- 2025-09-17: Phase 4.2 (Aggressive null-move reduction scaffolding) implemented. Introduced `UseAggressiveNullMove` UCI toggle (default off), added TT-aware gating and telemetry (`extra(...)`) for candidate/apply/suppress counts, and confirmed Release build plus startpos depth-11 smoke test. Shadow telemetry (depth-vs-time/node-explosion/tactical) recorded for both toggle states; aggressive mode shows +25.7% nodes on the `8/4bk1p/...` defensive FEN at 1s and a modest tactical uptick (236/300 vs. 234/300) while `extra(...)` counters stayed at zero. Tooling updated to pass UCI options so OpenBench prep can exercise the feature—see `docs/project_docs/telemetry/phase4/Phase4_2_Null_Move_Tuning.md`.
- 2025-09-17: Harvested high-eval FENs that actually trigger the aggressive null move. Added `tools/collect_aggressive_null_candidates.py` and seeded `aggressive_candidates_default.csv` from the WAC suite (depth 10). WAC.095 and WAC.113 both produce applied candidates (4 and 1 respectively) and see large node drops at 1s when the toggle is on (−33%, −18%), while the defensive `8/4bk1p/...` FEN now shows only a +0.8% node uptick. Telemetry artefacts saved alongside Phase 4.2 records.
- 2025-09-17: Ran the 20-position candidate pack (`aggressive_candidates_pack.epd`) at 1 s with/without the toggle. Baseline results (`null_move_pack_metrics_1s.csv`) were +8.6% nodes (12 savings, 8 large regressions). Adding a high-eval guard (≥600 cp), positive-beta requirement, and 64-application cap brings the pack to −0.5% average nodes (`null_move_pack_metrics_1s_newgate.csv`) and keeps defensive `8/4bk1p/...` at a healthy −28% node delta. Toggle remains shadow-only for Phase 4 SPRT while we iterate on the remaining outliers.
