# Evaluation Bias Investigation Index (2025-10-09)

Central tracker for the evaluation discrepancy effort spun out of `feature/20251002-move-picking`.
Use this when resuming work on branch `feature/20251009-eval-bias`.

## Objectives
- Quantify SeaJay's evaluation drift against trusted references (Komodo, Stockfish) across tactical and strategic suites.
- Prioritise fixes for queen/slider pressure, king safety scaling, and PST optimism highlighted during MP3 regression analysis.
- Confirm each evaluation change is Elo-neutral or better before re-merging into `main` and resuming move-picking phases.

## Key Assets
- `tools/eval_compare.py` – Per-FEN Komodo vs SeaJay depth≤14 comparison runner (60s timeout per engine) writing to the JSON tracker.
- `docs/issues/eval_bias_tracker.json` – Raw comparison data maintained by the script.
- `docs/issues/eval_bias_tracker.md` – Human-readable table generated from the JSON file.
- Legacy helpers: `tools/analyze_position.sh`, `external/problem_positions.txt` for deeper multi-engine snapshots.
- `docs/project_docs/Queen_Sack_Investigation_Plan.md` – staged remediation plan for queen/slider capture-check motifs (2025-10-09).

- 2025-10-11: QS3 danger detection now treats king-only defenses and bishop/rook contact checks as unsafe, generating explicit `qs3_king_danger` telemetry (e.g., +120 cp for `r3r1k1/pp1q1pp1/...`, `docs/project_docs/telemetry/eval_bias/eval_extended_r3r1k1.txt`). A new threat compensation hook (`qs3AttackerCompensationPercent`, default 60%) feeds a portion of that danger back to the attacker so the static eval surfaces sacrificial lines even before material is recovered (`src/core/engine_config.h:109-115`, `src/evaluation/evaluate.cpp:1955-1964`).
- 2025-10-11: Depth-18 rerun confirms large QS3 deltas shrink dramatically: SeaJay now scores +190 cp on the canonical queen-sack (`docs/issues/eval_bias_tracker.json:6-20`), while slider-led sacs pick up 150–200 cp of danger but still show search selectivity gaps (`docs/project_docs/telemetry/eval_bias/eval_extended_fen3.txt`).
- 2025-10-11: Remaining regression cases are search-bound. Depth-18 search on `r3r1k1/pp1q1pp1/...` still outputs −112 cp even though the static eval is +190 cp (`docs/project_docs/telemetry/eval_bias/eval_extended_r3r1k1.txt`), indicating follow-up work is needed on move ordering / pruning gates to capitalise on the stronger heuristic signal. Slider contact heuristics were reverted to avoid broad regressions; future QS3 work will be search-side.
- 2025-10-09: Depth-18 rerun completed alongside the original depth-14 sweep. Deviations persist (many outliers still ±150–300 cp) but depth-18 scores often diverge further, confirming root-cause issues in static evaluation and king-safety scaling.
- Instrumented queen-sacrifice case (`r1b1k2r/p2n1p2/...`) shows the reference move `g3e4` generated every iteration yet collapsing under pruning; once LMR/null/futility/SEE gates are disabled the engine plays `g3e4`, proving the move exists but is suppressed by selectivity (logs in `docs/project_docs/telemetry/eval_bias/fen_r1b1k2r_*.txt`).
- Second outlier (`6k1/p1p2pp1/...`) already chooses Komodo’s `h4g3` yet scores ≈−100 cp, underscoring the evaluation bias component (`docs/project_docs/telemetry/eval_bias/fen_6k1_default.txt`).
- TT telemetry remains healthy; the −44 nELO regression observed in MP3 is still attributed primarily to evaluation optimism.
- 2025-10-06: queen contact capture-checks now bypass LMR/LMP/move-count pruning in search (QS1). The 200 ms queen-sack sweep improved from 2/20 to 4/20 solved, confirming coverage gains while leaving evaluation optimism as the dominant blocker (`docs/project_docs/telemetry/queen_sack/tactical_queen-sack_2025-10-06_19-38-59.csv`).
- 2025-10-07: QS2 adds a checking-capture history kicker plus contact-check replay slot so sacrificial queen checks stay near the front even without TT help. Commit `fd569d9` landed on `feature/20251009-eval-bias` with `bench 2501279`, awaiting fresh telemetry to validate ordering lift (`src/search/history_heuristic.cpp`, `src/search/negamax.cpp`, `src/search/types.h`). Follow-up queen-sack sweep (`docs/project_docs/telemetry/queen_sack/tactical_queen-sack_2025-10-07_23-37-00.csv`) still solves 4/20 positions. Depth-18 eval comparisons (`docs/project_docs/telemetry/queen_sack/2025-10-07/eval_compare_queen_sack_depth18.md`) confirm SeaJay remains hundreds of centipawns more optimistic than Komodo on the key sac motifs, underscoring the need for QS3 evaluation work.
- 2025-10-09 (QS3 prototype): New king-danger heuristics (`useQS3KingSafety`) now penalise safe queen contact checks and flank shield holes (`src/core/engine_config.h:108-113`, `src/evaluation/evaluate.cpp:443-525`). The 200 ms queen-sack rerun (`docs/project_docs/telemetry/queen_sack/tactical_queen-sack_2025-10-09_21-02-16.csv`) still hits 4/20 motifs but surfaces the target move in 6 PVs; matching node-explosion data (`docs/project_docs/telemetry/queen_sack/node_explosion_queen-sack_2025-10-09_21-02-37.csv`) shows reduced info-line counts (25–37) but no additional conversions yet.
- 2025-10-09 (QS3 tuning): Strengthened penalties (`qs3SafeQueenContactPenalty=48`, `qs3ShieldHolePenalty=28`, `qs3SliderSupportPenalty=20`, `qs3NoMinorDefenderPenalty=24`) and shield rebate fix bump 200 ms coverage to 5/20 with seven PV hits (`docs/project_docs/telemetry/queen_sack/tactical_queen-sack_2025-10-09_21-09-48.csv`); latest node diagnostics (`docs/project_docs/telemetry/queen_sack/node_explosion_queen-sack_2025-10-09_21-10-06.csv`) remain compact while reflecting the more decisive queen-sac scoring.
- 2025-10-09 (Threats): Hanging-threat evaluator now ignores queen contacts that immediately check the enemy king, preventing the −40 cp penalty on post-sac positions and logging suppression in `EvalExtended` (`detail name=threat_suppression`). Root deltas still lean on material optimism, but sac follow-ups evaluate closer to reference (`src/evaluation/evaluate.cpp:586-706`, `src/evaluation/eval_trace.h:20-111`).

