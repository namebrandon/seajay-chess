# SPRT Testing How-To Guide for SeaJay Chess Engine

## What is SPRT?

**SPRT** (Sequential Probability Ratio Test) is a statistical method used to determine if a chess engine modification provides a genuine strength improvement. Unlike fixed-sample testing (e.g., "play 1000 games"), SPRT continues testing until it reaches statistical confidence, potentially saving thousands of games.

## Why Use SPRT?

In chess engine development:
- **Random variation** can make a weaker version appear stronger over a small sample
- **Small improvements** (1-5 ELO) require many games to detect reliably
- **Time is valuable** - SPRT stops as soon as statistical significance is reached
- **Industry standard** - Used by Stockfish, Leela, and all serious engines

## Understanding the Parameters

### The Hypothesis Test
SPRT tests two hypotheses:
- **H0 (Null Hypothesis)**: "The change provides no improvement" (ELO difference = elo0)
- **H1 (Alternative Hypothesis)**: "The change provides improvement" (ELO difference = elo1)

### Key Parameters

#### ELO Bounds (elo0 and elo1)
- **elo0**: The threshold below which we consider changes worthless (typically 0)
- **elo1**: The minimum improvement we want to detect (typically 3-5 ELO)
- **Example**: [0, 5] means "reject if no improvement, accept if +5 ELO or better"

#### Error Rates (alpha and beta)
- **alpha** (α): Probability of false positive (accepting a bad change) - typically 0.05 (5%)
- **beta** (β): Probability of false negative (rejecting a good change) - typically 0.05 (5%)
- **Standard**: Most engines use α=0.05, β=0.05

### Recommended Settings by Phase

| Phase | elo0 | elo1 | alpha | beta | Why |
|-------|------|------|-------|------|-----|
| Phase 2 (Early) | 0 | 5 | 0.05 | 0.05 | Catch major improvements |
| Phase 3 (Mature) | 0 | 3 | 0.05 | 0.05 | More sensitive testing |
| Phase 4 (Polish) | -1 | 3 | 0.05 | 0.10 | Allow slight regression for simplification |

## Step-by-Step Guide

### Prerequisites
1. **Two engine versions**: 
   - Base version (current master)
   - Test version (with your changes)
2. **Python 3.x** installed
3. **fast-chess** installed (already at `/workspace/external/testers/fast-chess/fastchess`)
4. **Opening book** (optional but recommended)

### Step 1: Prepare Your Engines

```bash
# Build your test version
cd /workspace/build
make clean && make -j

# Save it with a version name
cp /workspace/bin/seajay /workspace/bin/seajay_test

# Get or keep the base version
cp /workspace/bin/seajay /workspace/bin/seajay_base
```

### Step 2: Choose Your Testing Parameters

For a typical Phase 2 test:
```bash
# Standard Phase 2 parameters
ELO0=0      # No regression allowed
ELO1=5      # Looking for +5 ELO improvement
ALPHA=0.05  # 5% false positive rate
BETA=0.05   # 5% false negative rate
TC="10+0.1" # 10 seconds + 0.1 increment
```

### Step 3: Run the SPRT Test

#### Basic Command
```bash
python3 /workspace/tools/scripts/run_sprt.py \
    /workspace/bin/seajay_test \
    /workspace/bin/seajay_base \
    --elo0 0 --elo1 5 \
    --alpha 0.05 --beta 0.05 \
    --time-control "10+0.1" \
    --concurrency 4
```

#### With Opening Book
```bash
python3 /workspace/tools/scripts/run_sprt.py \
    /workspace/bin/seajay_test \
    /workspace/bin/seajay_base \
    --opening-book /workspace/external/books/4moves_test.pgn \
    --time-control "10+0.1" \
    --concurrency 4
```

#### Using Configuration File
```bash
python3 /workspace/tools/scripts/run_sprt.py \
    /workspace/bin/seajay_test \
    /workspace/bin/seajay_base \
    --config /workspace/tools/scripts/sprt_config.json
```

### Step 4: Understanding Live Output

During testing, you'll see updates like:
```
Games: 250 | Score: 135.5 (54.2%) | ELO: +29.3 | LLR: 1.23 | ⋯ CONTINUE
Games: 500 | Score: 265.0 (53.0%) | ELO: +20.9 | LLR: 1.87 | ⋯ CONTINUE
Games: 750 | Score: 395.5 (52.7%) | ELO: +19.1 | LLR: 2.35 | ✓ PASS
```

**What each field means:**
- **Games**: Total games played so far
- **Score**: Points scored (wins + 0.5×draws)
- **Percentage**: Win rate of test engine
- **ELO**: Current estimated ELO difference
- **LLR**: Log Likelihood Ratio (test statistic)
- **Status**: 
  - ⋯ CONTINUE = Keep testing
  - ✓ PASS = Improvement confirmed
  - ✗ FAIL = No improvement detected

