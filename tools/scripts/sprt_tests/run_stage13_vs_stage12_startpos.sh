#!/bin/bash
# SPRT Test: Stage 13 (Iterative Deepening) vs Stage 12 (baseline) - From Startpos
# Testing hypothesis: Stage 13 should show +60-95 ELO improvement over Stage 12
# Using starting position only (no opening book)

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
OUTPUT_DIR="$PROJECT_ROOT/sprt_results/SPRT-Stage13-vs-Stage12-startpos-$TIMESTAMP"
mkdir -p "$OUTPUT_DIR"

# Test configuration
TIME_CONTROL="10+0.1"  # 10 seconds + 0.1 second increment
THREADS="1"
CONCURRENCY="1"  # Number of games to run in parallel

# SPRT parameters
# Testing for Stage 13 gain: +60-95 Elo
ALPHA="0.05"      # Type I error rate
BETA="0.05"       # Type II error rate  
ELO0="50"         # Null hypothesis: less than +50 Elo improvement
ELO1="80"         # Alternative hypothesis: +80 Elo improvement (middle of expected range)

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
echo "SPRT Test: Stage 13 (Iterative Deepening) vs Stage 12 (TT) - STARTPOS"
echo "==================================================================="
echo "Testing for +60-95 Elo gain from iterative deepening improvements"
echo "Time Control: $TIME_CONTROL"
echo "SPRT: elo0=$ELO0, elo1=$ELO1, alpha=$ALPHA, beta=$BETA"
echo "Opening: Starting position only (no book)"
echo "Output: $OUTPUT_DIR"
echo "==================================================================="

# Create log file with test parameters
cat > "$OUTPUT_DIR/test_info.txt" << EOF
SPRT Test: Stage 13 vs Stage 12 (Startpos)
==========================================
Date: $(date)
Stage 13 Binary: $STAGE13_BINARY
Stage 12 Binary: $STAGE12_BINARY
Time Control: $TIME_CONTROL
SPRT Parameters:
  - H0 (null): ELO difference <= $ELO0
  - H1 (alternative): ELO difference >= $ELO1
  - Alpha (Type I error): $ALPHA
  - Beta (Type II error): $BETA
Opening: Starting position only
Expected Result: H1 accepted (+60-95 Elo improvement)
Features Tested:
  - Aspiration windows (16cp initial, progressive widening)
  - Dynamic time management with stability detection
  - Enhanced UCI output with iteration details
  - Branching factor tracking and prediction
  - Performance optimizations
EOF

# Run SPRT test from starting position
"$FASTCHESS" \
    -engine name="Stage13-ID" cmd="$STAGE13_BINARY" \
    -engine name="Stage12-TT" cmd="$STAGE12_BINARY" \
    -each proto=uci tc="$TIME_CONTROL" \
    -games 2 \
    -rounds 5000 \
    -repeat \
    -recover \
    -concurrency "$CONCURRENCY" \
    -sprt elo0="$ELO0" elo1="$ELO1" alpha="$ALPHA" beta="$BETA" \
    -ratinginterval 10 \
    -scoreinterval 10 \
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
    elif grep -q "H0 accepted" "$OUTPUT_DIR/sprt_output.txt"; then
        echo ""
        echo "❌ FAILURE: Stage 13 does not show expected improvement"
    else
        echo ""
        echo "⚠️  Test ended without conclusion (may have hit game limit)"
    fi
fi

echo "==================================================================="
echo "View games: less $OUTPUT_DIR/games.pgn"
echo "View log: less $OUTPUT_DIR/fastchess.log"
echo "=================================================================="