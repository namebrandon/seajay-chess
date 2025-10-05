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
- Largest gaps continue to map to queen/slider pressure and exposed-king motifs (`docs/issues/eval_bias_tracker.md`).
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
1. K-factor scaling for exposed kings on open files (positions `4r1k1/b1p3pp/...` and `8/p2r1k1p/...` remain outliers at both depths).
2. Slider pressure on undefended backbone squares (`2r3k1/2p1n1b1/...` cases show optimistic SeaJay scores).
3. Queen mobility penalties when trapped behind own pieces (`2r3k1/Qpb1qp1p/...` delta +228 cp needs investigation).
4. Interaction between PST bonuses and piece coordination—evaluate whether PST weights remain tuned after recent aspiration defaults.

## Reporting
- Update this index whenever new tools or datasets are added.
- Summarise major evaluation fixes and their measured impact (bench, SPRT, eval deltas).
- Once the bias curve flattens, prepare a merge plan for `feature/20251009-eval-bias` and notify the move-picking effort to rebase.
