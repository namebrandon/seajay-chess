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
- Added move ordering telemetry (`firstCutoff` buckets, TT availability) and the `UseUnorderedMovePicker` diagnostic toggle (see `docs/project_docs/move_ordering_telemetry.md`).

## Benchmarks (SEE off)
- MP3 head: `2441603 nodes / 1,310,720 nps`
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
3. Plan staged picker integration (fully incremental `NextMove()` pipeline) to eliminate bulk quiet reorder.
4. Re-run SPRT after addressing the above to confirm recovery.

## Notes
- SEE production remains disabled; enabling it still costs ~7% NPS with no proven Elo benefit.
- No outstanding tactical regressions observed yet; perft parity intact (spot-check).
- Added `MOVE_ORDER_DUMP=ply:count` instrumentation in both `negamax` and `RankedMovePicker`.
- Benchmarked SEE production toggle (confirmed ~–7% NPS, still off by default).
- Captured MP2 baseline `MOVE_ORDER_DUMP` traces for comparison with MP3 instrumentation.
- Restored MP2-style countermove gating (only reserve the slot when `countermoveBonus > 0`).
- Added move ordering telemetry (`firstCutoff` buckets, TT availability) and the `UseUnorderedMovePicker` diagnostic toggle (see `docs/project_docs/move_ordering_telemetry.md`).
