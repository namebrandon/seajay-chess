# Phase P4 – Passed Pawn Scaling Enhancements

## Motivation
Harness comparisons against Komodo highlight large evaluation gaps in positions where:
- Connected or advanced enemy passers roll down the board with rook/queen support.
- Our king distance terms are too weak (SeaJay often scores near 0 while Komodo awards +200–400 cp).
- Blocked passers or those lacking support remain overestimated relative to references.

## Reference Model
Laser chess engine (src/eval.cpp ~L680) introduces several heuristics we can adapt:
1. **Non-linear rank scaling:** bonuses grow quadratically with advancement.
2. **Path-to-queen analysis:** rewards for free paths or fully defended promotion squares.
3. **Rook/queen behind support:** x-ray attacks behind the passer increase defender set.
4. **King distance adjustments:** subtract own king distance, add opponent king distance.
5. **File-based bonuses:** central passers receive slightly higher base values.

## SeaJay Approach
- Introduce a toggle `EvalPasserPhaseP4` (default off) to gate new scoring.
- Extend pawn cache or runtime evaluation to gather needed signals (path occupancy, support, king distance).
- Maintain existing bonuses (protected/connected/unstoppable) but rebalance around the new model.
- Emit additional telemetry fields when tracing (e.g., pathFree, rookSupport, kingDist terms) to validate behaviour.

## Implementation Plan
1. **Data Prep**
   - Compute promotion path bitboards (forward rays) for each passer.
   - Determine block masks (occupied squares, opponent attacks) and support masks (own attacks, rook/queen behind).
2. **Scoring Components**
   - Replace linear `PASSED_PAWN_BONUS` usage with a rank lookup tuned for non-linear growth (table or polynomial).
   - Add bonuses for:
     - Path completely free of opposing pieces/attacks.
     - Stop square defended by own pieces.
     - Friendly rook/queen x-rayed behind the passer.
   - Add penalties if opponent controls the path/stop square.
   - Apply king-distance modifiers using Chebyshev distance to promotion square.
3. **Phase Scaling**
   - Keep existing phase multiplier (50/75/150%) but adjust coefficients to avoid double-counting in endgames.
4. **Telemetry**
   - Update `EvalTrace::PassedPawnDetail` with booleans for `pathFree`, `pathDefended`, `rookSupport`, and numeric king-distance values for the most advanced passer on each side.
5. **Validation Workflow**
   - Harness summary must show reductions in |Δ| for positions 2,3,7,8,10,13 of `eval_pawn_focus` at 100 ms.
   - Standard `bench` to confirm performance is within noise.
   - Document results and decision on toggling to default.

## Current Baseline (2025-09-28)
- Toggle `EvalPasserPhaseP4=false` continues to show large gaps on eval pack positions #10 & #13 (SeaJay down ≥350 cp versus Komodo).
- Toggle `EvalPasserPhaseP4=true` still overshoots the same positions; latest OpenBench runs (tests 692 & 694) report ≈ −40 nELO despite endgame-focused books, confirming the new passer model hurts match play at current settings.
- Bench (Release) with toggle off: 2 371 156 nodes @ 1.59 MNPS. Toggle on: 2 645 416 nodes @ 1.74 MNPS when instrumentation was active during profiling, but OpenBench indicates an effective slowdown once full search overhead is considered (≈ −6 % versus main).
- Attack profiling shows the new path logic drives ~26 % more `isSquareAttacked` probes per side (≈ +735k white / +727k black queries over 13 eval-pack FENs), explaining the observed NPS loss.
- All new hooks remain default-off so mainline play is unaffected while we iterate.

## Risks
- Increased computation per passer could reduce NPS; optimize by reusing cached masks where possible.
- Overcompensation leading to inflated scores; keep toggle off until OpenBench confirmation.
