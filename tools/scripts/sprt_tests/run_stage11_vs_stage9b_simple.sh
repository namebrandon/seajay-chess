#!/bin/bash
# SPRT Test: Stage 11 (with MVV-LVA) vs Stage 9b (without MVV-LVA)
# Testing hypothesis: Stage 11 should show improvement from MVV-LVA
# Using Stage 9b as baseline since it's the last stable binary without MVV-LVA

# Get the directory this script is in
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../../.." && pwd)"

# Binary paths
STAGE11_BINARY="$PROJECT_ROOT/bin/seajay_stage11_mvv_lva"
STAGE9B_BINARY="$PROJECT_ROOT/bin/seajay_stage9b_no_magic"

# Fast-chess location
FASTCHESS="$PROJECT_ROOT/external/testers/fast-chess/fastchess"

# Output directory with timestamp
TIMESTAMP=$(date +"%Y%m%d_%H%M%S")
OUTPUT_DIR="$PROJECT_ROOT/sprt_results/SPRT-Stage11-vs-Stage9b-$TIMESTAMP"
mkdir -p "$OUTPUT_DIR"

# Test configuration
TIME_CONTROL="10+0.1"  # 10 seconds + 0.1 second increment

# SPRT parameters
# Testing for MVV-LVA specific gain: +50 Elo (conservative estimate)
ALPHA="0.05"      # Type I error rate
BETA="0.05"       # Type II error rate  
ELO0="0"          # Null hypothesis: no improvement
ELO1="50"         # Alternative hypothesis: +50 Elo improvement

# Opening book - use 4moves_test.pgn
OPENING_BOOK="$PROJECT_ROOT/external/books/4moves_test.pgn"

# Verify binaries exist
if [ ! -f "$STAGE11_BINARY" ]; then
    echo "Error: Stage 11 binary not found at $STAGE11_BINARY"
    exit 1
fi

if [ ! -f "$STAGE9B_BINARY" ]; then
    echo "Error: Stage 9b binary not found at $STAGE9B_BINARY"
    exit 1
fi

if [ ! -f "$FASTCHESS" ]; then
    echo "Error: fast-chess not found at $FASTCHESS"
    echo "Please run: $PROJECT_ROOT/tools/scripts/setup-external-tools.sh"
    exit 1
fi

if [ ! -f "$OPENING_BOOK" ]; then
    echo "Error: Opening book not found at $OPENING_BOOK"
    echo "Please ensure external books are set up"
    exit 1
fi

# Test that both engines respond to UCI
echo "Testing engine responsiveness..."
echo -e "uci\nquit" | timeout 2 "$STAGE11_BINARY" > /dev/null 2>&1
if [ $? -ne 0 ]; then
    echo "Error: Stage 11 binary not responding to UCI"
    exit 1
fi

echo -e "uci\nquit" | timeout 2 "$STAGE9B_BINARY" > /dev/null 2>&1
if [ $? -ne 0 ]; then
    echo "Error: Stage 9b binary not responding to UCI"
    exit 1
fi

echo "==================================================================="
echo "SPRT Test: Stage 11 (MVV-LVA) vs Stage 9b (baseline)"
echo "==================================================================="
echo "Testing for +50 Elo gain from MVV-LVA"
echo "Time Control: $TIME_CONTROL"
echo "SPRT: elo0=$ELO0, elo1=$ELO1, alpha=$ALPHA, beta=$BETA"
echo "Opening Book: 4moves_test.pgn"
echo "Output: $OUTPUT_DIR"
echo "==================================================================="

# Run SPRT test
"$FASTCHESS" \
    -engine name="Stage11-MVV-LVA" cmd="$STAGE11_BINARY" \
    -engine name="Stage9b-Baseline" cmd="$STAGE9B_BINARY" \
    -each tc="$TIME_CONTROL" proto=uci \
    -openings file="$OPENING_BOOK" format=pgn order=random \
    -games 2 \
    -repeat \
    -recover \
    -sprt elo0="$ELO0" elo1="$ELO1" alpha="$ALPHA" beta="$BETA" \
    -ratinginterval 10 \
    -pgnout "$OUTPUT_DIR/games.pgn" fi \
    -log file="$OUTPUT_DIR/fastchess.log" level=info \
    | tee "$OUTPUT_DIR/sprt_output.txt"

# Parse results
echo ""
echo "==================================================================="
echo "Test Complete!"
echo "Results saved to: $OUTPUT_DIR"
echo ""

# Extract final statistics
if [ -f "$OUTPUT_DIR/sprt_output.txt" ]; then
    echo "Final Statistics:"
    tail -20 "$OUTPUT_DIR/sprt_output.txt" | grep -E "Elo|LLR|Games|Score|Result"
fi

echo "==================================================================="