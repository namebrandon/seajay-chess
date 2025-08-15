# Stage 15 SEE SPRT Test Configuration

## Test Overview

**Purpose:** Validate that Stage 15 Static Exchange Evaluation (SEE) implementation provides the expected ELO improvement over Stage 14 baseline.

**Expected Improvement:** +30-50 ELO

## Binary Configuration

### Test Engine (New)
- **Path:** `/workspace/binaries/seajay_stage15_sprt_candidate1`
- **Version:** SeaJay Stage-15-SPRT-Candidate-1
- **Features:** 
  - Full SEE algorithm with x-ray support
  - SEE-based move ordering (production mode)
  - SEE-based quiescence pruning (aggressive mode)
- **UCI Options Set:**
  - `SEEMode=production` (use SEE for all captures)
  - `SEEPruning=aggressive` (prune captures with SEE < -50)

### Base Engine (Control)
- **Path:** `/workspace/binaries/seajay-stage14-final`
- **Version:** SeaJay Stage-14-FINAL
- **Features:**
  - MVV-LVA move ordering only
  - No SEE evaluation
  - Conservative delta pruning (900cp)

## SPRT Parameters

### Statistical Configuration
- **elo0:** 0 (no regression allowed)
- **elo1:** 30 (minimum expected improvement)
- **alpha:** 0.05 (5% false positive rate)
- **beta:** 0.05 (5% false negative rate)

### Time Control
- **Format:** 10+0.1 (10 seconds + 0.1 second increment)
- **Rationale:** Fast time control for rapid iteration
- **Expected games/hour:** ~400 with 4 concurrent games

### Test Variants

#### 1. Opening Book Test (Recommended)
- **Book:** `/workspace/external/books/4moves_test.pgn`
- **Script:** `stage15_see_test_with_book.sh`
- **Advantage:** More position variety, reduces draw bias
- **Output:** `/workspace/sprt_results/stage15_see_with_book/`

#### 2. Starting Position Test
- **Opening:** Standard starting position
- **Script:** `stage15_see_test_startpos.sh`
- **Advantage:** Traditional test, consistent conditions
- **Output:** `/workspace/sprt_results/stage15_see_startpos/`

## Running the Tests

### Quick Start
```bash
# Run the main test runner
/workspace/tools/scripts/sprt_tests/run_stage15_sprt_tests.sh

# Choose option 1 for opening book test (recommended)
```

### Manual Execution
```bash
# Validate binaries first
/workspace/tools/scripts/sprt_tests/validate_stage15_binaries.sh

# Run opening book test
/workspace/tools/scripts/sprt_tests/stage15_see_test_with_book.sh

# OR run startpos test
/workspace/tools/scripts/sprt_tests/stage15_see_test_startpos.sh
```

### Monitoring Progress
```bash
# Watch live results (opening book test)
tail -f /workspace/sprt_results/stage15_see_with_book/fastchess.log

# Check current statistics
cat /workspace/sprt_results/stage15_see_with_book/fastchess.log | tail -20
```

## Expected Results

### Success Criteria
- **PASS (H1 accepted):** SEE provides ≥30 ELO improvement
- **Time to decision:** 2-4 hours with 4 concurrent games
- **Typical games needed:** 500-2000 depending on actual improvement

### Interpretation
- **Early PASS:** SEE improvement is substantial (>40 ELO)
- **Late PASS:** SEE improvement is around 30 ELO
- **FAIL:** Would indicate implementation issue (unexpected)
- **Inconclusive:** Improvement is borderline, may need tighter bounds

## Stage 15 SEE Features Being Tested

1. **SEE Algorithm Core**
   - Multi-piece exchange evaluation
   - X-ray attack detection
   - Special moves (en passant, promotions)
   - King participation rules

2. **Move Ordering Integration**
   - SEE-based capture ordering replacing MVV-LVA
   - Better move ordering = faster alpha-beta cutoffs
   - Expected to improve search efficiency

3. **Quiescence Pruning**
   - Aggressive pruning of bad captures (SEE < -50)
   - Depth-dependent thresholds
   - Equal exchange pruning in quiet positions
   - Should reduce quiescence tree size by 40-60%

## Test History

### Pre-SPRT Validation
- Performance benchmarks: 5-10% NPS improvement
- Tactical suite: No loss in solving rate
- SEE vs MVV-LVA agreement: 75%
- Cache hit rate: >99% for repeated positions

### Expected Outcome
Based on implementation quality and standard SEE improvements in other engines:
- **Conservative estimate:** +30 ELO
- **Likely result:** +35-40 ELO
- **Best case:** +50 ELO

## Troubleshooting

### If test fails immediately
1. Check binary paths are correct
2. Verify both engines run: `echo -e "uci\nquit" | [binary]`
3. Check fast-chess is installed

### If no improvement detected
1. Verify SEE options are set correctly
2. Check that Stage 15 binary has SEE enabled
3. Review game logs for anomalies

### If test runs too long
1. Results are borderline (close to elo0)
2. Can stop manually and review partial results
3. Consider adjusting elo1 parameter

## Next Steps After SPRT

### If PASS
1. Document final ELO gain
2. Merge to main branch
3. Update Stage 15 documentation as COMPLETE
4. Begin Stage 16 planning

### If FAIL (Unexpected)
1. Review implementation for bugs
2. Check test configuration
3. Run diagnostic tests
4. Consider parameter tuning (Day 8 work)

---

*Created: August 15, 2025*  
*Stage 15 Days 1-6 Complete, Day 7 SPRT Testing Ready*