### Step 5: Interpreting Results

#### PASS Result ✓
```
RESULT: PASSED - New engine shows significant improvement
```
- Your change provides at least elo1 improvement
- Safe to merge into master
- Document the improvement

#### FAIL Result ✗
```
RESULT: FAILED - No significant improvement detected
```
- Your change doesn't provide elo0 improvement
- Don't merge this change
- Try a different approach

#### INCONCLUSIVE Result ⋯
```
RESULT: INCONCLUSIVE - More games needed
```
- Reached max games without decision
- Results are borderline
- Consider running with more games or different bounds

## Practical Examples

### Example 1: Testing a New Evaluation Feature
You've added piece-square tables to evaluation:

```bash
# Quick test with lower confidence
python3 /workspace/tools/scripts/run_sprt.py \
    ./seajay_with_pst ./seajay_material_only \
    --elo0 0 --elo1 10 \
    --time-control "5+0.05" \
    --max-games 5000
```

### Example 2: Testing a Search Optimization
You've implemented null move pruning:

```bash
# Thorough test with standard parameters
python3 /workspace/tools/scripts/run_sprt.py \
    ./seajay_nullmove ./seajay_base \
    --elo0 0 --elo1 5 \
    --opening-book 8moves_v3.pgn \
    --time-control "10+0.1" \
    --concurrency 8
```

### Example 3: Testing a Simplification
You're removing complex code for maintainability:

```bash
# Allow small regression for simplification
python3 /workspace/tools/scripts/run_sprt.py \
    ./seajay_simplified ./seajay_complex \
    --elo0 -3 --elo1 2 \
    --alpha 0.05 --beta 0.10
```

## Time Estimates

| Time Control | Games/Hour | Time to Decision* |
|--------------|------------|-------------------|
| 1+0.01 | ~2000 | 15-30 minutes |
| 5+0.05 | ~800 | 1-2 hours |
| 10+0.1 | ~400 | 2-4 hours |
| 60+0.6 | ~60 | 12-24 hours |

*Assuming 4 concurrent games and typical improvement

## Common Issues and Solutions

### Issue: Test Runs Forever
**Cause**: Change provides borderline improvement
**Solution**: 
- Stop test manually (Ctrl+C)
- Review partial results
- Consider different elo bounds

### Issue: Immediate FAIL
**Cause**: Change is actually harmful
**Solution**:
- Verify engines are correct (not swapped)
- Check for bugs in implementation
- Run perft tests to ensure correctness

### Issue: High Variance in Results
**Cause**: Time control too fast or opening book missing
**Solution**:
- Use longer time control
- Add opening book for variety
- Increase concurrency for more games

### Issue: Different Results on Re-run
**Cause**: Normal statistical variation
**Solution**:
- This is expected behavior
- Trust the SPRT decision
- Use tighter bounds if needed

## Best Practices

1. **Always test against current master** - Not an old version
2. **Use opening books** - Prevents draw bias from startpos
3. **Run perft first** - Ensure no bugs before SPRT
4. **Document results** - Save SPRT output for history
5. **Be patient** - Let SPRT decide when to stop
6. **Test one change at a time** - Isolate improvements
7. **Use appropriate time controls** - Faster for big changes, slower for small

## Advanced Usage

### Custom SPRT Bounds
For specific testing scenarios:

```python
# Testing if version A is exactly equal to version B
--elo0 -2 --elo1 2  # Tight bounds around zero

# Testing if new version is at least 10 ELO better
--elo0 10 --elo1 15  # Both bounds positive

# Testing if simplification doesn't lose more than 5 ELO
--elo0 -5 --elo1 0  # Both bounds negative or zero
```

### Parallel Testing
Run multiple SPRT tests simultaneously:

```bash
# Terminal 1: Test feature A
python3 run_sprt.py engineA base --output-dir sprt_featureA

# Terminal 2: Test feature B  
python3 run_sprt.py engineB base --output-dir sprt_featureB
```

### Continuous Integration
Add to CI pipeline:

```bash
#!/bin/bash
# ci_test.sh
if python3 run_sprt.py new_build master_build \
   --elo0 -5 --elo1 0 \
   --max-games 1000; then
    echo "No regression detected"
    exit 0
else
    echo "Regression detected!"
    exit 1
fi
```

## Summary

SPRT testing is essential for chess engine development. It provides:
- **Statistical confidence** in improvements
- **Efficient testing** that stops when ready
- **Industry standard** validation

Remember: Trust the SPRT result even if it seems counterintuitive. The statistics account for variance that human observation might miss.

For questions or issues, consult:
- This guide
- `/workspace/tools/scripts/run_sprt.py --help`
- Chess Programming Wiki on SPRT
- The SeaJay development diary for examples