# TT Coverage (Post-SPSA defaults)

Commit: b8c51c9 (AspirationWindow=9, AspirationMaxAttempts=6, StabilityThreshold=5)

## WAC.049 (Depth 10, ordered picker)
- Log: `logs/tt_probe/wac049_after_tune.log`
- Root best move sequence remains stable on `h5h6` after depth 5.
- NonPV coverage stays at ≥40% through ply 9 (`Coverage NonPV`: 194/544 at ply 9 → 35.7%, first <40%).
- PV coverage remains 100% through entire horizon.

## WAC.002 (Depth 10, ordered picker)
- Log: `logs/tt_probe/wac002_after_tune.log`
- Root best move transitions `f6e6 → c4c3 → b3b6` as expected; guard handles oscillation.
- NonPV coverage keeps ≥40% through ply 9 (`Coverage NonPV`: 1121/3097 at ply 9 → 36.2%).

## Notes
- SEE-off and aspiration-off control runs from earlier harness still applicable for comparison (`*_seeoff_ttcoverage.log`, `*_noasp_ttcoverage.log`).
- Latest defaults appear to maintain the coverage improvements achieved with the aspiration guard.
