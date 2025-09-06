# Tactical Test Suite Evaluation Tool

## Overview
This tool evaluates SeaJay's tactical performance using standard test suites like WAC (Win At Chess). It helps monitor tactical strength while making engine optimizations.

## Script
- `tactical_test.sh` - Evaluates engine performance on tactical positions

## Usage

### Basic Usage
```bash
./tools/tactical_test.sh
```
Uses defaults: SeaJay at ./bin/seajay, WAC test suite, 2 seconds per position

### Custom Parameters
```bash
./tools/tactical_test.sh [engine_path] [test_file] [time_ms] [depth]
```

Parameters:
- `engine_path`: Path to engine binary (default: ./bin/seajay)
- `test_file`: EPD file with test positions (default: ./tests/positions/wacnew.epd)
- `time_ms`: Time per position in milliseconds (default: 2000)
- `depth`: Fixed depth limit, 0 for time-based (default: 0)

### Examples

Quick tactical check (1 second per position):
```bash
./tools/tactical_test.sh ./bin/seajay ./tests/positions/wacnew.epd 1000
```

Deep analysis (5 seconds per position):
```bash
./tools/tactical_test.sh ./bin/seajay ./tests/positions/wacnew.epd 5000
```

Fixed depth testing:
```bash
./tools/tactical_test.sh ./bin/seajay ./tests/positions/wacnew.epd 0 10
```

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