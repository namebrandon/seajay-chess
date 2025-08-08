# External Tools Documentation

This document describes the external tools used for testing and development of SeaJay Chess Engine.

## Quick Setup

Run the setup script to automatically download and build required tools:

```bash
./tools/scripts/setup-external-tools.sh
```

## Directory Structure

All external tools are stored in `external/` which is gitignored:

```
external/
├── engines/          # Chess engines for testing
├── testers/          # Tournament and testing tools  
├── books/            # Opening books
├── tablebases/       # Endgame tablebases (optional)
└── networks/         # NNUE network files
```

## Required Tools

### Testing Engines

**Stockfish** (Required from Phase 1, Stage 5)
- Purpose: Reference engine for SPRT testing
- Source: https://github.com/official-stockfish/Stockfish
- Installation: Automatically built by setup script

**Other Engines** (Optional)
- Ethereal: Mid-strength reference
- Laser: Alternative testing partner
- Various strength levels for progression testing

### Tournament Management

**fast-chess** (Required from Phase 1, Stage 5)
- Purpose: SPRT testing and tournament management
- Source: https://github.com/Disservin/fast-chess
- Features: Built-in SPRT, concurrent games, PGN output
- Installation: Automatically built by setup script

**cutechess-cli** (Alternative)
- Purpose: Alternative tournament manager
- Source: https://github.com/cutechess/cutechess

### Opening Books

**8moves_v3.pgn** (Required from Phase 1, Stage 5)
- Purpose: Balanced opening book for engine testing
- Lines: 8 moves deep, variety of openings
- Usage: Ensures diverse game positions

**4moves_test.pgn** (Created by setup)
- Purpose: Simple book for early development
- Usage: Initial testing when engine is weak

### NNUE Networks (Phase 4+)

**Stockfish Networks**
- Source: https://tests.stockfishchess.org/nns
- Purpose: Initial NNUE testing before custom networks
- Size: 20-40MB typically

### Endgame Tablebases (Optional)

**Syzygy Tablebases**
- Purpose: Perfect endgame play
- Source: https://syzygy-tables.info/
- Recommended: 3-4-5 piece tables
- Size: ~1GB for 3-4-5 piece

## Tool Configuration

Configuration files in `tools/configs/`:
- `engines.json`: Engine paths and options
- `sprt-config.json`: SPRT test parameters
- `fast-chess.json`: Tournament settings

## Usage Examples

### Running SPRT Test

```bash
./tools/scripts/run-sprt.sh new_version old_version
```

### Manual fast-chess Command

```bash
./external/testers/fast-chess/fast-chess \
    -engine cmd=./bin/seajay name=new \
    -engine cmd=./bin/seajay_old name=old \
    -each tc=8+0.08 \
    -rounds 5000 \
    -sprt elo0=0 elo1=5 alpha=0.05 beta=0.05 \
    -openings file=./external/books/8moves_v3.pgn
```

## Troubleshooting

### Build Failures
- Ensure C++17/20 compiler is installed
- Check make is available
- On macOS: Install Xcode Command Line Tools

### Missing Books
- Download manually from provided URLs
- Place in `external/books/`

### Permission Issues
- Ensure scripts are executable: `chmod +x tools/scripts/*.sh`

## License Considerations

These external tools have their own licenses:
- Stockfish: GPL v3
- fast-chess: MIT
- Opening books: Various (check individual sources)

These tools are not distributed with SeaJay and must be downloaded separately.