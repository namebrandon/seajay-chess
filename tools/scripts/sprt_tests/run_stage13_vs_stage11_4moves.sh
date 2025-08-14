#!/bin/bash
# SPRT Test: Stage 13 (Iterative Deepening) vs Stage 11 (MVV-LVA) - With Opening Book
# Testing hypothesis: Stage 13 should show significant improvement over Stage 11
# Expected: Stage 12 TT (+130-175) + Stage 13 ID (+60-95) = +190-270 Elo total

# Get the directory this script is in
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../../.." && pwd)"

# Binary paths
STAGE13_BINARY="$PROJECT_ROOT/binaries/seajay-stage13-sprt"
STAGE11_BINARY="$PROJECT_ROOT/binaries/seajay-stage11-candidate2-x86-64"

# Fast-chess location
FASTCHESS="$PROJECT_ROOT/external/testers/fast-chess/fastchess"

# Output directory with timestamp
TIMESTAMP=$(date +"%Y%m%d_%H%M%S")
OUTPUT_DIR="$PROJECT_ROOT/sprt_results/SPRT-Stage13-vs-Stage11-4moves-$TIMESTAMP"
mkdir -p "$OUTPUT_DIR"

# Test configuration
TIME_CONTROL="10+0.1"  # 10 seconds + 0.1 second increment
THREADS="1"
CONCURRENCY="1"  # Number of games to run in parallel

# SPRT parameters
# Testing for cumulative gain: Stage 12 + Stage 13 = +190-270 Elo
ALPHA="0.05"      # Type I error rate
BETA="0.05"       # Type II error rate  
ELO0="150"        # Null hypothesis: less than +150 Elo improvement
ELO1="200"        # Alternative hypothesis: +200 Elo improvement (conservative estimate)

# Opening book - use 4moves_test.pgn as recommended
OPENING_BOOK="$PROJECT_ROOT/external/books/4moves_test.pgn"

# Verify binaries exist
if [ ! -f "$STAGE13_BINARY" ]; then
    echo "Error: Stage 13 binary not found at $STAGE13_BINARY"
    exit 1
fi

if [ ! -f "$STAGE11_BINARY" ]; then
    echo "Error: Stage 11 binary not found at $STAGE11_BINARY"
    echo "Note: You may need to use a different Stage 11 binary path"
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

echo -e "uci\nquit" | timeout 2 "$STAGE11_BINARY" > /dev/null 2>&1
if [ $? -ne 0 ]; then
    echo "Error: Stage 11 binary not responding to UCI"
    exit 1
fi

# Verify versions
echo "Verifying engine versions..."
echo "Stage 13 version:"
echo -e "uci\nquit" | "$STAGE13_BINARY" 2>/dev/null | grep "id name"
echo "Stage 11 version:"
echo -e "uci\nquit" | "$STAGE11_BINARY" 2>/dev/null | grep "id name"

echo "==================================================================="
echo "SPRT Test: Stage 13 (ID+TT) vs Stage 11 (MVV-LVA) - 4MOVES"
echo "==================================================================="
echo "Testing cumulative improvements from Stage 12 and 13"
echo "Expected gain: +190-270 Elo (TT: +130-175, ID: +60-95)"
echo "Time Control: $TIME_CONTROL"
echo "SPRT: elo0=$ELO0, elo1=$ELO1, alpha=$ALPHA, beta=$BETA"
echo "Opening Book: 4moves_test.pgn (varied positions)"
echo "Output: $OUTPUT_DIR"
echo "==================================================================="

# Create log file with test parameters
cat > "$OUTPUT_DIR/test_info.txt" << EOF
SPRT Test: Stage 13 vs Stage 11 (4moves book)
==============================================
Date: $(date)
Stage 13 Binary: $STAGE13_BINARY
Stage 11 Binary: $STAGE11_BINARY
Time Control: $TIME_CONTROL
SPRT Parameters:
  - H0 (null): ELO difference <= $ELO0
  - H1 (alternative): ELO difference >= $ELO1
  - Alpha (Type I error): $ALPHA
  - Beta (Type II error): $BETA
Opening Book: 4moves_test.pgn
Expected Result: H1 accepted (+190-270 Elo improvement)
Stage 13 Features (vs Stage 11):
  - Transposition Tables (128MB default)
  - Iterative Deepening with aspiration windows
  - Dynamic time management with stability detection
  - Enhanced UCI output with iteration details
  - Branching factor tracking and prediction
  - Performance optimizations
EOF

# Run SPRT test with opening book
"$FASTCHESS" \
    -engine name="Stage13-ID+TT" cmd="$STAGE13_BINARY" \
    -engine name="Stage11-MVV-LVA" cmd="$STAGE11_BINARY" \
    -each proto=uci tc="$TIME_CONTROL" \
    -openings file="$OPENING_BOOK" format=pgn order=random \
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
        echo "✅ SUCCESS: Stage 13 shows significant improvement over Stage 11!"
        echo "   This confirms both TT and ID improvements are working"
    elif grep -q "H0 accepted" "$OUTPUT_DIR/sprt_output.txt"; then
        echo ""
        echo "❌ FAILURE: Stage 13 does not show expected improvement over Stage 11"
    else
        echo ""
        echo "⚠️  Test ended without conclusion (may have hit game limit)"
    fi
fi

echo "==================================================================="
echo "View games: less $OUTPUT_DIR/games.pgn"
echo "View log: less $OUTPUT_DIR/fastchess.log"
echo "=================================================================="