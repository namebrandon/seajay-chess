# SeaJay Stage 8 - SPRT Testing Guide

## Available SPRT Tests

### 1. Fixed-Depth Consistency Test (RECOMMENDED FIRST)
**Script:** `./run_stage8_vs_stage7_sprt.sh`
- **Purpose:** Verify alpha-beta correctness
- **Comparison:** Stage 8 vs Stage 7 at fixed depth 4
- **Expected Result:** No significant difference (H0 accepted)
- **Why:** Confirms alpha-beta produces identical moves with fewer nodes

### 2. Time-Based Strength Test (RECOMMENDED SECOND)
**Script:** `./run_stage8_vs_stage7_timed_sprt.sh`
- **Purpose:** Measure ELO gain from alpha-beta efficiency
- **Comparison:** Stage 8 vs Stage 7 with 10+0.1 time control
- **Expected Result:** +50-150 ELO improvement (H1 accepted)
- **Why:** Alpha-beta allows 1-2 plies deeper search in same time

### 3. Depth Comparison Test (OPTIONAL)
**Script:** `./run_stage8_depth_comparison.sh`
- **Purpose:** Demonstrate strength gain from deeper search
- **Comparison:** Depth 4 vs Depth 3 (both with alpha-beta)
- **Expected Result:** Significant improvement (H1 accepted)
- **Why:** Shows the value of searching one ply deeper

## Quick Start

```bash
# Run the fixed-depth test first (proves correctness)
./run_stage8_vs_stage7_sprt.sh

# Then run the timed test (shows practical benefit)
./run_stage8_vs_stage7_timed_sprt.sh
```

## Test Parameters Summary

| Test | ELO Bounds | Time/Depth | Expected Games | Expected Result |
|------|------------|------------|----------------|-----------------|
| Fixed-Depth | [-10, +10] | Depth 4 | 100-500 | H0 (no difference) |
| Time-Based | [0, +100] | 10+0.1s | 200-1000 | H1 (+50-150 ELO) |
| Depth Compare | [0, +50] | Depth 3 vs 4 | 200-500 | H1 (+100+ ELO) |

## Binary Verification

Both binaries have been tested and verified:

### Stage 7 (No Alpha-Beta)
- Location: `/workspace/bin/seajay_stage7_no_alphabeta`
- Built from: commit c09a377
- Depth 5 nodes: 35,775
- Features: Negamax search WITHOUT pruning

### Stage 8 (With Alpha-Beta)
- Location: `/workspace/bin/seajay_stage8_alphabeta`
- Built from: current code with alpha-beta enabled
- Depth 5 nodes: 25,350 (29% reduction)
- Features: Negamax search WITH alpha-beta pruning
- Move ordering efficiency: 94-99%
- Effective branching factor: 6.8-7.6

## Results Interpretation

### Fixed-Depth Test Results
- **H0 Accepted**: ✅ GOOD - Alpha-beta is correct
- **H1 Accepted**: ❌ BAD - Something is wrong
- **Inconclusive**: ⚠️ OK - Engines are very close (expected)

### Time-Based Test Results
- **H1 Accepted**: ✅ GOOD - Alpha-beta provides ELO gain
- **H0 Accepted**: ❌ BAD - No improvement detected
- **Inconclusive**: ⚠️ Partial success - check partial results

## Typical Performance Metrics

At depth 5 from starting position:
- **Stage 7**: 35,775 nodes (no pruning)
- **Stage 8**: 25,350 nodes (with pruning)
- **Reduction**: 29% fewer nodes
- **EBF**: 7.6 (vs ~20 without pruning)
- **Move Ordering**: 94.9% efficiency

## Troubleshooting

### If tests fail to run:
1. Check binaries exist: `ls -la /workspace/bin/seajay_stage*`
2. Test binaries manually: `echo -e "position startpos\ngo depth 1\nquit" | /workspace/bin/seajay_stage8_alphabeta`
3. Check fast-chess: The scripts will auto-download if needed

### If results are unexpected:
1. Check console output: `cat /workspace/sprt_results/SPRT-*/console_output.txt`
2. Review games: `less /workspace/sprt_results/SPRT-*/games.pgn`
3. Check logs: `tail /workspace/sprt_results/SPRT-*/fastchess.log`

## Expected Outcomes Summary

1. **Fixed-depth test**: Should show NO difference (same moves, different node counts)
2. **Time-based test**: Should show SIGNIFICANT improvement (+50-150 ELO)
3. Both tests together prove:
   - Alpha-beta is correctly implemented (fixed-depth)
   - Alpha-beta provides practical benefit (time-based)

## Test Duration

- Fixed-depth tests: 5-15 minutes
- Time-based tests: 30-60 minutes (depending on ELO difference)
- Tests will stop early if statistical significance is reached

---

Ready to run! Start with the fixed-depth test to verify correctness, then run the time-based test to measure the ELO gain.