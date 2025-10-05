# Selectivity Bound Experiments (2025-10-09)

## FEN A — `r1b1k2r/p2n1p2/5np1/3p3p/qBp1pP1P/2P1P1N1/P1PQB1P1/1R3RK1 w kq - 0 18`
- Komodo: `g3e4` (queen sac) ≈ +179 cp (depth 18).
- SeaJay baseline (all heuristics on): `bestmove b1b2`, `score cp ≈ -36`.
- Incremental toggles (logs: `selectivity_bounds_g3e4.txt`):
  1. **LMR disabled** → still `b1b2` (neg score).
  2. **LMR + SEE/QSEE off** → still `b1b2`.
  3. **+ Null move off** → still `b1b2`.
  4. **+ Futility & UseFutility off** → `bestmove g3e4`, `score cp +39` (remains lower than Komodo but positive).
- Minimum viable set observed: {LMR disabled, SEE/QSEE off, Null move off, Futility off}. Any subset leaves the sac buried. Suggest tightening reductions around checking captures rather than blanket disable (these toggles massively slow the search).

## FEN B — `2r3k1/2p1n1b1/p3p1pp/3bP2q/5P2/1P2B3/P4QPP/2RR2K1 w - - 0 25`
- Komodo: `h3` (depth 18) ≈ −124 cp for black.
- SeaJay baseline: `bestmove e3c5` (keeps bishop). Score floats +13…+53 cp.
- Heuristic toggles (same sequence) never promote `h3`; even with LMR/SEE/Null/Futility disabled the engine sticks to `e3c5`. Indicates evaluation bias dominates rather than pruning.

## FEN C — `6k1/p1p2pp1/2b4r/b1p2P2/2P3Pq/1P2N3/P3KR2/2QR4 b - - 0 37`
- Already matches Komodo’s `h4g3`, yet scores remain ~−100 cp vs Komodo’s +something (see `fen_6k1_default.txt`). Treated as pure evaluation issue.

### Next Steps
1. Prototype targeted reductions for checking captures/SEE-positive trades instead of global heuristics-off; recreate the above table once a candidate change exists.
2. For FEN B, run EvalExtended to see which king-safety/slider terms mis-score `h3`. Use Komodo delta to guide tuning.

### Automated Probe Snapshot
- Script: `tools/selectivity_probe.py` (movetime 2000 ms per run)
- Dataset: 29 FENs from `docs/issues/eval_bias_tracker.json` (prefers depth-18 Komodo references)
- Baseline matches (all heuristics on): **9 / 29**
- Relaxed matches (LMR/SEE/null/futility disabled): **11 / 29**
- Interpretation: turning off the major pruning heuristics recovers only two additional Komodo-aligned moves, while score deltas remain large. Evaluation drift is the dominant factor across this sample, with pruning contributing in a smaller subset (notably FEN A).
