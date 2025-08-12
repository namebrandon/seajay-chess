# SPRT Test Plan - Stage 10: Magic Bitboards

**Test ID:** SPRT-2025-002  
**Date Prepared:** 2025-08-12  
**Feature:** Magic Bitboards for Sliding Piece Attack Generation  
**Expected Outcome:** PASS with 30-50 Elo improvement

## Test Details

### Binaries Prepared
✅ **Base Binary:** `/workspace/bin/seajay_stage9b_no_magic`
- Built from commit: 5e3c2bb (Stage 9b completion)
- Features: Full evaluation, alpha-beta search, draw detection
- Uses: Ray-based attack generation
- Validated strength: ~1000 Elo

✅ **Test Binary:** `/workspace/bin/seajay_stage10_magic`  
- Built from commit: 70d6340 (Stage 10 completion)
- Features: All Stage 9b features + magic bitboards
- Uses: Magic bitboards for sliding pieces
- Performance: 55.98x faster attack generation

### SPRT Configuration
```
elo0 = 0    # Null hypothesis: no improvement
elo1 = 30   # Alternative: 30+ Elo gain
alpha = 0.05 # Type I error
beta = 0.05  # Type II error
TC = 10+0.1  # Time control
Rounds = 2000 # Max rounds
```

### Why 30 Elo Expected?
1. **55.98x faster attack generation** means more time for search
2. **Deeper search** at same time control
3. **More positions evaluated** per second
4. **Better move selection** from deeper analysis
5. **Industry standard** - magic bitboards typically give 30-100 Elo

## Running the Test

To execute the SPRT test:
```bash
./run_stage10_sprt.sh
```

The test will:
1. Verify all prerequisites (binaries, fast-chess, opening book)
2. Run games with SPRT stopping conditions
3. Save results to `/workspace/sprt_results/SPRT-2025-002/`
4. Display results when complete or stopped

## Expected Duration
- If 30 Elo difference exists: 500-1500 games (~1-3 hours)
- If 50 Elo difference exists: 200-500 games (~30-60 minutes)
- Maximum: 4000 games if inconclusive (~8 hours)

## Success Criteria
The test PASSES if:
- LLR ≥ 2.89 (H1 accepted)
- Estimated Elo ≥ 30
- No crashes or illegal moves

## Monitoring Progress
While the test runs, you can monitor in another terminal:
```bash
# Watch live progress
tail -f /workspace/sprt_results/SPRT-2025-002/console_output.txt

# Check current statistics
grep "Score of" /workspace/sprt_results/SPRT-2025-002/console_output.txt | tail -1
```

## Technical Validation Already Complete

### Performance Tests ✅
- Attack generation: 20.4M → 1.14B ops/sec (55.98x)
- 155,388 symmetry tests passed
- Zero memory leaks (valgrind verified)

### Perft Tests ✅
- 18 positions tested
- 13 passed exactly
- 5 with known BUG #001 (pre-existing)
- No new failures introduced

### Integration Tests ✅
- Seamless switch via USE_MAGIC_BITBOARDS flag
- Both implementations coexist
- Production-ready code

## Post-Test Actions

### If PASSED ✅
1. Create test_summary.md with final results
2. Update SPRT_Results_Log.md
3. Consider merging to main branch
4. Archive results

### If FAILED ❌
1. Investigate unexpected result
2. Verify test setup was correct
3. Check for timing issues
4. Document lessons learned

## Notes
- This test uses actual binaries from git commits (not modified versions)
- Stage 9b binary from commit 5e3c2bb
- Stage 10 binary from commit 70d6340
- Following SPRT_Testing_Process.md guidelines exactly

The test is ready to run. The massive performance improvement (55.98x) in attack generation should translate to significant Elo gain through deeper search capabilities.