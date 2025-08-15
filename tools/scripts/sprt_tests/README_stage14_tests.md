# Stage 14 SPRT Test Scripts

## Overview
These scripts test SeaJay Stage 14 (Quiescence Search) against previous versions to measure the ELO improvement from implementing quiescence search.

## Available Tests

### 1. Stage 14 vs Stage 13 - Fast TC with Opening Book
**Script:** `./run_stage14_vs_stage13_4moves_fast.sh`
- **Engines:** Stage 14 Candidate 1 vs Stage 13 SPRT-Fixed
- **Opening Book:** 4moves_test.pgn
- **Time Control:** 10+0.1 seconds
- **SPRT Bounds:** [0, 50] ELO
- **Expected Result:** H1 accepted (+50-150 ELO gain)
- **Duration:** ~1-2 hours
- **Purpose:** Primary validation of quiescence search improvement

### 2. Stage 14 vs Stage 12 - Fast TC with Opening Book
**Script:** `./run_stage14_vs_stage12_4moves_fast.sh`
- **Engines:** Stage 14 Candidate 1 vs Stage 12 Baseline
- **Opening Book:** 4moves_test.pgn
- **Time Control:** 10+0.1 seconds
- **SPRT Bounds:** [50, 100] ELO
- **Expected Result:** H1 accepted (+100-200 ELO gain)
- **Duration:** ~1-2 hours
- **Purpose:** Measure cumulative improvement (QS + ID)

### 3. Stage 14 vs Stage 13 - Fast TC from Starting Position
**Script:** `./run_stage14_vs_stage13_startpos_fast.sh`
- **Engines:** Stage 14 Candidate 1 vs Stage 13 SPRT-Fixed
- **Opening:** Starting position (no book)
- **Time Control:** 10+0.1 seconds
- **SPRT Bounds:** [0, 30] ELO
- **Expected Result:** H1 accepted (+30-100 ELO gain)
- **Duration:** ~1-2 hours
- **Purpose:** Test improvement without opening variety (may show more draws)

### 4. Stage 14 vs Stage 13 - Tournament TC with Opening Book
**Script:** `./run_stage14_vs_stage13_4moves_60s.sh`
- **Engines:** Stage 14 Candidate 1 vs Stage 13 SPRT-Fixed
- **Opening Book:** 4moves_test.pgn
- **Time Control:** 60+0.6 seconds
- **SPRT Bounds:** [30, 60] ELO
- **Expected Result:** H1 accepted (+60-150 ELO gain)
- **Duration:** ~6-12 hours
- **Purpose:** More accurate ELO measurement at tournament time control

## Output Files

All tests save their results in timestamped directories under `/workspace/sprt_results/`:
- `games.pgn` - All games played in PGN format with node counts and selective depths
- `fastchess.log` - Detailed log of the test execution
- `console_output.txt` - Live SPRT updates and final results
- `config.json` - Complete test configuration

## Running the Tests

### Quick Test Sequence (Recommended)
```bash
# 1. Run primary test
./run_stage14_vs_stage13_4moves_fast.sh

# 2. If successful, test against Stage 12
./run_stage14_vs_stage12_4moves_fast.sh

# 3. For overnight/thorough testing
./run_stage14_vs_stage13_4moves_60s.sh
```

### Monitoring Progress
While tests are running, you can monitor progress in the console output or check the PGN file:
```bash
# Watch live progress
tail -f /workspace/sprt_results/SPRT-*/console_output.txt

# Count games played
grep -c "Result" /workspace/sprt_results/SPRT-*/games.pgn
```

## Hypothesis Validation

### Expected ELO Gains
- **Stage 14 vs Stage 13:** +50-150 ELO (quiescence search benefit)
- **Stage 14 vs Stage 12:** +100-200 ELO (QS + ID combined)

### Why These Bounds?
- Quiescence search typically provides 50-150 ELO improvement
- The bounds are set conservatively to ensure statistical significance
- Wider bounds for Stage 12 comparison due to cumulative improvements

## Troubleshooting

### If Tests Don't Start
1. Verify binaries exist:
   ```bash
   ls -la /workspace/binaries/seajay-stage14-sprt-candidate1
   ls -la /workspace/binaries/seajay-stage13-sprt-fixed
   ```

2. Test engines manually:
   ```bash
   echo -e "uci\nquit" | /workspace/binaries/seajay-stage14-sprt-candidate1
   ```

### If Results Are Unexpected
- Check selective depth in games.pgn - should show 20-30+ ply extensions
- Review console_output.txt for any errors
- Verify the correct build mode was used (should be PRODUCTION)

## Notes
- All scripts will auto-download fast-chess if not present
- Tests use 4 concurrent games by default (2 for 60s time control)
- No adjudication is enabled to get accurate results
- Games are saved with full move notation and search statistics