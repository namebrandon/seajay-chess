#!/bin/bash
# SPRT Test: Stage 13 (Iterative Deepening) vs Stage 12 (baseline) - 60+0.6 Time Control
# Testing hypothesis: Stage 13 should show even stronger improvement at longer time controls
# Using 4moves_test.pgn opening book for diversity

# Get the directory this script is in
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../../.." && pwd)"

# Binary paths
STAGE13_BINARY="$PROJECT_ROOT/binaries/seajay-stage13-sprt"
STAGE12_BINARY="$PROJECT_ROOT/binaries/seajay-stage12-baseline"

# Fast-chess location
FASTCHESS="$PROJECT_ROOT/external/testers/fast-chess/fastchess"

# Output directory with timestamp
TIMESTAMP=$(date +"%Y%m%d_%H%M%S")
OUTPUT_DIR="$PROJECT_ROOT/sprt_results/SPRT-Stage13-vs-Stage12-60s-$TIMESTAMP"
mkdir -p "$OUTPUT_DIR"

# Test configuration - LONGER TIME CONTROL
TIME_CONTROL="60+0.6"  # 60 seconds + 0.6 second increment
THREADS="1"
CONCURRENCY="1"  # Number of games to run in parallel

# SPRT parameters
# At longer time controls, improvements may be more pronounced
ALPHA="0.05"      # Type I error rate
BETA="0.05"       # Type II error rate  
ELO0="60"         # Null hypothesis: less than +60 Elo improvement
ELO1="90"         # Alternative hypothesis: +90 Elo improvement (higher end of range)

# Opening book - use 4moves_test.pgn as recommended
OPENING_BOOK="$PROJECT_ROOT/external/books/4moves_test.pgn"

# Verify binaries exist
if [ ! -f "$STAGE13_BINARY" ]; then
    echo "Error: Stage 13 binary not found at $STAGE13_BINARY"
    exit 1
fi

if [ ! -f "$STAGE12_BINARY" ]; then
    echo "Error: Stage 12 binary not found at $STAGE12_BINARY"
    exit 1
fi

if [ ! -f "$FASTCHESS" ]; then
    echo "Error: fast-chess not found at $FASTCHESS"
    echo "Attempting to download..."
    "$PROJECT_ROOT/tools/scripts/setup-external-tools.sh"
    if [ ! -f "$FASTCHESS" ]; then
        echo "Failed to download fast-chess"
        exit 1
    fi
fi

if [ ! -f "$OPENING_BOOK" ]; then
    echo "Error: Opening book not found at $OPENING_BOOK"
    echo "Please ensure external books are set up"
    exit 1
fi

# Test that both engines respond to UCI
echo "Testing engine responsiveness..."
echo -e "uci\nquit" | timeout 2 "$STAGE13_BINARY" > /dev/null 2>&1
if [ $? -ne 0 ]; then
    echo "Error: Stage 13 binary not responding to UCI"
    exit 1
fi

echo -e "uci\nquit" | timeout 2 "$STAGE12_BINARY" > /dev/null 2>&1
if [ $? -ne 0 ]; then
    echo "Error: Stage 12 binary not responding to UCI"
    exit 1
fi

# Verify versions
echo "Verifying engine versions..."
echo "Stage 13 version:"
echo -e "uci\nquit" | "$STAGE13_BINARY" 2>/dev/null | grep "id name"
echo "Stage 12 version:"
echo -e "uci\nquit" | "$STAGE12_BINARY" 2>/dev/null | grep "id name"

echo "==================================================================="
echo "SPRT Test: Stage 13 vs Stage 12 - LONG TIME CONTROL (60+0.6)"
echo "==================================================================="
echo "Testing iterative deepening improvements at longer time controls"
echo "Time Control: 60+0.6 seconds"
echo "SPRT: elo0=$ELO0, elo1=$ELO1, alpha=$ALPHA, beta=$BETA"
echo "Opening Book: 4moves_test.pgn (varied positions)"
echo "Output: $OUTPUT_DIR"
echo ""
echo "⚠️  WARNING: This test will take several hours to complete!"
echo "   Each game takes ~2-3 minutes, and we need hundreds of games"
echo "==================================================================="

