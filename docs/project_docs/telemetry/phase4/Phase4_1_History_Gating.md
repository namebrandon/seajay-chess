# Phase 4.1 – History / Countermove Gating Relaxation (2025-09-17)

## Change Summary
- Reduced counter-move history (CMH) activation threshold from depth ≥6 to ≥2 for both legacy ordering and the ranked move picker.
- Added `SearchData::historyStats` telemetry, tracked per ply and surfaced through the `SearchStats` info string as `hist(...)`.
- History context now records basic-history usage vs. CMH usage, first-move hits, beta cutoffs, and re-search counts.

Example (`go depth 6` from startpos with `SearchStats` enabled):
```
info string SearchStats: ... hist(apps=970,basic=691,cmh=279,first=549+115,cuts=33+21,re=148)
```
This run shows CMH applied on 279 nodes (vs. 0 previously at depth <6), with 115 first-move hits driven by CMH and 21 CMH-induced beta cutoffs.

## Depth vs Time (1s / position)
Source: `depth_vs_time_phase4_1.csv`

| FEN | Baseline Depth | Phase 4.1 Depth | Baseline Nodes (×10⁶) | Phase 4.1 Nodes (×10⁶) | Notes |
| --- | --- | --- | --- | --- | --- |
| `startpos` | 14 | 14 | 0.612 | 0.612 | No change |
| `8/4bk1p/...` | 15 | 15 | 0.812 | 0.812 | No change |
| `r1bq1rk1/...` | 17 | 17 | 0.619 | 0.619 | No change |
| `r1b1k2r/...` | 12 | 12 | 0.411 | 0.411 | No change |
| `8/2p5/...` | 16 | 16 | 1.237 | 1.225 | ~1% fewer nodes |

## Node Explosion Diagnostic
Source: `node_explosion_phase4_1_{report,csv}` (depths 5/7/10).

- Average node ratios unchanged (6.47× vs Stash, 1.82× vs Komodo).
- Worst cases remain the endgame FEN (15.6× at depth 7). CMH gating alone does not reduce breadth yet.

## Tactical Suite (WAC, 500 ms/move)
Source: `tactical_suite_phase4_1_{csv,failures}.`

- Score: **236 / 300 (78.7%)** – +0.4 pp vs baseline but still below the 80% target.
- Total nodes: 97.3 M (↑ ~2.2 M from baseline), average depth 14.5.
- Failure set still dominated by queen-sac/forcing motifs (WAC.001/014/021/030/049/263/etc.).

## Instrumentation Notes
- `SearchStats` now reports history metrics: total applications (`apps`), basic vs CMH splits, first-move hits (`first`), beta cutoffs (`cuts`), and re-searches (`re`). These feed the Phase 4.1 requirement to correlate history hits with PVS re-search rate.
- `NodeExplosionStats` records quiet cutoffs attributed to CMH vs. basic history via the new context markers.
- `SearchData` exposes `historyStats` for deeper analysis or CSV export if required.

## Observations
- CMH now engages at shallow depths (apps: 279 at depth 6 from startpos), but aggregate depth-vs-time and node explosion numbers remain flat; follow-up work should monitor whether later sub-phases exploit the wider gating to reduce nodes.
- Tactical pass rate improved slightly but not enough to clear the 80% gate; watch for regressions when stacking future Phase 4 changes.
