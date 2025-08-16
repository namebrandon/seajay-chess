# White Bias Investigation Test Suite

## Purpose
This directory contains self-play test scripts to identify when the white bias bug was introduced in the SeaJay chess engine.

## Hypothesis
Based on our investigation, we believe:
- **Stage 7 & 8**: Should NOT exhibit white bias (material-only evaluation)
- **Stage 9**: Should exhibit WHITE BIAS due to PST implementation bug

## Test Scripts

### Individual Stage Tests
- `stage7_selfplay.sh` - Tests Stage 7 (material-only evaluation)
- `stage8_selfplay.sh` - Tests Stage 8 (material tracking infrastructure)
- `stage9_selfplay.sh` - Tests Stage 9 (PST implementation)

### Complete Test Suite
- `run_all_tests.sh` - Runs all three tests sequentially with pauses

## Usage

### Run Individual Test
```bash
./stage7_selfplay.sh
```

### Run All Tests
```bash
./run_all_tests.sh
```

## Test Configuration
- **Time Control**: 10+0.1 (10 seconds + 0.1 increment)
- **Games**: 100 per stage (50 game pairs)
- **Starting Position**: All games from startpos
- **Engine**: Self-play (same engine plays both sides)

## Output Files
Each test creates:
- `stage[N]_selfplay_results.log` - Detailed test log
- `stage[N]_selfplay.pgn` - PGN file of all games

## Interpreting Results

### Understanding the Test Method
- **Engine A** and **Engine B** are the same binary
- Each engine plays both White and Black equally (alternating colors)
- If no color bias exists, A and B should have similar scores
- If color bias exists, one engine will consistently win more

### Expected Results
- **Stage 7 & 8**: Balanced A/B scores (~50% each)
- **Stage 9**: Imbalanced A/B scores (indicating color bias)

### Bias Detection
The scripts automatically detect:
- ⚠️ WARNING if score difference > 20 games (significant imbalance)
- ✓ No bias if A and B have balanced scores

## Technical Details

### Why Stage 9?
Stage 9 introduced Piece-Square Tables (PST) which:
1. Added positional evaluation for the first time
2. Had a bug where Black's PST values weren't negated properly
3. Caused Black to evaluate good positions as bad

### The Bug
In `/workspace/src/evaluation/pst.h`:
```cpp
// BUG: Returns same sign for both colors
return s_pstTables[pt][lookupSq];

// SHOULD BE: Negate for Black
return (c == WHITE) ? val : -val;
```

This makes Black actively avoid good squares, giving White a systematic advantage.