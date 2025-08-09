# SeaJay Chess Engine - Development Tools

This directory contains development and debugging tools for the SeaJay chess engine.

## Directory Structure

```
tools/
├── configs/          # Configuration files for testing and tuning
├── debugging/        # Debugging utilities
├── scripts/          # Setup and automation scripts
├── datagen/         # Data generation tools (future)
└── tuner/           # Parameter tuning tools (future)
```

## Available Tools

### Debugging Tools (`debugging/`)

#### perft_debug
A comprehensive perft debugging tool that compares SeaJay's move generation with Stockfish.

**Build:**
```bash
g++ -std=c++20 -I/workspace -O3 debugging/perft_debug.cpp /workspace/src/core/*.cpp -o debugging/perft_debug
```

**Usage:**
```bash
# Compare with Stockfish at specific depth
./debugging/perft_debug compare "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1" 5

# Drill down into specific move
./debugging/perft_debug drill "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1" "b4b1" 4

# Find exact divergence point
./debugging/perft_debug find "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1" 3
```

### Setup Scripts (`scripts/`)

#### setup-external-tools.sh
Downloads and sets up external tools like Stockfish, fast-chess, and test suites.

**Usage:**
```bash
./scripts/setup-external-tools.sh
```

### Configuration Files (`configs/`)

- `engines.json` - Engine configurations for testing
- `sprt-config.json` - SPRT testing parameters

## External Tools

The engine uses several external tools stored in `/workspace/external/`:

### Stockfish
Location: `/workspace/external/engines/stockfish/stockfish`

Used for:
- Perft validation
- Move generation comparison
- Testing reference

**Test a position:**
```bash
echo "position fen <FEN> | go perft <depth> | quit" | /workspace/external/engines/stockfish/stockfish
```

### Fast-Chess (future)
Tournament and testing framework for engine development.

## Development Notes

- All debugging tools should be built with optimization (`-O3`) for performance
- Stockfish is used as the reference implementation for move generation validation
- Tools are designed to be standalone - they don't require the full engine to be built

## Known Issues

See `/workspace/project_docs/tracking/known_bugs.md` for current issues being debugged.