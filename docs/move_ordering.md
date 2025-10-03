# Move Ordering Diagnostic Guide

## Key Invariants

- **Countermove gating:** Prefetching the countermove into the quiet prefix only happens when `countermoveBonus > 0`. If the bonus is zero we leave the countermove in-place and let history scoring handle it.
- **Killers first:** Both killer slots are attempted (slot 0 then slot 1) before inserting the countermove. Stale killers are ignored if their piece no longer exists or belongs to the wrong side.
- **History/CMH selection:**
  - Depth `< HISTORY_GATING_DEPTH` drops to the “basic” history path (killers + counter move, no CMH weighting).
  - Depth ≥ `HISTORY_GATING_DEPTH` and CMH availability uses the CMH-weighted sorter (requires `countermoveBonus`, `history`, `counterMoves`, and `counterMoveHistory`).
- **Perft parity:** Perft and tactical correctness should be untouched; changes must only affect move ordering.

## Instrumentation

### Environment Variables

- `MOVE_ORDER_DUMP=<ply>:<count>`
  - Emits `info string MoveOrder`/`PickerOrder` lines for up to `<count>` unique positions at plies ≤ `<ply>`.
  - Example: `env MOVE_ORDER_DUMP=4:128 ./bin/seajay <<<'uci\nsetoption name Threads value 1\nbench\nquit\n'`

- `MOVE_ORDER_DEBUG_HASH=<hash[,hash...]>`
  - Enables detailed gate/score logging for specific zobrist keys. Defaults to `11026875567868458929` if unset.
  - Use to inspect quiet gating decisions (`QuietGate`), buffer scores (`QuietHistory scores` / `QuietBasic scores`), and final ordering (`QuietHistory ordered`).
  - Example: `env MOVE_ORDER_DEBUG_HASH=11026875567868458929 MOVE_ORDER_DUMP=4:128 ./bin/seajay …`

### Baseline Hashes (MP2 parity checks)

| Hash | Scenario | Notes |
|------|----------|-------|
| `11026875567868458929` | After `f3f6` in bench position 1 | High-history quiets should start `e8d8 a8b8 b6c4 …` |
| `8171775095838377669` | Ply 3 deeper in same bench suite | Countermove is `a8b8`; bonus 0 means history order should lead with `e8d8` |
| `11833121023070133230` | Ply 4 queue, CMH path | `countermoveBonus` zero: `h1g1` must stay in history order |

Compare the `PickerOrder` lines against MP2 for these hashes after any code change.

## Testing Workflow

1. **Build**
   ```bash
   ./build.sh Release
   ```
2. **Run bench dumps**
   ```bash
   env MOVE_ORDER_DUMP=4:128 ./bin/seajay <<'EOF'
   uci
   setoption name Threads value 1
   bench
   quit
   EOF
   ```
3. **Check key hashes**
   ```bash
   rg "11026875567868458929" bench_output.txt
   rg "8171775095838377669" bench_output.txt
   rg "11833121023070133230" bench_output.txt
   ```
4. **Detailed debugging (optional)**
   ```bash
   env MOVE_ORDER_DEBUG_HASH=11026875567868458929,8171775095838377669 \
       MOVE_ORDER_DUMP=4:128 ./bin/seajay <<'EOF'
   uci
   setoption name Threads value 1
   bench
   quit
   EOF
   ```
   Inspect `QuietGate`, `Quiet* scores`, and `Quiet* ordered` lines.

5. **Compare against MP2**
   Keep a reference dump from the neutral commit (e.g. `83f6845`) and diff `PickerOrder` lines for the hashes above.

6. **Bench summary**
   Include the bench line in commits: `Total: <nodes> nodes in …`.

## Tips

- When a divergence appears, check:
  1. Has the countermove bonus changed? (If zero, it must not be moved to the front.)
  2. Are killer slots stale? The debug logs show whether they were inserted.
  3. Did we fall back to the basic path because depth < `HISTORY_GATING_DEPTH` or CMH data is missing?
- Add new “golden hash” rows as additional benchmark positions are introduced.
