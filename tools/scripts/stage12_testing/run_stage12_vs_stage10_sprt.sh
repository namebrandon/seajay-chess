#!/bin/bash
# SPRT Test: Stage 12 (Transposition Tables) vs Stage 10 (Magic Bitboards)
# Testing hypothesis: Stage 12 should show significant improvement over Stage 10
# Combines TT + MVV-LVA improvements against baseline Magic Bitboards

# Get the directory this script is in
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../../.." && pwd)"

# Binary paths - using the binaries in the binaries directory
STAGE12_BINARY="$PROJECT_ROOT/binaries/seajay-stage12-tt-candidate1-x86-64"
STAGE10_BINARY="$PROJECT_ROOT/binaries/seajay-stage10-x86-64"

# Fast-chess location
FASTCHESS="$PROJECT_ROOT/external/testers/fast-chess/fastchess"

# Output directory with timestamp
TIMESTAMP=$(date +"%Y%m%d_%H%M%S")
OUTPUT_DIR="$PROJECT_ROOT/sprt_results/SPRT-Stage12-vs-Stage10-$TIMESTAMP"
mkdir -p "$OUTPUT_DIR"

# Test configuration
TIME_CONTROL="10+0.1"  # 10 seconds + 0.1 second increment
THREADS="1"

# SPRT parameters for Phase 3
# Testing cumulative improvements: TT + MVV-LVA
# Expected gain: ~180-225 Elo total (TT: 130-175, MVV-LVA: 50)
# Using more aggressive target since this is cumulative
ALPHA="0.05"      # Type I error rate (5% false positive)
BETA="0.05"       # Type II error rate (5% false negative)
ELO0="0"          # Null hypothesis: no improvement
ELO1="75"         # Alternative hypothesis: +75 Elo improvement (conservative for combined)

# Opening book - use 4moves_test.pgn as recommended
OPENING_BOOK="$PROJECT_ROOT/external/books/4moves_test.pgn"

# Verify binaries exist
if [ ! -f "$STAGE12_BINARY" ]; then
    echo "Error: Stage 12 binary not found at $STAGE12_BINARY"
    exit 1
fi

if [ ! -f "$STAGE10_BINARY" ]; then
    echo "Error: Stage 10 binary not found at $STAGE10_BINARY"
    exit 1
fi

if [ ! -f "$FASTCHESS" ]; then
    echo "Error: fast-chess not found at $FASTCHESS"
    echo "Please run: $PROJECT_ROOT/tools/scripts/setup-external-tools.sh"
    exit 1
fi

if [ ! -f "$OPENING_BOOK" ]; then
    echo "Warning: Opening book not found at $OPENING_BOOK"
    echo "Will run without opening book (from startpos)"
    OPENING_BOOK=""
fi

# Test that both engines respond to UCI
echo "Testing engine responsiveness..."
echo -e "uci\nquit" | timeout 2 "$STAGE12_BINARY" > /dev/null 2>&1
if [ $? -ne 0 ]; then
    echo "Error: Stage 12 binary not responding to UCI"
    exit 1
fi

echo -e "uci\nquit" | timeout 2 "$STAGE10_BINARY" > /dev/null 2>&1
if [ $? -ne 0 ]; then
    echo "Error: Stage 10 binary not responding to UCI"
    exit 1
fi

echo "==================================================================="
echo "SPRT Test: Stage 12 (TT + MVV-LVA) vs Stage 10 (Magic Bitboards)"
echo "==================================================================="
echo "Testing cumulative improvements from Stage 11 and Stage 12"
echo "Combined features being tested:"
echo "  - Transposition Tables (Stage 12)"
echo "  - MVV-LVA move ordering (Stage 11)"
echo "  - Against baseline Magic Bitboards (Stage 10)"
echo ""
echo "Expected cumulative gain: +180-225 Elo"
echo "  - TT contribution: +130-175 Elo"
echo "  - MVV-LVA contribution: +50 Elo"
echo ""
echo "Engines:"
echo "  Test:  Stage 12 - TT + MVV-LVA + Magic Bitboards"
echo "  Base:  Stage 10 - Magic Bitboards only"
echo ""
echo "Parameters:"
echo "  Time Control: $TIME_CONTROL"
echo "  SPRT: elo0=$ELO0, elo1=$ELO1, alpha=$ALPHA, beta=$BETA"
if [ -n "$OPENING_BOOK" ]; then
    echo "  Opening Book: 4moves_test.pgn"
