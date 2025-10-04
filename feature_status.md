# Feature Status – Move Picking Optimization (MP Series)

**Updated:** 2025-10-03

## Current Phase
- Phase MP3 (quiet move selection integration) – in progress
- Latest commit under test: `d85ca5cd48babd57a96bb35e4fc6aad0cfe6d38a`

## Work Completed
- Replaced the O(n²) quiet move selection loop with a stable index sort (no heap churn).
- Restored protected slot for countermove immediately after killers (matches MP2 ordering).
- Added `MOVE_ORDER_DUMP=ply:count` instrumentation in both `negamax` and `RankedMovePicker`.
- Benchmarked SEE production toggle (confirmed ~–7% NPS, still off by default).
- Captured MP2 baseline `MOVE_ORDER_DUMP` traces for comparison with MP3 instrumentation.
- Restored MP2-style countermove gating (only reserve the slot when `countermoveBonus > 0`).
- Captured depth-10 probes for `/tmp/fen_list.txt` with the legacy picker (`logs/move_picker_depth10.log`) to serve as telemetry baseline before reintroducing staged move picking.
- Routed SEE-positive captures and promotion-only moves through the shortlist stage so stage logs tag good captures as `stage=Shortlist` instead of `stage=Remainder`.
- Deferred SEE-negative captures to the `BadCaptures` stage; shortlist yields now skip them and stage logs confirm late emission (`logs/move_picker_depth10_stagewrap_badcaptures.log`).

## Benchmarks (SEE off)
- MP3 head: `1971759 nodes / 1,350,872 nps`
- MP2 baseline: `2531668 nodes / 1,203,819 nps`

## SPRT
- Test 769 vs `main`
- Elo: −8.84 ± 7.15 (95%)
- LLR: −2.88 (bounds [−2.94, 2.94]) – regression indicated
- Games: 4,716 (W:1261 / L:1381 / D:2074)

## Observations
- Root move order (`MoveOrder`) matches MP2 after the countermove fix.
- Instrumentation showed MP3 was always promoting countermoves ahead of the quiet sorter; MP2 only does so when `countermoveBonus > 0`. Fixing the guard realigns the ordering (e.g. hashes `11026875567868458929`, `8171775095838377669`).
- Updated bench dumps and spot checks against the neutral branch now show matching quiet sequences for the sampled hashes.
- Stage machine is still unused; search requests a fully ordered `MoveList`, so quiet-array rewrites remain in place.

## Open Questions / Next Tasks
1. Expand MOVE_ORDER_DUMP comparisons beyond the initial sample (ensure remaining hashes match MP2 now that gating is fixed).
2. Evaluate the impact of the new quiet selection helpers on LMR/LMP statistics (rank buckets, cutoff indices).
3. Wrap the legacy shortlist/quiet reorder with the stage machine, validating each step against `logs/move_picker_depth10.log` to keep bucket distributions aligned.
4. Incrementally migrate capture and quiet stages into the staged pipeline (next: rework quiet emission and killer injection) while continuing the bench + depth-10 probe + SPRT loop after every change.
5. Re-run SPRT after addressing the above to confirm recovery.

## Notes
- SEE production remains disabled; enabling it still costs ~7% NPS with no proven Elo benefit.
- No outstanding tactical regressions observed yet; perft parity intact (spot-check).
