# External Stage 15 Validation Tests

## Purpose
Validate whether Stage 15's SEE implementation provides real playing strength improvement by testing against an external reference engine (Stash v10, 1620 Elo).

## Rationale
Internal SPRT testing between Stage 14 and Stage 15 showed contradictory results:
- **From startpos:** Stage 15 +29 Elo (H1 accepted)
- **With opening book:** Stage 15 -4 Elo (H0 accepted)

This split result raises questions about whether SEE is providing real value or just affecting internal dynamics between our versions.

## Test Design
Each stage will play 2000 games against Stash v10 in two conditions:
1. With 4moves opening book
2. From starting position

If SEE provides real improvement, Stage 15 should outperform Stage 14 against the same external opponent.

## Test Scripts

### With Opening Book
- `stage14_vs_stash_v10_book.sh` - Stage 14 baseline with book
- `stage15_vs_stash_v10_book.sh` - Stage 15 test with book

### From Starting Position
- `stage14_vs_stash_v10_startpos.sh` - Stage 14 baseline from startpos
- `stage15_vs_stash_v10_startpos.sh` - Stage 15 test from startpos

## Running Tests
Due to system limitations, only 2 tests can run simultaneously:
```bash
# Run both Stage 14 tests first (baseline)
./stage14_vs_stash_v10_book.sh &
./stage14_vs_stash_v10_startpos.sh &

# After completion, run Stage 15 tests
./stage15_vs_stash_v10_book.sh &
./stage15_vs_stash_v10_startpos.sh &
```

## Expected Outcomes

### Success Scenario
Stage 15 shows consistent improvement over Stage 14:
- Both conditions show positive Elo difference
- Improvement magnitude similar to internal testing

### Failure Scenario
Stage 15 performs similarly to Stage 14:
- No significant Elo difference in either condition
- Suggests SEE not providing real strength

### Mixed Scenario
Improvement only in specific conditions:
- May indicate position-dependent benefit
- Could suggest tuning issues

## Analysis
After all tests complete, compare:
1. Stage 14 vs Stash scores
2. Stage 15 vs Stash scores
3. Calculate relative improvement
4. Determine if SEE provides real value

## Reference Engine: Stash v10
- **Elo:** 1620 (calibrated)
- **Path:** `/workspace/external/engines/stash-bot/v10/stash-10.0-linux-x86_64`
- **Why chosen:** Close to our strength range, well-calibrated, no shared bugs

## Time Control
All tests use 10+0.1 (10 seconds + 0.1 increment) for consistency with previous SPRT testing.

## Binary Checksums
- **Stage 14 Final:** 1c65de0c2e95cb371e0d637368f4d60d
- **Stage 15 Final (Tuned):** 1364e4c6b35e19d71b07882e9fd08424

## Notes
- Tests will take approximately 3-4 hours each
- Results saved in timestamped subdirectories
- Use `tail -20 [dir]/fastchess.log` to monitor progress
- Final Elo calculations will have ±10 error bars with 2000 games