else
    echo "  Opening Book: None (starting position)"
fi
echo "  Output: $OUTPUT_DIR"
echo "==================================================================="
echo ""
echo "Starting SPRT test..."
echo "  - Max rounds: 10000 (20000 games)"
echo "  - Concurrency: 1 (sequential games)"
echo "  - SPRT will stop when reaching statistical decision"
echo "  - Estimated time: 2-4 hours at 10+0.1 time control"
echo ""
echo "Press Ctrl+C to stop early if needed"
echo ""

# Build fast-chess command
# Note: -rounds sets max number of game pairs (each pair = 2 games, one with each color)
# SPRT will stop early if it reaches a decision
FASTCHESS_CMD="$FASTCHESS \
    -engine name=\"Stage12-TT\" cmd=\"$STAGE12_BINARY\" \
    -engine name=\"Stage10-Magic\" cmd=\"$STAGE10_BINARY\" \
    -each tc=\"$TIME_CONTROL\" proto=uci \
    -rounds 10000 \
    -concurrency 1 \
    -recover \
    -sprt elo0=\"$ELO0\" elo1=\"$ELO1\" alpha=\"$ALPHA\" beta=\"$BETA\" \
    -ratinginterval 5 \
    -pgnout \"$OUTPUT_DIR/games.pgn\" fi \
    -log file=\"$OUTPUT_DIR/fastchess.log\" level=info"

# Add opening book if available
if [ -n "$OPENING_BOOK" ]; then
    FASTCHESS_CMD="$FASTCHESS_CMD -openings file=\"$OPENING_BOOK\" format=pgn order=random"
fi

# Run SPRT test and save output
eval "$FASTCHESS_CMD" | tee "$OUTPUT_DIR/sprt_output.txt"

# Parse results
echo ""
echo "==================================================================="
echo "Test Complete!"
echo "Results saved to: $OUTPUT_DIR"
echo ""

# Extract final statistics
if [ -f "$OUTPUT_DIR/sprt_output.txt" ]; then
    echo "Final Statistics:"
    echo "-----------------"
    tail -30 "$OUTPUT_DIR/sprt_output.txt" | grep -E "Elo|LLR|Games|Score|Result|SPRT"
fi

echo ""
echo "==================================================================="
echo "Interpretation:"
echo "---------------"
echo "PASS: Stage 12 provides significant improvement (≥$ELO1 Elo)"
echo "FAIL: Stage 12 provides no meaningful improvement"
echo "CONTINUE: Test still running (max games not reached)"
echo "==================================================================="

# Save test summary
cat > "$OUTPUT_DIR/test_summary.txt" << EOF
SPRT Test: Stage 12 vs Stage 10
================================
Date: $(date)
Test Engine: Stage 12 (TT + MVV-LVA)
Base Engine: Stage 10 (Magic Bitboards)

Expected Cumulative Improvement: +180-225 Elo
SPRT Parameters: elo0=$ELO0, elo1=$ELO1, alpha=$ALPHA, beta=$BETA
Time Control: $TIME_CONTROL

Stage 12 Features (cumulative):
- Transposition table with 128MB default size
- Proper Zobrist hashing with fifty-move counter
- Always-replace strategy
- Search integration (probe and store)
- Mate score adjustment for ply
- MVV-LVA move ordering for captures
- 25-30% node reduction from TT
- 87% TT hit rate

Stage 10 Features:
- Magic bitboards for move generation
- Optimized perft performance
- Basic alpha-beta search

Results:
See sprt_output.txt for detailed results
EOF

echo ""
echo "Test summary saved to: $OUTPUT_DIR/test_summary.txt"