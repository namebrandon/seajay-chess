# Historical Progression Tests

This directory contains individual test scripts to compare consecutive stages of SeaJay development.

## Purpose

These tests use diverse opening positions to avoid the deterministic behavior seen when testing from startpos. Each test plays 200 games using the `8moves_v3.pgn` opening book which contains 34,700 unique positions.

## Available Tests

Each script tests one stage against the next:

- `stage09_vs_stage10.sh` - PST evaluation vs Magic bitboards
- `stage10_vs_stage11.sh` - Magic bitboards vs MVV-LVA move ordering
- `stage11_vs_stage12.sh` - MVV-LVA vs Transposition tables
- `stage12_vs_stage13.sh` - Transposition tables vs Iterative deepening
- `stage13_vs_stage14.sh` - Iterative deepening vs Quiescence search
- `stage14_vs_stage15.sh` - Quiescence search vs SEE parameter tuning

## Usage

Run any individual test:
```bash
./stage09_vs_stage10.sh
./stage10_vs_stage11.sh
# etc...
```

Run all tests sequentially:
```bash
for script in stage*_vs_stage*.sh; do
    echo "Running $script..."
    ./$script
done
```

## Configuration

All tests use:
- **Time Control**: 10+0.1 (10 seconds + 0.1 increment)
- **Games**: 200 (100 with each color)
- **Opening Book**: `/workspace/external/books/8moves_v3.pgn`
- **Output Directory**: `./results/`

## Results

Results are saved in the `results/` directory:
- `.pgn` files contain all games
- `.log` files contain match statistics

To view results:
```bash
# View summary of all matches
for log in results/*.log; do
    echo "$(basename $log .log):"
    grep "Score of" "$log" | tail -1
done

# Analyze game results
grep "Result" results/*.pgn | sort | uniq -c
```

## Expected Outcomes

Each newer stage should ideally show improvement:
- **50-55% win rate**: Small improvement
- **55-60% win rate**: Clear improvement
- **60%+ win rate**: Significant improvement
- **<50% win rate**: Regression (unexpected)

## Notes

- These tests use diverse openings to avoid deterministic repetition
- 200 games provides reasonable statistical significance
- Draw rates may vary between stages based on evaluation improvements
- Some stages may show minimal improvement if they optimize for different aspects