# Create log file with test parameters
cat > "$OUTPUT_DIR/test_info.txt" << EOF
SPRT Test: Stage 13 vs Stage 12 (60+0.6 time control)
======================================================
Date: $(date)
Stage 13 Binary: $STAGE13_BINARY
Stage 12 Binary: $STAGE12_BINARY
Time Control: 60+0.6 seconds
SPRT Parameters:
  - H0 (null): ELO difference <= $ELO0
  - H1 (alternative): ELO difference >= $ELO1
  - Alpha (Type I error): $ALPHA
  - Beta (Type II error): $BETA
Opening Book: 4moves_test.pgn
Expected Result: H1 accepted (+60-95 Elo improvement)
Note: Longer time controls may show stronger improvement due to:
  - Better time management effectiveness
  - More iterations benefiting from aspiration windows
  - Stability detection more impactful with more time
Features Tested:
  - Aspiration windows (16cp initial, progressive widening)
  - Dynamic time management with stability detection
  - Enhanced UCI output with iteration details
  - Branching factor tracking and prediction
  - Performance optimizations
EOF

# Ask for confirmation due to long runtime
echo ""
echo "This test will take several hours. Continue? (y/n)"
read -r response
if [[ ! "$response" =~ ^[Yy]$ ]]; then
    echo "Test cancelled."
    exit 0
fi

echo "Starting long time control test..."
echo "You can monitor progress by tailing: $OUTPUT_DIR/sprt_output.txt"
echo ""

# Run SPRT test with longer time control
"$FASTCHESS" \
    -engine name="Stage13-ID" cmd="$STAGE13_BINARY" \
    -engine name="Stage12-TT" cmd="$STAGE12_BINARY" \
    -each proto=uci tc="$TIME_CONTROL" \
    -openings file="$OPENING_BOOK" format=pgn order=random \
    -games 2 \
    -rounds 2000 \
    -repeat \
    -recover \
    -concurrency "$CONCURRENCY" \
    -sprt elo0="$ELO0" elo1="$ELO1" alpha="$ALPHA" beta="$BETA" \
    -ratinginterval 5 \
    -scoreinterval 5 \
    -autosaveinterval 10 \
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
    echo "-------------------"
    tail -30 "$OUTPUT_DIR/sprt_output.txt" | grep -E "Elo|LLR|Games|Score|Result|accepted|rejected"
    
    # Check if H0 or H1 was accepted
    if grep -q "H1 accepted" "$OUTPUT_DIR/sprt_output.txt"; then
        echo ""
        echo "✅ SUCCESS: Stage 13 shows significant improvement over Stage 12!"
        echo "   Time management and aspiration windows work well at longer TCs"
    elif grep -q "H0 accepted" "$OUTPUT_DIR/sprt_output.txt"; then
        echo ""
        echo "❌ FAILURE: Stage 13 does not show expected improvement"
    else
        echo ""
        echo "⚠️  Test ended without conclusion (may have hit game limit)"
    fi
fi

# Calculate approximate test duration
if [ -f "$OUTPUT_DIR/sprt_output.txt" ]; then
    GAMES_PLAYED=$(grep -o "Games: [0-9]*" "$OUTPUT_DIR/sprt_output.txt" | tail -1 | awk '{print $2}')
    if [ -n "$GAMES_PLAYED" ]; then
        echo ""
        echo "Test Statistics:"
        echo "  Games played: $GAMES_PLAYED"
        echo "  Approximate duration: $(($GAMES_PLAYED * 3 / 60)) hours"
    fi
fi

echo "==================================================================="
echo "View games: less $OUTPUT_DIR/games.pgn"
echo "View log: less $OUTPUT_DIR/fastchess.log"
echo "=================================================================="