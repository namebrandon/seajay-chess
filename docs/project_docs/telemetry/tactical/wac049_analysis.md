# WAC.049 Tactical Investigation (2025-10-05)

- **FEN**: `2b3k1/4rrpp/p2p4/2pP2RQ/1pP1Pp1N/1P3P1P/1q6/6RK w - - 0 1`
- **Expected solution**: `Qxh7+` (Win at Chess catalog)
- **SeaJay 1s result**: plays `h5h6` with search score `cp = -6` (white advantage disappears)
- **Reference**: forcing `Qxh7+` in the root yields a mate-in-5 once the follow-up is searched without pruning

## Telemetry snapshot (EvalKingDangerIndex = true)

| Position | total cp (white) | King danger (black) | Notes |
|----------|------------------|---------------------|-------|
| Root (no moves) | −128 | flank=7, flank_multi=1, queen_safe=2 | Index already injects +68 cp vs baseline, but static evaluation remains deeply negative |
| After `Qxh7+` | +14 | flank=8, flank_multi=2, queen_safe=1 | The sacrifice flips the score, but only once search actually reaches the position |
| After `h5h6` | −121 | flank=7, flank_multi=2 | Chosen move keeps the king boxed but loses on material |

## DebugTrackedMoves (`h5h7`)

Running `go depth 12` with `DebugTrackedMoves = h5h7` shows the move is generated early but returns large negative scores until the search reaches **ply ≥ 9**:

```
info string DebugMove h5h7 depth=4 ... score=-1002 alpha=-24 beta=-23
info string DebugMove h5h7 depth=6 ... score=-163 alpha=-24 beta=-23
info string DebugMove h5h7 depth=9 ... score=+377 (first fail-high)
info string DebugMove h5h7 depth=12 ... score=mate 5
```

All intermediate eval traces show material deficits around `-500` centipawns (queen for pawn), so SEE-based pruning and aspiration windows keep rejecting the line until the mate appears. With the 1 s WAC budget the root never revisits the move after the first fail-low, so `h5h6` stays on top.

## Reproduction scripts

- Root evaluation: `docs/project_docs/telemetry/tactical/wac_20250930_king_danger_breakdown.csv`
- Forced line: `position ... moves h5h7 g8h7` → `go movetime 1000` (SeaJay finds `mate 5` with `g5h5` follow-up)

## Hypotheses

1. **Late positive swing**: The queen sacrifice only turns positive after ~9 plies, so a single fail-low at shallow depth is enough to bury it under history/SEE penalties.
2. **LMR/SEE interaction**: (addressed) Late-ranked checking captures now bypass the rank-based `seeGE` gate, so the move is always searched; contact checks gain a +1 extension.
3. **Move-order starvation**: `h5h6` still scores around `-6` and remains the root PV. Even with the new extension the first fail-low keeps the move out of the TT window; we need to boost re-search priority (history, counter-move seed, or TT re-try) once the capture returns a non-mating score.

## Next considerations

- ✅ Late capture SEE gate now defers pruning decisions until after legality checks and skips them for checking queen sacrifices.
- ✅ Added a 1-ply contact-check extension for adjacent-king queen captures (triggering on `Qxh7+`).
- Next: strengthen the re-search story—either elevate the move via history/TT feedback or schedule an explicit PV re-search when a contact check appears in a fail-low bucket.
- Re-run WAC.049 after tuning to confirm the engine finally locks onto `Qxh7+` without manual forcing.
