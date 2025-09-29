Title: Integration – Evaluation Framework & Pawn Evaluation Focus

Branch Context
- Integration branch: integration/eval-framework
- Parent baseline: main (post-2025-09-18)
- Primary collaborators: evaluation, telemetry, and tooling stakeholders

Objectives
- Build repeatable tooling to diagnose evaluation errors, starting with pawns/passed pawns.
- Instrument SeaJay to expose term-level telemetry (via EvalExtended and new logs) without prohibitive overhead.
- Establish curated test packs ("alt bench") for evaluation stress positions and measure behaviour vs. reference engines.
- Maintain performance visibility (NPS, nodes, time) so accuracy gains do not regress speed.

Out of Scope
- No immediate adjustments to non-pawn evaluation terms beyond instrumentation hooks.
- No search algorithm rewrites (LMR, pruning) unless directly required for telemetry interpretation.
- No OpenBench submissions until alt bench + telemetry tooling demonstrate stability.

Current Signals
- Problem suite points to understated king danger once material is balanced and undervalued advanced passers.
- ExtendedEval UCI option exists but lacks structured output capture.
- Previous king-safety work suffered from phase gating and overhead; lessons apply to pawn work.

Broader Evaluation Themes (Backlog)
- **King danger calibration:** recurring underestimation of direct attacks (g-file batteries, heavy-piece infiltration). Revisit safety tables, scaling with attacking pieces, and phase-aware gating once pawn work stabilises.
- **Piece activity / infiltration:** SeaJay favors quiet manoeuvres over dominant centralisation. Need future pass to audit mobility, outpost, and rook-file bonuses after pawn telemetry lands.
- **Search-eval interaction:** Some misplays stem from aggressive pruning or flashy tactical choices (e.g., Re5+). Track interactions between eval changes and move ordering/reduction heuristics in later phases.
- **Tactical threat awareness:** Ensure evaluation penalises loose pieces/pins; feed findings into future tactical investigation branch.

Telemetry & Instrumentation Plan
- Harden `EvalExtended` output: structured (tagged) breakdown by term, phase-interpolated contributions, pawn hash keys.
- Add optional DEBUG/DEV logging guard (`EvalLogFile`) to persist snapshots per root for curated FENs.
- Sample counters: number of pawn-eval invocations, cache hits/misses, seconds spent in pawn eval (Release timing off by default, on via compile flag).
- Ensure toggles compile out in production builds (macro or runtime flag) to avoid regressing OpenBench.
  - ✅ 2025-09-19: Structured `info eval` output and `EvalLogFile` toggle landed.

Evaluation Focus – Pawns & Passed Pawns
- Audit current pawn hash data: structure keys, cached values, phase blending.
- Catalogue pawn-term components (structure penalties, passer bonuses, blockader penalties, king proximity bonuses).
- Define hypotheses per component (e.g., passer bonus lacking king distance scaling, insufficient rook support bonus).
- Tie hypotheses to measurable expectations on curated positions before any code change.

Game-Phase Considerations
- Document how interpolation weights apply to pawn terms (opening vs endgame).
- For each planned tweak, specify expected phase deltas (e.g., boost late-endgame passer weights only).
- Validate via telemetry by capturing term contributions at 0.2, 0.5, 0.8 phase fractions.

Alt Bench / Test Suites
- Create `tests/packs/eval_pawn_focus.epd` (initial set: external/problem_positions plus additional pawn-heavy FENs).
- Add new UCI command or CLI flag (`bench eval-critical`) to run suite quickly, recording: score, depth, principal move, per-term breakdown when `EvalExtended` on.
- Baseline expected results from Komodo/Fritz to compare.
- Include both Release (timing) and Debug (verbose logs) runs in workflow.
  - ✅ 2025-09-19: Initial pawn/king-danger pack landed at `tests/packs/eval_pawn_focus.epd`.
  - ✅ 2025-09-28: Wrapper script `tools/eval_harness/run_eval_pack.sh` automates pack execution and summary export.

External Harness
- Python driver to: run multiple engines over pack, capture stdout/JSON, produce diff tables (score delta, best move agreement, term deltas).
- Integrate with telemetry logs (parse `EvalExtended` tags) to graph term contribution differences.
- Provide overhead report: time per position, NPS delta vs standard bench.
  - ✅ 2025-09-28: Harness CLI implemented with SeaJay telemetry capture (`tools/eval_harness/compare_eval.py`).
  - ✅ 2025-09-28: Komodo (non-NNUE) auto-integrated as reference engine for baseline comparisons.

