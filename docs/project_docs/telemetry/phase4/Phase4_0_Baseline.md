# Phase 4.0 – Baseline Telemetry (2025-09-17)

## Test Environment
- Build: Release (`./build.sh Release`) – `bin/seajay` current head (`feature/20250917-phase4-selective-tuning`)
- Threads: 1 (engine default)
- Hash: 128 MB (engine default)
- UCI toggles: defaults (no selective-search changes enabled)
- Host: Codex CLI sandbox (AMD EPYC class); run timestamps 2025-09-17 ~16:10 CDT

## Depth vs Time (1s / position)
Source: `depth_vs_time_phase4_baseline.csv`

| FEN | Depth | Nodes (×10⁶) | TT Hit % | Notes |
| --- | --- | --- | --- | --- |
| `startpos` | 14 | 0.612 | 44.7 | Baseline opening depth | 
| `8/4bk1p/...` (Position 1) | 15 | 0.812 | 39.9 | Still re-searching defensive resources |
| `r1bq1rk1/...` (Position 2) | 17 | 0.619 | 52.1 | Depth parity point of reference |
| `r1b1k2r/...` | 12 | 0.411 | 37.8 | Known middlegame pruning stress |
| `8/2p5/...` | 16 | 1.237 | 48.8 | Endgame still heavy on nodes |

Takeaways
- Depth vs time is broadly unchanged from pre-Phase-4 reports: middlegame/endgame cases still saturate around 0.4–1.2M nodes at 1s.
- TT hit rates vary 38–52%; higher in positional FEN #3 but regression in defensive FEN #4.

## Node Explosion Diagnostic
Source: `node_explosion_phase4_baseline_{report,csv}` using `tools/node_explosion_diagnostic.sh` (depths 5/7/10).

- Average node ratio vs Stash: **6.47×**; worst cases >15× (endgame depth 7, 13.5× at depth 10).
- Average node ratio vs Komodo: **1.82×** (SeaJay still wider tree in all test FENs).
- Laser binary unavailable in sandbox (`ratio N/A`).
- Consistent hotspots:
  - Endgame FEN: SeaJay uses 13–16× nodes vs Stash at depths 7–10.
  - Middlegame FEN: >10× at depth 7, still 7.9× at depth 10.
  - Complex FEN: 3.7× at depth 10 even against Stash (1.08× vs Komodo).

## Tactical Suite (WAC, 500 ms/move)
Source: `tactical_suite_phase4_baseline_{baseline,failures}.csv` via `tools/tactical_test.py`.

- Score: **235 / 300 (78.3%)** – barely below the 80% target.
- Average depth: 14.4; average nodes 317k per position; total nodes 95M.
- 65 failures spread across pin/queen sac themes (WAC.001, 014, 021, 049, 263, 288, etc.).
- Several misses cluster around queen sac motifs, matching prior selective-search gating concerns.

## Summary
- Baseline telemetry confirms selective-search debt remains in middlegame/endgame pruning (node ratios >6×).
- Tactical pass rate slips under the 80% bar, giving us a quantitative guard for Phase 4.1 gating changes.
- Depth-vs-time remains the reference grid for verifying gains post-selective tuning.
- All raw artefacts stored under `docs/project_docs/telemetry/phase4/` for reproducibility.