## Workflow
1. **Select FEN** from `external/problem_positions.txt` or new observations.
2. Run `python3 tools/eval_compare.py "<FEN>" --depth 18 --timeout 240` to record deeper comparisons (depth value clamps at 18). Keep existing depth-14 measurements for contrast.
3. Review the Markdown table diff for new deltas at both depths and prioritise the worst offenders.
4. For candidate fixes, log supporting evidence (EvalExtended terms) in `docs/issues/eval_bias_tracker.md` under a short notes section.
5. After coding a change:
   - Re-run `eval_compare.py` on the tracked FENs.
   - Capture any new data points.
   - Bench (`./build.sh Release`, `echo "bench" | ./bin/seajay`).
   - Stage succinct commits with `bench <nodes>` in the message.
6. Once local deltas trend toward zero and tactical suites stay neutral, queue an SPRT vs `main` before merging.

## Open Questions / Next Tasks
1. Monitor the queen contact check gating (QS1) for regressions and quantify how much evaluation reinforcement is still needed to push suite coverage toward the 12/20 target.
2. K-factor scaling for exposed kings on open files (positions `4r1k1/b1p3pp/...` and `8/p2r1k1p/...` remain outliers at both depths).
3. Slider pressure on undefended backbone squares (`2r3k1/2p1n1b1/...` cases show optimistic SeaJay scores).
4. Queen mobility penalties when trapped behind own pieces (`2r3k1/Qpb1qp1p/...` delta +228 cp needs investigation).
5. Interaction between PST bonuses and piece coordination—evaluate whether PST weights remain tuned after recent aspiration defaults.
6. **New (2025-10-07)**: Re-run queen-sack suite (`tools/tactical_investigation.py --suite queen-sack`) and node-explosion diagnostics to confirm ≥12/20 contact-check coverage with QS2 changes; archive outputs under `docs/project_docs/telemetry/queen_sack/2025-10-07/`. *(First run complete: 4/20, no change from QS1.)*
7. Capture updated history/ordering stats (`moveOrderingStats`, `historyStats`) from a bounded search trace (retry with `go nodes 300000` after the 1M-node attempt stalled) to ensure the new bonuses are being exercised.
8. **New (2025-10-10)**: Fold in external heuristics for QS3—safe queen-check pressure and king-zone attacker counts from Stashbot (`stash-bot/src/sources/evaluate.c#L760-L820`), flank shield/ storm penalties plus undefended-ring detection from Laser (`laser/src/eval.cpp#L392-L470`, `#L1061-L1190`), and integrate them with SeaJay’s `evaluateKingSafetyWithContext` hooks so queen sacs only score when the capturing square yields a safe follow-up check.
9. QS3 tuning pass: iterate on `qs3SafeQueenContactPenalty`, `qs3ShieldHolePenalty`, and defender weights using queen-sack telemetry and eval-compare deltas to push success ≥8/20 before scheduling SPRT.