Milestones & Deliverables
1. Tooling Foundation (Phase P1)
   - Harden EvalExtended output, add logging guard, document usage.
   - Deliver README snippet + example capture.
2. Alt Bench Integration (Phase P2)
   - Implement eval-critical suite + harness skeleton.
   - Record baseline metrics vs reference engines.
3. Pawn Term Audit (Phase P3)
   - Map existing coefficients, produce table with phase weights.
   - Capture telemetry on curated pack, identify top discrepancies.
4. Hypothesis Testing (Phase P4+)
   - Prioritise one pawn-term tweak at a time; ensure instrumentation validates impact.
   - Gate changes behind branch toggles until OpenBench-ready.

Testing & Validation Framework
- For each phase, run: `bench` (standard) + `bench eval-critical` (new pack) in Release.
- Debug builds: enable EvalExtended + EvalLogFile; archive outputs under `docs/project_docs/telemetry/eval-framework/`.
- External harness comparisons stored as CSV/plots in same folder, versioned by phase.

Performance Monitoring
- Track NPS, nodes, depth for both standard bench and eval-critical suite.
- When instrumentation enabled, collect perf counter snapshots (Instructions, L1 misses) once per phase.
- Ensure toggles default off; document cost when on via timing table in README appendix.

Documentation & Reporting
- Update `docs/project_docs/integration_branches.md` with branch summary.
- Maintain running log under `docs/project_docs/telemetry/eval-framework/` describing experiments.
- Report phase completion using feature guidelines template (bench counts, ELO expectations, risk notes).

Risks & Mitigations
- Overhead from logging -> keep instrumentation compile-time gated, add sampling.
- Subjective hypothesis chase -> require telemetry evidence before code changes.
- Data overload -> external harness automates comparison, keep pack size manageable (≤40 FENs initially).

Current Status (P4 – Passed Pawn Scaling)
- Tuned baseline passer defaults are live (commit `f531133`), and blockade handling now removes the “defended stop” bonus when the square is occupied (commit `4b7b229`).
- Eval harness defaults increased to 200 ms with `StabilityThreshold=8` so the pawn pack reaches depth 12+ on the problematic FENs before we diff against references.
- OpenBench test 702 (+11 nELO on `endgames.epd`) confirms the passer adjustments are performance-neutral to positive; continue using the same harness snapshot as the baseline for future tweaks.
- 2025-10-02: `feature/eval-framework/pawn-path-tiers` landed tiered promotion-path logic and quadratic king-distance ramp. Initial OpenBench test 703 vs. `main` returned −2.9 ± 4.3 nELO (7,736 games, Threads=1, Hash=128 MB). Result indicates the legacy SPSA passer coefficients no longer match the gated bonuses; schedule a retune once the remaining pawn-stage branches are in place.
- 2025-10-02: `feature/eval-framework/pawn-semi-open-liability` adds semi-open file surcharges for isolated/backward pawns when enemy heavies contest the file, with UCI knobs (`EvalSemiOpenLiabilityPenalty=12`, `EvalSemiOpenGuardRebate=4`) ready for SPSA after the pawn suite lands. Await telemetry to confirm guard rebates before tuning.
- 2025-10-03: `feature/eval-framework/pawn-loose-pawns` introduces "loose pawn" penalties when forward pawns lack friendly pawn defenders. Defaults (`EvalLoosePawnEnemyHalfPenalty=6`, `EvalLoosePawnOwnHalfPenalty=2`, `EvalLoosePawnPhalanxRebate=3`) only fire when the pawn isn't part of a phalanx, keeping the surcharge lightweight until SPSA rebalances with the other pawn branches.
- OpenBench test 709 (EvalPasserPhaseP4 enabled) vs. integration baseline recorded +7.8 ± 4.7 nELO after 6k games, so loose-pawn penalties appear SPRT-positive within error; keep defaults for now and revisit during the combined passer/sem-open SPSA sweep.
- 2025-10-03: `feature/eval-framework/pawn-phalanx` differentiates same-rank phalanx support vs. advancing phalanxes and adds rook-backed bonuses. New knobs (`EvalPasserPhalanxSupportBonus=8`, `EvalPasserPhalanxAdvanceBonus=12`, `EvalPasserPhalanxRookBonus=6`) retain the older “connected passer” boost only as a fallback when no true phalanx is present.
- 2025-10-03: `feature/eval-framework/pawn-candidate-levers` ports lever-based candidate detection with bonuses for safe pushes, support ratios, and rank progress. Knobs (`EvalCandidateLeverBaseBonus=4`, `EvalCandidateLeverAdvanceBonus=6`, `EvalCandidateLeverSupportBonus=5`, `EvalCandidateLeverRankBonus=3`) will tune alongside passer/phalanx values. OpenBench test 713 vs. `integration/eval-framework` (EvalPasserPhaseP4 enabled) returned −1.11 ± 4.20 nELO with LLR −1.42, which we accept as neutral given the new SPSA knobs; branch merged to `integration/eval-framework` on 2025-10-03.
- 2025-10-03: `feature/eval-framework/pawn-bishop-color` adds bishop/pawn color-complex scoring with telemetry (harmony, tension, blocked central same-color counts) and exposes SPSA knobs (`EvalBishopColorHarmonyBonus=2`, `EvalBishopColorTensionPenalty=2`, `EvalBishopColorBlockedPenalty=3`). New EvalExtended detail block `bishop_color` surfaces light/dark pawn splits for tuning.

