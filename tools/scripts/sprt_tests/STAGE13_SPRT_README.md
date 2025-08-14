# Stage 13 SPRT Testing Guide

## Overview

Stage 13 implements **Iterative Deepening with Aspiration Windows** and sophisticated time management. These SPRT tests validate the expected +60-95 Elo improvement over Stage 12.

## Test Scripts

### 1. `run_stage13_vs_stage12_startpos.sh`
- **Purpose**: Basic validation from starting position
- **Time Control**: 10+0.1 seconds
- **Expected**: +60-95 Elo improvement
- **Duration**: ~30-60 minutes
- **Use Case**: Quick validation of improvements

### 2. `run_stage13_vs_stage12_4moves.sh` (RECOMMENDED)
- **Purpose**: Test with opening variety
- **Time Control**: 10+0.1 seconds
- **Opening Book**: 4moves_test.pgn
- **Expected**: +60-95 Elo improvement
- **Duration**: ~30-60 minutes
- **Use Case**: Primary validation test

### 3. `run_stage13_vs_stage11_4moves.sh`
- **Purpose**: Validate cumulative improvements
- **Time Control**: 10+0.1 seconds
- **Opening Book**: 4moves_test.pgn
- **Expected**: +190-270 Elo improvement (Stage 12 + 13)
- **Duration**: ~30-60 minutes
- **Use Case**: Confirm both TT and ID working together

### 4. `run_stage13_vs_stage12_60s.sh`
- **Purpose**: Test at longer time controls
- **Time Control**: 60+0.6 seconds
- **Opening Book**: 4moves_test.pgn
- **Expected**: +60-95 Elo (possibly higher)
- **Duration**: 3-6 hours
- **Use Case**: Validate time management effectiveness

### 5. `run_all_stage13_tests.sh`
- **Purpose**: Interactive menu to run any/all tests
- **Use Case**: Convenient test management

## Quick Start

```bash
# Run the recommended test
./run_stage13_vs_stage12_4moves.sh

# Or use the interactive menu
./run_all_stage13_tests.sh
```

## Stage 13 Features Being Tested

1. **Aspiration Windows**
   - 16cp initial window (Stockfish-proven value)
   - Progressive widening (delta += delta/3)
   - Maximum 5 re-searches before infinite window

2. **Time Management**
   - Dynamic allocation based on position stability
   - Soft/hard limits with stability adjustments
   - Game phase awareness

3. **Enhanced Search**
   - Full iterative deepening from depth 1
   - Branching factor tracking (EBF)
   - Move stability detection
   - Early termination logic

4. **Performance Optimizations**
   - Cached time checks (every 1024 nodes)
   - Inlined hot path functions
   - Optimized data structures

## SPRT Parameters Explained

### Standard Tests (vs Stage 12)
- **elo0 = 50**: Reject if improvement is less than 50 Elo
- **elo1 = 80**: Accept if improvement is 80+ Elo
- **alpha = 0.05**: 5% false positive rate
- **beta = 0.05**: 5% false negative rate

### Cumulative Test (vs Stage 11)
- **elo0 = 150**: Reject if less than 150 Elo improvement
- **elo1 = 200**: Accept if 200+ Elo improvement
- Tests both Stage 12 (TT) and Stage 13 (ID) improvements

## Expected Results

### Success Indicators
- **H1 accepted**: Stage 13 shows significant improvement
- **Elo difference**: +60-95 over Stage 12
- **Win rate**: ~58-63% vs Stage 12
- **LLR**: Positive and exceeding upper bound

### Failure Indicators
- **H0 accepted**: Improvement less than expected
- **Elo difference**: Less than +50
- **Win rate**: Below 57%
- **LLR**: Negative or below lower bound

## Monitoring Tests

```bash
# Watch test progress
tail -f /workspace/sprt_results/SPRT-Stage13-*/sprt_output.txt

# Check current statistics
grep -E "Elo|LLR|Games" /workspace/sprt_results/SPRT-Stage13-*/sprt_output.txt | tail -5

# View games
less /workspace/sprt_results/SPRT-Stage13-*/games.pgn
```

## Troubleshooting

### If tests fail to start
1. Check binaries exist: `ls -la /workspace/binaries/seajay-stage1*`
2. Verify UCI response: `echo -e "uci\nquit" | /workspace/binaries/seajay-stage13-sprt`
3. Check fastchess: `ls -la /workspace/external/fastchess/`

### If results are unexpected
1. Check iteration output in games
2. Verify aspiration window re-searches are happening
3. Look for time management adjustments in logs
4. Compare node counts between versions

## Test Duration Estimates

| Test | Games Needed | Time per Game | Total Duration |
|------|-------------|---------------|----------------|
| 10+0.1 startpos | 200-500 | 20-30s | 30-60 min |
| 10+0.1 4moves | 200-500 | 20-30s | 30-60 min |
| 10+0.1 vs S11 | 100-300 | 20-30s | 20-40 min |
| 60+0.6 | 200-400 | 2-3 min | 3-6 hours |

## Success Criteria

Stage 13 is considered successful if:
1. Shows +60-95 Elo improvement over Stage 12
2. Shows +190-270 Elo improvement over Stage 11
3. Time management adjustments visible in logs
4. Aspiration windows reduce re-search overhead
5. No performance regression (maintains ~1M NPS)

## Next Steps After Testing

If all tests pass:
1. Merge Stage 13 branch to main
2. Update project_status.md
3. Prepare for Stage 14 (Null Move Pruning)

If tests fail:
1. Check implementation against plan
2. Verify all 43 deliverables completed
3. Profile for performance bottlenecks
4. Review aspiration window parameters

---

Ready to validate Stage 13's sophisticated iterative deepening implementation!