# Phase 4.2 – Null-Move Reduction Shadow Instrumentation (2025-09-17)

## Test Environment
- Build: Release (`./build.sh Release`), binary `bin/seajay` @ `feature/20250917-phase4-selective-tuning`
- Threads: 1, Hash: 128 MB
- Unless noted, UCI options left at defaults (`UseAggressiveNullMove=false`, `NullMoveEvalMargin=198`)

## SearchStats Probes (Shadow Mode)

### Default toggle (`UseAggressiveNullMove=false`)
```
info string SearchStats: depth=11 seldepth=21 nodes=81797 nps=1076276 tt: probes=23343 hits=13946 hit%=59.7 cutoffs=3155 stores=19078 coll=0 pvs: scout=53390 re%=4.7 null: att=9515 cut=4632 cut%=48.7 extra(cand=0,app=0,blk=0,sup=0,cut=0,vpass=0,vfail=0) no-store=0 static-cut=1570 static-no-store=1110 prune: fut=17523 mcp=17475 razor: att=10543 cut=245 cut%=2.3 skips(tact=2657,tt=2083,eg=0) razor_b=[215,30] asp: att=5 low=3 high=1 fut_b=[0,0,0,0] fut_eff_b=[1223,0,0,0] mcp_b=[8163,9277,35,0] illegal: first=15 total=283 hist(apps=13741,basic=8283,cmh=5458,first=5416+2710,cuts=356+541,re=2522)
```

### Forced margin experiment (`UseAggressiveNullMove=true`, `NullMoveEvalMargin=0`)
```
info string SearchStats: depth=11 seldepth=21 nodes=81797 nps=951127 tt: probes=23343 hits=13946 hit%=59.7 cutoffs=3155 stores=19078 coll=0 pvs: scout=53390 re%=4.7 null: att=9515 cut=4632 cut%=48.7 extra(cand=0,app=0,blk=0,sup=0,cut=0,vpass=0,vfail=0) no-store=0 static-cut=1570 static-no-store=1110 prune: fut=17523 mcp=17475 razor: att=10543 cut=245 cut%=2.3 skips(tact=2657,tt=2083,eg=0) razor_b=[215,30] asp: att=5 low=3 high=1 fut_b=[0,0,0,0] fut_eff_b=[1223,0,0,0] mcp_b=[8163,9277,35,0] illegal: first=15 total=283 hist(apps=13741,basic=8283,cmh=5458,first=5416+2710,cuts=356+541,re=2522)
```

The `extra(...)` counters stay at zero for startpos even with the margin pulled to zero, indicating we need sharper test positions before the new path activates.

## Depth vs Time (1s per position)

| FEN | Toggle | Depth | Nodes (×10⁶) | Notes |
| --- | --- | --- | --- | --- |
| startpos | OFF | 14 | 0.612 | Matches Phase 4.0 baseline |
| startpos | ON  | 14 | 0.612 | Identical to baseline |
| `8/4bk1p/...` | OFF | 15 | 0.812 | Defensive reference |
| `8/4bk1p/...` | ON  | 15 | **1.020** | +25.7% nodes (problematic) |
| `r1bq1rk1/...` | OFF | 17 | 0.619 | Neutral |
| `r1bq1rk1/...` | ON  | 17 | 0.618 | Neutral |
| `r1b1k2r/...` | OFF | 12 | 0.411 | Neutral |
| `r1b1k2r/...` | ON  | 12 | 0.411 | Neutral |
| `8/2p5/...` | OFF | 16 | 1.200 | Slight wobble (+0.8%) |
| `8/2p5/...` | ON  | 16 | 1.229 | +2.4% nodes |

CSV artefacts:
- `depth_vs_time_phase4_2_shadow.csv`
- `depth_vs_time_phase4_2_aggressive.csv`

## Node Explosion Diagnostic (Depths 5/7/10)
- Script: `tools/node_explosion_diagnostic.sh` (supports `--seajay-option`)
- Outputs: `node_explosion_phase4_2_shadow_{report,csv}` and `node_explosion_phase4_2_aggressive_{report,csv}`
- Result: averages unchanged vs. 4.1 (SeaJay ≈6.47× Stash, 1.82× Komodo) for both toggle states—no apparent gain or additional regression beyond the defensive FEN spike.

## Tactical Suite (WAC, 500ms/pos)
- Command: `python3 tools/tactical_test.py ./bin/seajay ./tests/positions/wacnew.epd 500 [--uci-option ...]`
- Toggle OFF: 234 / 300 (78.0%), 95.7M nodes, avg depth 14.5.
- Toggle ON: 236 / 300 (78.7%), 99.2M nodes, avg depth 14.6.
- Artefacts: `tactical_suite_phase4_2_shadow.{csv,log}`, `tactical_suite_phase4_2_shadow_failures.csv`, `tactical_suite_phase4_2_aggressive.{csv,log}`, `tactical_suite_phase4_2_aggressive_failures.csv`