Next Steps – Pawn Stage
1. **`feature/eval-framework/pawn-path-tiers` – Promotion-path tiers:** Implementation complete on feature branch. Next action: rerun SPSA (or equivalent) on the P4 passer knobs after the other pawn-stage updates land so the new tiering recovers the lost Elo headroom.
2. **`feature/eval-framework/pawn-semi-open-liability` – Semi-open liability penalties:** Implementation complete on integration branch; telemetry validation via `eval_harness` still pending before the combined SPSA sweep.
3. **`feature/eval-framework/pawn-loose-pawns` – Loose and unsupported pawns:** Implementation complete; next action is to archive EvalExtended traces for the pawn pack and carry the loose-pawn signals into the upcoming threat work.
4. **`feature/eval-framework/pawn-phalanx` – Phalanx and connected bonuses:** Implementation complete on feature branch; queue telemetry validation and include the new phalanx knobs in the consolidated SPSA sweep with passer/semi-open settings.
5. **`feature/eval-framework/pawn-candidate-levers` – Candidate/lever logic:** Implementation merged; next step is to capture post-merge telemetry on the pawn pack and bundle the lever knobs into the combined SPSA batch with passer/phalanx/semi-open parameters.
6. **`feature/eval-framework/pawn-bishop-color` – Bishop/pawn color complexes:** Telemetry-aware scoring implemented on feature branch with default knobs (`HarmonyBonus=2`, `TensionPenalty=2`, `BlockedPenalty=3`). Next steps: capture eval pack telemetry, confirm bishop_color trace coverage, and stage SPSA parameters alongside other pawn knobs once logs land.
7. **`feature/eval-framework/pawn-span-tension` – Pawn span & tension metrics:** Build pawn-span masks (advancePathMasked) to drive queen/rook infiltration, pawn-tension counts, and pending pawn-push threats; feed these into both evaluation and telemetry.
8. **`feature/eval-framework/pawn-telemetry-refresh` – Telemetry + harness coverage:** After each feature lands, refresh EvalExtended logs and the pawn pack summaries (positions 2, 3, 7, 8, 10, 13); keep per-branch artifacts so SPRT evidence stays isolated.

Once telemetry updates for items 1–5 are archived, draft a consolidated SPSA request covering the passer tiers, semi-open liability, loose-pawn, phalanx, and candidate lever knobs using the 713 harness snapshot as the baseline.

Future Stage – King Safety & Pawn Storms
- Once the pawn-stage items land, branch off for the king-focused work (shield degradation, pawn storms, king-pawn tropism, attack tables) using branch names such as `feature/eval-framework/king-shield` and `feature/eval-framework/king-storms`. Consolidate requirements from Laser’s king heuristics and our existing `king_safety.cpp`, but keep those changes isolated from the pawn commits.
- Revisit harness packs (`endgames.epd` plus middlegame tactical suites) to ensure the king updates are validated separately from the pawn work.
