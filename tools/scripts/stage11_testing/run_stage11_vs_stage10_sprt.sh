#!/bin/bash
# SPRT Test: Stage 11 (MVV-LVA) vs Stage 10 (Magic Bitboards)
# Testing hypothesis: Stage 11 should show +50-100 ELO improvement over Stage 10
# MVV-LVA move ordering should provide 15-30% node reduction and improved search efficiency

# Get the directory this script is in
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$SCRIPT_DIR"

# Binary paths - using the binaries in the binaries directory
# Updated to use SPRT candidate 2 with performance fixes
STAGE11_BINARY="$PROJECT_ROOT/binaries/seajay-stage11-candidate2-x86-64"
STAGE10_BINARY="$PROJECT_ROOT/binaries/seajay-stage10-x86-64"

# Fast-chess location
FASTCHESS="$PROJECT_ROOT/external/testers/fast-chess/fastchess"

# Output directory with timestamp
TIMESTAMP=$(date +"%Y%m%d_%H%M%S")
OUTPUT_DIR="$PROJECT_ROOT/sprt_results/SPRT-Stage11-vs-Stage10-$TIMESTAMP"
mkdir -p "$OUTPUT_DIR"

# Test configuration
TIME_CONTROL="10+0.1"  # 10 seconds + 0.1 second increment
THREADS="1"

# SPRT parameters for Phase 2
# Testing for MVV-LVA expected gain: +50 to +100 Elo
ALPHA="0.05"      # Type I error rate (5% false positive)
BETA="0.05"       # Type II error rate (5% false negative)
ELO0="0"          # Null hypothesis: no improvement
ELO1="50"         # Alternative hypothesis: +50 Elo improvement (conservative estimate)

# Opening book - use 4moves_test.pgn as recommended
OPENING_BOOK="$PROJECT_ROOT/external/books/4moves_test.pgn"

# Verify binaries exist
if [ ! -f "$STAGE11_BINARY" ]; then
    echo "Error: Stage 11 binary not found at $STAGE11_BINARY"
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
echo -e "uci\nquit" | timeout 2 "$STAGE11_BINARY" > /dev/null 2>&1
if [ $? -ne 0 ]; then
    echo "Error: Stage 11 binary not responding to UCI"
    exit 1
fi

echo -e "uci\nquit" | timeout 2 "$STAGE10_BINARY" > /dev/null 2>&1
if [ $? -ne 0 ]; then
    echo "Error: Stage 10 binary not responding to UCI"
    exit 1
fi

echo "==================================================================="
echo "SPRT Test: Stage 11 (MVV-LVA Candidate 2) vs Stage 10 (Magic Bitboards)"
echo "==================================================================="
echo "Testing MVV-LVA move ordering improvement (FIXED VERSION)"
echo "Previous candidate showed -10 ELO regression"
echo "This candidate has performance fixes:"
echo "  - In-place sorting (no heap allocation)"
echo "  - Only sorts captures, preserves quiet move order"
echo "  - Removed thread-local overhead"
echo "Expected gain: +20 to +50 Elo (conservative after fixes)"
echo ""
echo "Engines:"
echo "  Test:  Stage 11 - Magic Bitboards + MVV-LVA"
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
    -engine name=\"Stage11-MVV-LVA-v2\" cmd=\"$STAGE11_BINARY\" \
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
echo "PASS: Stage 11 provides significant improvement (≥$ELO1 Elo)"
echo "FAIL: Stage 11 provides no meaningful improvement"
echo "CONTINUE: Test still running (max games not reached)"
echo "==================================================================="

# Save test summary
cat > "$OUTPUT_DIR/test_summary.txt" << EOF
SPRT Test: Stage 11 vs Stage 10
================================
Date: $(date)
Test Engine: Stage 11 (MVV-LVA)
Base Engine: Stage 10 (Magic Bitboards)

Expected Improvement: +50 to +100 Elo
SPRT Parameters: elo0=$ELO0, elo1=$ELO1, alpha=$ALPHA, beta=$BETA
Time Control: $TIME_CONTROL

Stage 11 Features:
- MVV-LVA move ordering for captures
- Expected 15-30% node reduction
- Better alpha-beta pruning efficiency

Stage 10 Features:
- Magic bitboards for move generation
- Optimized perft performance

Results:
See sprt_output.txt for detailed results
EOF

echo ""
echo "Test summary saved to: $OUTPUT_DIR/test_summary.txt"