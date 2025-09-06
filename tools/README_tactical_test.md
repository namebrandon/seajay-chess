# Tactical Test Suite Evaluation Tool

## Overview
This tool evaluates SeaJay's tactical performance using standard test suites like WAC (Win At Chess). It properly handles both algebraic (SAN) and coordinate (UCI) notation for accurate move validation.

## Quick Start

### Run WAC Test (Recommended)
```bash
# Standard test (1 second per position)
./tools/run_wac_test.sh

# Quick test (100ms per position)
./tools/run_wac_test.sh ./bin/seajay 100

# Deep test (2 seconds per position)
./tools/run_wac_test.sh ./bin/seajay 2000
```

### Custom Test Suite
```bash
# General syntax
python3 tools/tactical_test.py [engine] [epd_file] [time_ms] [depth]

# Example with custom EPD file
python3 tools/tactical_test.py ./bin/seajay ./tests/my_positions.epd 1000

# Fixed depth testing
python3 tools/tactical_test.py ./bin/seajay ./tests/positions/wacnew.epd 0 10
```

## Key Features

### Accurate Move Validation
- Uses python-chess library for proper SAN to UCI conversion
- Correctly validates moves like "Nca4" = "c5a4", "g6" = "g5g6"
- No more false failures due to notation differences

### Enhanced Output
- Real-time progress updates every 20 seconds
- Clear final statistics with success rate prominently displayed
- Color-coded results for easy interpretation
- Performance grade (Excellent/Good/Moderate/Poor)

## Output

### Console Output
- Real-time results for each position (✓ PASS / ✗ FAIL)
- Move found, depth reached, evaluation score, nodes searched
- Summary statistics and performance rating

### Files Generated
- `tactical_test_YYYY-MM-DD_HH-MM-SS.csv` - Detailed results for this run
- `tactical_test_history.csv` - Historical tracking (appended each run)

## Performance Ratings
- **Excellent** (≥90%): Strong tactical ability
- **Good** (80-89%): Solid tactical performance  
- **Moderate** (70-79%): Average tactical strength
- **Below Average** (60-69%): Needs improvement
- **Poor** (<60%): Significant tactical weaknesses

## Test Suites

### WAC (Win At Chess)
- 300 tactical positions
- Located at: `/tests/positions/wacnew.epd`
- Tests various tactical motifs:
  - Checkmates (1-3 moves)
  - Winning material
  - Tactical combinations
  - Positional tactics

## Monitoring Tactical Performance

### During Development
Run quick tests (500ms) to ensure no tactical regression:
```bash
./tools/tactical_test.sh ./bin/seajay ./tests/positions/wacnew.epd 500 | grep "Success rate"
```

### Before Commits
Run standard test (2000ms) for baseline:
```bash
./tools/tactical_test.sh
```

### Tracking Progress
View historical performance:
```bash
tail tactical_test_history.csv
```

## Integration with Development

When working on optimizations like the e8f7 node explosion fix:

1. **Baseline Test**: Run before changes
   ```bash
   ./tools/tactical_test.sh
   mv tactical_test_history.csv tactical_baseline.csv
   ```

2. **After Changes**: Compare performance
   ```bash
   ./tools/tactical_test.sh
   diff tactical_baseline.csv tactical_test_history.csv
   ```

3. **Monitor Key Metrics**:
   - Success rate should not decrease
   - Average nodes per position (efficiency)
   - Average depth reached (search effectiveness)

## Troubleshooting

### Engine returns wrong notation
The script handles both:
- Algebraic notation (Qg6, Rxb2)
- Coordinate notation (g3g6, b3b2)

### Missing move counters in FEN
Script automatically adds "0 1" if missing

### Test suite format
EPD format required:
```
[FEN] bm [move]; id "[name]";
```

## Exit Codes
- 0: All tests passed
- 1-255: Number of failed positions (capped at 255)

## Related Tools
- `analyze_position.sh` - Deep analysis of single position
- `node_explosion_diagnostic.sh` - Node explosion testing