## Telemetry Artifacts (2025-10-09)
- `docs/project_docs/telemetry/eval_bias/fen_r1b1k2r_default.txt` – baseline search trace for the Komodo queen-sacrifice FEN highlighting pruning pressure on `g3e4`.
- `docs/project_docs/telemetry/eval_bias/fen_r1b1k2r_heuristics_off.txt` – LMR/SEE-only disable; move remains suppressed until deeper pruning toggles are removed.
- `docs/project_docs/telemetry/eval_bias/fen_r1b1k2r_selectivity_off.txt` – null and futility disabled alongside LMR/SEE, yielding `g3e4` despite negative eval.
- `docs/project_docs/telemetry/eval_bias/fen_r1b1k2r_legal_moves.txt` – python-chess dump confirming `g3e4` generation (38 legal moves).
- `docs/project_docs/telemetry/eval_bias/fen_6k1_default.txt` – exposed-king FEN where move selection matches Komodo yet eval lags behind.
- `docs/project_docs/telemetry/eval_bias/selectivity_bounds_*.txt` – stepwise LMR/null/futility toggling logs for representative FENs; `selectivity_bounds_summary.md` captures the minimal heuristic sets required to surface Komodo’s choices.
- `docs/project_docs/telemetry/eval_bias/selectivity_probe_results.(json|md)` – automated sweep (movetime 2000 ms) comparing baseline vs relaxed selectivity; with the lighter guards (`NullMoveDesperationMargin=0`, `FutilitySeeMargin=20`) the baseline hits 9/29 Komodo moves, the relaxed configuration 11/29, reaffirming that pruning tweaks help only marginally while evaluation still drives most deltas.
- `docs/project_docs/telemetry/queen_sack/tactical_queen-sack_2025-10-06_19-38-59.csv` – latest 200 ms queen-sack telemetry after search gating; highlights residual evaluation optimism on the remaining 16 motifs.
- `docs/project_docs/telemetry/queen_sack/tactical_queen-sack_2025-10-09_21-02-16.csv` – QS3 king-danger prototype still solves 4/20 motifs (PV hits 6/20); compare against QS1/QS2 baselines to isolate evaluation-only shifts.
- `docs/project_docs/telemetry/queen_sack/node_explosion_queen-sack_2025-10-09_21-02-37.csv` / `.log` – raw 200 ms diagnostics with per-position info-line counts (25–37) confirming the new penalties add evaluation pressure without inflating node counts.
- `docs/project_docs/telemetry/queen_sack/tactical_queen-sack_2025-10-09_21-09-48.csv` – QS3 tuned weights raise coverage to 5/20 (PV hits 7/20) while keeping failure set consistent for further tuning.
- `docs/project_docs/telemetry/queen_sack/node_explosion_queen-sack_2025-10-09_21-10-06.csv` / `.log` – node traces for the tuned configuration; use alongside earlier runs to spot per-motif search pressure shifts.

## Reporting
- Update this index whenever new tools or datasets are added.
- Summarise major evaluation fixes and their measured impact (bench, SPRT, eval deltas).
- Once the bias curve flattens, prepare a merge plan for `feature/20251009-eval-bias` and notify the move-picking effort to rebase.
- **Next Check-in Prep**: Before pausing, drop the latest commit SHA, bench count, telemetry directory, and outstanding tasks into this index so resumption is frictionless. Current head: `fd569d9` (`bench 2501279`), telemetry pending.
