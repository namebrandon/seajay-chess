# Stage 12 SPRT Testing Scripts

## Overview

This directory contains SPRT (Sequential Probability Ratio Test) scripts for validating the Stage 12 Transposition Tables implementation against previous stages.

## Available Tests

### 1. Stage 12 vs Stage 11 (Primary Test)
**Script:** `run_stage12_vs_stage11_sprt.sh`
- **Purpose:** Test TT improvement over MVV-LVA baseline
- **Expected Gain:** +130-175 Elo
- **Time Control:** 10+0.1 (fast)
- **Duration:** 2-4 hours

### 2. Stage 12 vs Stage 10 (Cumulative Test)  
**Script:** `run_stage12_vs_stage10_sprt.sh`
- **Purpose:** Test combined TT + MVV-LVA improvements
- **Expected Gain:** +180-225 Elo cumulative
- **Time Control:** 10+0.1 (fast)
- **Duration:** 2-4 hours

### 3. Stage 12 vs Stage 11 - 60s (Deep Search Test)
**Script:** `run_stage12_vs_stage11_60s_sprt.sh`
- **Purpose:** Test TT benefits at deeper search depths
- **Expected Gain:** +130-175 Elo (potentially higher)
- **Time Control:** 60+0.6 (slow)
- **Duration:** 12-24 hours

## Running Tests

### Quick Start
```bash
# Primary test - Stage 12 vs Stage 11
./run_stage12_vs_stage11_sprt.sh

# Cumulative test - Stage 12 vs Stage 10
./run_stage12_vs_stage10_sprt.sh

# Deep search test - 60 second games
./run_stage12_vs_stage11_60s_sprt.sh
```

### Prerequisites
1. Ensure binaries exist:
   - `/workspace/binaries/seajay-stage12-tt-candidate1-x86-64`
   - `/workspace/binaries/seajay-stage11-candidate2-x86-64`
   - `/workspace/binaries/seajay-stage10-x86-64`

2. Fast-chess installed:
   - Run `/workspace/tools/scripts/setup-external-tools.sh` if needed

3. Opening books (optional but recommended):
   - `4moves_test.pgn` for variety
   - `8moves_v3.pgn` for deeper openings

## SPRT Parameters

All tests use standard Phase 3 parameters:
- **α (alpha):** 0.05 (5% false positive rate)
- **β (beta):** 0.05 (5% false negative rate)
- **elo0:** 0 (null hypothesis: no improvement)
- **elo1:** 50-75 (alternative hypothesis: significant improvement)

## Expected Results

### Stage 12 Features Being Tested:
- Transposition table with 128MB default size
- Proper Zobrist hashing with fifty-move counter
- Always-replace strategy
- 25-30% node reduction
- 87% TT hit rate in middlegame
- Better move ordering from hash moves

### Success Criteria:
- **PASS:** Confirms TT provides expected improvement
- **FAIL:** Indicates implementation issues or overestimated gains
- **CONTINUE:** Needs more games (borderline result)

## Output

Results are saved to:
```
/workspace/sprt_results/SPRT-Stage12-vs-StageXX-TIMESTAMP/
├── games.pgn           # All games played
├── fastchess.log       # Detailed engine logs
├── sprt_output.txt     # SPRT statistics
└── test_summary.txt    # Test configuration summary
```

## Monitoring Progress

During testing, you'll see live updates:
```
Games: 500 | Score: 265.0 (53.0%) | ELO: +20.9 | LLR: 1.87 | ⋯ CONTINUE
```

- **Games:** Total games played
- **Score:** Points scored (wins + draws/2)
- **ELO:** Current Elo estimate
- **LLR:** Log Likelihood Ratio
- **Status:** CONTINUE/PASS/FAIL

## Tips

1. **Start with Stage 12 vs Stage 11** - This is the primary validation
2. **Run overnight tests** - Use 60s time control for thorough validation
3. **Check TT statistics** - Monitor hit rates in the engine output
4. **Save results** - Archive SPRT outputs for documentation

## Troubleshooting

### Test fails immediately
- Check binaries are correct versions
- Verify UCI names match expected versions
- Ensure TT is actually enabled

### Very slow progress
- Normal for small improvements
- Consider using faster time control initially
- Check system isn't overloaded

### High draw rate
- Expected with repetition detection
- Opening book helps provide variety
- TT actually increases draw rate (finds repetitions faster)

## Stage 12 Implementation Notes

The TT implementation provides:
- **Phases 0-5 Complete:** Core functionality implemented
- **Phases 6-8 Deferred:** Advanced features saved for later
- **Production Ready:** All tests passing, valgrind clean
- **Expected Impact:** Major search efficiency improvement

See `/workspace/project_docs/stage_implementations/stage12_transposition_tables_implementation.md` for full details.