# Move Ordering Telemetry & Diagnostics (2025-10-04)

## Purpose

Capture the new instrumentation and diagnostic toggles added while investigating first-move fail-high rates and overall move ordering health. This document serves as the reference for running the telemetry, comparing ordered vs unordered searches, and interpreting the collected metrics.

## Added Telemetry (Release 2025-10-04)

### `MovePickerStats` Extensions (requires `SearchStats` build)

When `ShowMovePickerStats=true`, the engine now prints:

```
info string MovePickerStats: bestMoveRank [1]=… [2-5]=… [6-10]=… [11+]=… shortlistHits=… SEE(lazy): calls=… captures=…
                       ttFirstYield=… remainderYields=…
                       firstCutoffs=<count> firstCutoffTT=<used>/<avail> (XX.X%) firstBuckets … buckets …
```

New fields:

- `firstCutoffs`: how many fail-highs occurred on the first yielded move.
- `firstCutoffTT=<used>/<avail>`: TT availability for those first cutoffs, and how often the TT move was the decisive one.
- `firstBuckets`: bucket distribution (TT / GoodCapture / Killer / Quiet…) of the *first* cutoffs.
- `buckets`: overall distribution across all cutoffs.

These metrics are populated in both the primary and legacy negamax paths. Classification logic lives in `classifyCutoffMove` (`src/search/negamax.cpp`), which mirrors legacy heuristics (TT > good/bad captures via SEE > killers > counter moves > quiet buckets).

### First-Cutoff Tracking in Search

During beta cutoffs (`score >= beta`), we now record:

- Yield rank (existing telemetry) → `bestMoveRank` buckets.
- Whether the move came from the shortlist (`shortlistHits`).
- Bucket classification (TT/capture/killer/etc.).
- The first cutoff per node (to isolate the initial ranking quality).

See `src/search/negamax.cpp` and `src/search/negamax_legacy.inc` additions around the beta-cutoff block.

## Unordered Move Picker Toggle

A new UCI option is available:

```
setoption name UseUnorderedMovePicker value true
```

Behaviour:

- Disables the ranked picker construction.
- Skips `orderMoves(...)` in both modern and legacy searchers, leaving moves in generator order.
- Keeps all other heuristics intact so we can measure the raw impact of ordering on node counts.

The toggle threads through `SearchLimits::useUnorderedMovePicker` (default `false`), so scripted comparisons can simply flip the UCI option and re-run the same depth/benchmark sequence.

## Suggested Workflow

1. **Baseline (ordered)**
   ```
   setoption name UseRankedMovePicker value true
   setoption name UseUnorderedMovePicker value false
   setoption name SearchStats value true
   setoption name ShowMovePickerStats value true
   position fen <FEN>
   go depth <N>
   ```
   Collect `MovePickerStats`, `MoveOrdering` lines, and total node count.

2. **Unordered reference**
   ```
   setoption name UseUnorderedMovePicker value true
   go depth <N>
   ```
   Compare total nodes (goal: 40–60 % reduction when ordering is enabled), and note the first cutoffs collapsing to <50 % in unordered mode.

3. **Interpretation Tips**
   - `firstCutoffTT=<used>/<avail>` below ~70 % suggests TT misses or TT move displacement.
   - High `firstBuckets Killer/Quiet` values indicate quiet moves slipping ahead of captures or TT.
   - `buckets BadCapture` spikes prompt investigation into SEE thresholds or stage transitions.

## Cross References

- Link back to `Move_Picking_Optimization_Plan.md` (add to “Resumption Strategy” section).
- Mention in `feature_status.md` under current open tasks.

## Next Steps

1. Scripted harness to aggregate the new metrics across `/tmp/fen_list.txt` and deeper search nodes.
2. Investigate TT availability vs. usage, particularly where first bucket ≠ TT.
3. Explore retuning shortlist / killer sequencing based on the telemetry.
4. Leverage `UseUnorderedMovePicker` in benchmarks to quantify node-reduction percentages for future PRs.

