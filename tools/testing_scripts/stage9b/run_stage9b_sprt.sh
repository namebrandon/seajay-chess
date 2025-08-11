#!/bin/bash

# SPRT Test Script for SeaJay Stage 9b - Draw Detection
# Following SeaJay SPRT Testing Process Document

TEST_ID="SPRT-2025-008-STAGE9B"
TEST_NAME="Stage 9b (Draw Detection)" 
BASE_NAME="Stage 9 (No Draw Detection)"

# SPRT Parameters for Draw Detection Testing
# Note: This is NOT a traditional Elo improvement test
# We're testing for correctness and draw rate reduction
ELO0=-10    # Allow small Elo regression (draw detection may reduce tactical play)
ELO1=10     # Small Elo improvement bound (we're not expecting huge Elo gain)
ALPHA=0.05  # Type I error probability
BETA=0.05   # Type II error probability

# Test configuration optimized for draw detection
TC="10+0.1"     # Standard time control
ROUNDS=2000     # Higher game count for draw rate statistics
CONCURRENCY=1   # Single thread for consistency

# Paths
TEST_BIN="/workspace/bin/seajay_stage9b_draws"
BASE_BIN="/workspace/bin/seajay_stage9_base"
BOOK="/workspace/external/books/varied_4moves.pgn"
FASTCHESS="/workspace/external/testers/fast-chess/fastchess"
OUTPUT_DIR="/workspace/sprt_results/${TEST_ID}"

# Verify prerequisites
echo "=== Stage 9b SPRT Test Setup ==="
echo

if [ ! -f "$TEST_BIN" ]; then
    echo "ERROR: Test binary not found: $TEST_BIN"
    echo "Run ./prepare_stage9b_binaries.sh first"
    exit 1
fi

if [ ! -f "$BASE_BIN" ]; then
    echo "ERROR: Base binary not found: $BASE_BIN" 
    echo "Run ./prepare_stage9b_binaries.sh first"
    exit 1
fi

if [ ! -f "$FASTCHESS" ]; then
    echo "ERROR: Fast-chess not found: $FASTCHESS"
    echo "Install with: ./tools/scripts/setup-external-tools.sh"
    exit 1
fi

if [ ! -f "$BOOK" ]; then
    echo "WARNING: Opening book not found: $BOOK"
    echo "Will run without opening book"
    USE_BOOK=false
else
    USE_BOOK=true
fi

# Create output directory
mkdir -p "$OUTPUT_DIR"

echo "SPRT Test Configuration:"
echo "  Test ID: $TEST_ID"
echo "  Testing: $TEST_NAME vs $BASE_NAME"
echo "  SPRT bounds: [$ELO0, $ELO1] Elo"
echo "  Significance: α=$ALPHA, β=$BETA"
echo "  Time control: $TC"
echo "  Max rounds: $ROUNDS"
echo "  Output: $OUTPUT_DIR"
echo

echo "IMPORTANT: This test focuses on DRAW RATE REDUCTION"
echo "Success criteria:"
echo "  1. Stage 9b has <35% total draw rate (vs >40% for Stage 9)"
echo "  2. Stage 9b has <10% repetition draws (vs >20% for Stage 9)"  
echo "  3. No significant Elo regression (within [$ELO0, $ELO1])"
echo

read -p "Press Enter to start SPRT test (this may take 2-4 hours)..."

echo
echo "Starting SPRT test at $(date)"
echo "Results will be saved to: $OUTPUT_DIR"
echo

# Run fast-chess with SPRT
if [ "$USE_BOOK" = true ]; then
    $FASTCHESS \
        -engine name="$TEST_NAME" cmd="$TEST_BIN" \
        -engine name="$BASE_NAME" cmd="$BASE_BIN" \
        -each proto=uci tc="$TC" \
        -sprt elo0="$ELO0" elo1="$ELO1" alpha="$ALPHA" beta="$BETA" \
        -rounds "$ROUNDS" \
        -repeat \
        -recover \
        -openings file="$BOOK" format=pgn \
        -concurrency "$CONCURRENCY" \
        -pgnout "$OUTPUT_DIR/games.pgn" \
        -log file="$OUTPUT_DIR/fastchess.log" level=info \
        2>&1 | tee "$OUTPUT_DIR/console_output.txt"
else
    $FASTCHESS \
        -engine name="$TEST_NAME" cmd="$TEST_BIN" \
        -engine name="$BASE_NAME" cmd="$BASE_BIN" \
        -each proto=uci tc="$TC" \
        -sprt elo0="$ELO0" elo1="$ELO1" alpha="$ALPHA" beta="$BETA" \
        -rounds "$ROUNDS" \
        -repeat \
        -recover \
        -concurrency "$CONCURRENCY" \
        -pgnout "$OUTPUT_DIR/games.pgn" \
        -log file="$OUTPUT_DIR/fastchess.log" level=info \
        2>&1 | tee "$OUTPUT_DIR/console_output.txt"
fi

SPRT_RESULT=$?

echo
echo "=== SPRT Test Completed at $(date) ==="
echo "Results saved to: $OUTPUT_DIR"
echo

# Parse and display key metrics
if [ -f "$OUTPUT_DIR/console_output.txt" ]; then
    echo "=== Test Summary ==="
    
    # Extract final score line
    SCORE_LINE=$(grep -E "Score of.*:" "$OUTPUT_DIR/console_output.txt" | tail -1)
    if [ -n "$SCORE_LINE" ]; then
        echo "Final Score: $SCORE_LINE"
    fi
    
    # Extract Elo estimate
    ELO_LINE=$(grep -E "Elo:" "$OUTPUT_DIR/console_output.txt" | tail -1)
    if [ -n "$ELO_LINE" ]; then
        echo "Elo Estimate: $ELO_LINE"
    fi
    
    # Extract SPRT result
    SPRT_LINE=$(grep -E "SPRT:.*accepted|SPRT:.*failed" "$OUTPUT_DIR/console_output.txt" | tail -1)
    if [ -n "$SPRT_LINE" ]; then
        echo "SPRT Result: $SPRT_LINE"
        
        if echo "$SPRT_LINE" | grep -q "H1 accepted"; then
            echo
            echo "🎉 SPRT TEST PASSED!"
            echo "Stage 9b draw detection is statistically validated"
        elif echo "$SPRT_LINE" | grep -q "H0 accepted"; then
            echo
            echo "❌ SPRT TEST FAILED"
            echo "No significant improvement detected"
        fi
    fi
    
    echo
    echo "=== Draw Rate Analysis ==="
    echo "For detailed draw analysis, examine:"
    echo "  Games: $OUTPUT_DIR/games.pgn"
    echo "  Log: $OUTPUT_DIR/fastchess.log" 
    echo
    echo "Key metrics to check:"
    echo "  1. Total draw percentage in games"
    echo "  2. Games ending by repetition vs other draws"
    echo "  3. Average game length difference"
fi

echo "Test complete. Review results in: $OUTPUT_DIR"

exit $SPRT_RESULT