## Observations
- Basic telemetry suites (startpos, defensive FENs, node explosion) leave `extra(cand/app/blk/sup/...)` at zero, so the gate stays dormant unless we feed in high-eval testcases.
- Forcing `NullMoveEvalMargin=0` with the toggle (via direct option poking) previously crashed on a synthetic queen-vs-king FEN; we could not reproduce the failure during the latest pass (the UCI option clamps to ≥100), but keep any off-range experiments guarded until we have the original reproducer.
- The defensive `8/4bk1p/...` FEN now regresses only mildly in aggressive mode (~+0.8% nodes at 1s, depth 15→16); earlier measurements showed a larger +25.7% spike, so we should keep spot-checking before OpenBench. Tactical suite improves slightly (+2 solves) but costs ~3.5M extra nodes.
- Node explosion ratios stay at 6.47× / 1.82× (Stash / Komodo) regardless of the toggle, so any benefits will need to come from positions where the aggressive path actually activates.

## High-Eval Candidate Harvest (2025-09-17 PM)
- Script: `tools/collect_aggressive_null_candidates.py` (new) scans an EPD file with `SearchStats` and records positions where `extra(cand=…)` trips. Default run against `tests/positions/wacnew.epd` at depth 10 produced `docs/project_docs/telemetry/phase4/aggressive_candidates_default.csv`.
- Two WAC positions immediately activate the new path:

| ID | FEN | Candidates | Applied | Cutoffs | Verify Passes | Score (cp) |
| --- | --- | --- | --- | --- | --- | --- |
| WAC.095 | `2r5/1r6/4pNpk/3pP1qp/8/2P1QP2/5PK1/R7 w - - 0 1` | 4 | 4 | 4 | 4 | +447 |
| WAC.113 | `rnbqkb1r/1p3ppp/5N2/1p2p1B1/2P5/8/PP2PPPP/R2QKB1R b KQkq - 0 1` | 1 | 1 | 0 | 0 | +179 |

- Depth-vs-time (1s) comparisons for these FENs show the aggressive toggle reducing nodes substantially despite identical best moves:
  - WAC.095: depth 15→14, nodes 551,039 → 369,329 (−33%).
  - WAC.113: depth 19→19, nodes 799,518 → 658,568 (−17.6%).
  (Snapshots stored as `depth_vs_time_phase4_2_candidates_{shadow,aggressive}.csv`).

- Defensive reference FEN refresh: the latest 1s runs now show only a mild regression with the toggle (depth 15→16, nodes +0.8%; artefacts `depth_vs_time_phase4_2_defensive_{shadow,aggressive}.csv`).

### Candidate Pack (20 FENs @ 1 s)
- Built `aggressive_candidates_pack.{csv,epd}` from the top 20 WAC failures ranked by `extra(cand=…)` at depth 12 (script: `tools/collect_aggressive_null_candidates.py`).
- Baseline measurements with the original gating (`null_move_pack_metrics_1s.csv`) were mixed: 12 / 20 saved nodes (avg −11.1 %), 8 / 20 regressed sharply (avg +38.2 %) for a net **+8.6 % nodes**.
- Raising only `NullMoveEvalMargin` to 240 (`null_move_pack_metrics_margin240.csv`) still averaged +2.2 % nodes and flipped WAC.095 into a +90 % regression.
- **Prototype gating (default in this branch):**
  - Added high-eval guard (staticEval ≥ 600 cp), positive-beta requirement, and a 64-application cap.
  - Updated metrics (`null_move_pack_metrics_1s_newgate.csv`): average **−0.52 % nodes**, with 9 savings (avg −4.6 %) and 6 mild regressions (avg +5.2 %). Biggest remaining outlier is WAC.225 (+19.7 %) where the aggressive path never fires—regular null behaviour dominates.
  - Defensive FEN (`8/4bk1p/...`) now benefits from the toggle (≈−28 % nodes at 1 s) while keeping counters in check (`extra(cand=152, app=38, cap=0)`).

## Next Steps
1. Extend telemetry harnesses to accept UCI overrides. ✅
2. Compile a high-evaluation FEN pack (likely from tactical wins/mates) to trigger the aggressive reduction and capture `extra(...)` activity. ✅ (`aggressive_candidates_default.csv` seeded from WAC suite at depth 10.)
3. Iterate on gating (consider phase/king-safety filters) to address residual regressions like WAC.225 (+19.7 %) and WAC.290 (+4.1 %). _Pending_
4. Decide whether Phase 4 SPRT should keep `UseAggressiveNullMove=false` (recommended) while we continue tuning the aggressive guard off-line. _Pending_
