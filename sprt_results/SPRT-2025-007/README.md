# SPRT Test Configuration - Stage 9 PST

## Test Details

**Test ID:** SPRT-2025-007  
**Feature:** Piece-Square Tables (Positional Evaluation)  
**Date Prepared:** August 10, 2025  

## Binary Preparation

### Base Binary (Stage 8)
```bash
# Checked out commit 66a6637 (Stage 8 completion)
git checkout 66a6637
cmake .. -DCMAKE_BUILD_TYPE=Release && make clean && make -j4
cp seajay ../bin/seajay_stage8_alphabeta
```

### Test Binary (Stage 9)
```bash
# Restored Stage 9 changes with PST implementation
git stash pop
cmake .. -DCMAKE_BUILD_TYPE=Release && make clean && make -j4
cp seajay ../bin/seajay_stage9_pst
```

## Implementation Summary

### Stage 9 Changes:
1. **PST Infrastructure** (`src/evaluation/pst.h`)
   - Piece-square tables with middlegame/endgame values
   - Simplified PeSTO-style values
   - Compile-time validation

2. **Board Integration**
   - Incremental PST tracking in Board class
   - Special move handling (castling, en passant, promotion)
   - FEN loading recalculates PST from scratch

3. **Evaluation Enhancement**
   - Combined material + PST scores
   - Evaluation symmetry maintained
   - MVV-LVA capture ordering

## Test Parameters

- **SPRT Bounds:** [0, 50] Elo
- **Significance:** α = 0.05, β = 0.05
- **Time Control:** 10+0.1 seconds
- **Opening Book:** 4moves_test.pgn
- **Max Games:** 4000 (2000 rounds)

## Expected Results

PST typically provides **+150-200 Elo** improvement by adding:
- Piece centralization understanding
- Pawn structure evaluation
- King safety considerations
- Development incentives

Conservative SPRT bounds of [0, 50] should detect this improvement easily.

## Running the Test

```bash
cd /workspace
./run_stage9_sprt.sh
```

Test duration: 30-60 minutes depending on Elo difference.

## Verification Commands

```bash
# Test Stage 8 binary
echo -e "position startpos\ngo depth 4\nquit" | /workspace/bin/seajay_stage8_alphabeta

# Test Stage 9 binary  
echo -e "position startpos\ngo depth 4\nquit" | /workspace/bin/seajay_stage9_pst

# Check for PST influence (Stage 9 should prefer central moves)
echo -e "position startpos\ngo depth 1\nquit" | /workspace/bin/seajay_stage9_pst
```

## Files Created

- `/workspace/bin/seajay_stage8_alphabeta` - Base binary (material only)
- `/workspace/bin/seajay_stage9_pst` - Test binary (material + PST)
- `/workspace/run_stage9_sprt.sh` - SPRT test script
- `/workspace/sprt_results/SPRT-2025-007/` - Results directory

## Post-Test Analysis

After test completion:
1. Check console_output.txt for final results
2. Review games.pgn for game quality
3. Update test_summary.md with results
4. If passed, update project_status.md