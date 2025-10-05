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

## Current Findings
- 2025-10-09: Depth-18 rerun completed alongside the original depth-14 sweep. Deviations persist (many outliers still ±150–300 cp) but depth-18 scores often diverge further, confirming root-cause issues in static evaluation and king-safety scaling.
- Instrumented queen-sacrifice case (`r1b1k2r/p2n1p2/...`) shows the reference move `g3e4` generated every iteration yet collapsing under pruning; once LMR/null/futility/SEE gates are disabled the engine plays `g3e4`, proving the move exists but is suppressed by selectivity (logs in `docs/project_docs/telemetry/eval_bias/fen_r1b1k2r_*.txt`).
- Second outlier (`6k1/p1p2pp1/...`) already chooses Komodo’s `h4g3` yet scores ≈−100 cp, underscoring the evaluation bias component (`docs/project_docs/telemetry/eval_bias/fen_6k1_default.txt`).
- TT telemetry remains healthy; the −44 nELO regression observed in MP3 is still attributed primarily to evaluation optimism.

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
1. Triage selectivity settings (LMR/null/futility/SEE) on sacrificial motifs to prevent over-reduction while keeping overall pruning effective.
2. K-factor scaling for exposed kings on open files (positions `4r1k1/b1p3pp/...` and `8/p2r1k1p/...` remain outliers at both depths).
3. Slider pressure on undefended backbone squares (`2r3k1/2p1n1b1/...` cases show optimistic SeaJay scores).
4. Queen mobility penalties when trapped behind own pieces (`2r3k1/Qpb1qp1p/...` delta +228 cp needs investigation).
5. Interaction between PST bonuses and piece coordination—evaluate whether PST weights remain tuned after recent aspiration defaults.

## Telemetry Artifacts (2025-10-09)
- `docs/project_docs/telemetry/eval_bias/fen_r1b1k2r_default.txt` – baseline search trace for the Komodo queen-sacrifice FEN highlighting pruning pressure on `g3e4`.
- `docs/project_docs/telemetry/eval_bias/fen_r1b1k2r_heuristics_off.txt` – LMR/SEE-only disable; move remains suppressed until deeper pruning toggles are removed.
- `docs/project_docs/telemetry/eval_bias/fen_r1b1k2r_selectivity_off.txt` – null and futility disabled alongside LMR/SEE, yielding `g3e4` despite negative eval.
- `docs/project_docs/telemetry/eval_bias/fen_r1b1k2r_legal_moves.txt` – python-chess dump confirming `g3e4` generation (38 legal moves).
- `docs/project_docs/telemetry/eval_bias/fen_6k1_default.txt` – exposed-king FEN where move selection matches Komodo yet eval lags behind.

## Reporting
- Update this index whenever new tools or datasets are added.
- Summarise major evaluation fixes and their measured impact (bench, SPRT, eval deltas).
- Once the bias curve flattens, prepare a merge plan for `feature/20251009-eval-bias` and notify the move-picking effort to rebase.
