#!/bin/bash

# SPRT Test Script for SeaJay Stage 9b - Fixed Vector Operations
# Tests Stage 9b with draw detection after removing vector operations from hot path

TEST_ID="SPRT-2025-009-STAGE9B-FIXED"
TEST_NAME="Stage 9b Fixed (No Vector Ops)"
BASE_NAME="Stage 9 (Base)"

# SPRT Parameters for performance recovery test
# We expect significant Elo improvement after fixing the vector operations issue
ELO0=30     # Minimum expected improvement (should recover most of the -70 Elo loss)
ELO1=60     # Expected improvement bound (recovery from vector operations)
ALPHA=0.05  # Type I error probability
BETA=0.05   # Type II error probability

# Test configuration
TC="10+0.1"     # Standard time control
ROUNDS=2000     # Game count for statistical significance
CONCURRENCY=1   # Single thread for consistency

# Paths
TEST_BIN="/workspace/build/seajay"  # Current build with fix
BASE_BIN="/workspace/bin/seajay_stage9_base"
BOOK="/workspace/external/books/varied_4moves.pgn"
FASTCHESS="/workspace/external/testers/fast-chess/fastchess"
OUTPUT_DIR="/workspace/sprt_results/${TEST_ID}"

# Create timestamp for unique test run
TIMESTAMP=$(date +"%Y%m%d_%H%M%S")
OUTPUT_DIR="${OUTPUT_DIR}_${TIMESTAMP}"

echo "=== Stage 9b Fixed SPRT Test Setup ==="
echo "Testing Stage 9b with vector operations fix"
echo "Expected to recover ~70 Elo loss from regression"
echo

# Build info
echo "Build Information:"
echo "  Test binary: Stage 9b with draw detection + vector ops fix"
echo "  Base binary: Stage 9 (PST evaluation only)"
echo "  Key fix: Removed m_gameHistory push/pop from search hot path"
echo

# Verify prerequisites
if [ ! -f "$TEST_BIN" ]; then
    echo "ERROR: Test binary not found: $TEST_BIN"
    echo "Please build the current code first"
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
    # Try alternate book location
    BOOK="/workspace/external/books/4moves_test.pgn"
    if [ ! -f "$BOOK" ]; then
        echo "WARNING: Opening book not found"
        echo "Will run without opening book"
        USE_BOOK=false
    else
        USE_BOOK=true
    fi
else
    USE_BOOK=true
fi

# Create output directory
mkdir -p "$OUTPUT_DIR"

# Save test configuration
cat > "$OUTPUT_DIR/test_config.txt" <<EOF
Stage 9b Fixed SPRT Test Configuration
======================================
Test ID: $TEST_ID
Timestamp: $TIMESTAMP
Testing: $TEST_NAME vs $BASE_NAME
SPRT bounds: [$ELO0, $ELO1] Elo
Significance: α=$ALPHA, β=$BETA
Time control: $TC
Max rounds: $ROUNDS

Fix Description:
- Added m_inSearch flag to Board class
- Skip pushGameHistory() during search
- Skip pop_back() during unmake in search
- Draw detection remains fully enabled
- SearchInfo handles repetition detection during search

Expected Results:
- Recover most of the -70 Elo regression
- Maintain draw detection functionality
- Improved nodes per second (nps)
EOF

echo "SPRT Test Configuration:"
echo "  Test ID: $TEST_ID"
echo "  Testing: $TEST_NAME vs $BASE_NAME"
echo "  SPRT bounds: [$ELO0, $ELO1] Elo"
echo "  Significance: α=$ALPHA, β=$BETA"
echo "  Time control: $TC"
echo "  Max rounds: $ROUNDS"
echo "  Output: $OUTPUT_DIR"
echo

echo "SUCCESS CRITERIA:"
echo "  1. SPRT passes with Elo gain > $ELO0"
echo "  2. Draw detection still functional"
echo "  3. Performance improvement in nps"
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

# Parse and display results
if [ -f "$OUTPUT_DIR/console_output.txt" ]; then
    echo "=== Test Summary ==="
    
    # Extract final score
    SCORE_LINE=$(grep -E "Score of.*:" "$OUTPUT_DIR/console_output.txt" | tail -1)
    if [ -n "$SCORE_LINE" ]; then
        echo "Final Score: $SCORE_LINE"
    fi
    
    # Extract Elo estimate
    ELO_LINE=$(grep -E "Elo:" "$OUTPUT_DIR/console_output.txt" | tail -1)
    if [ -n "$ELO_LINE" ]; then
        echo "Elo Estimate: $ELO_LINE"
        
        # Extract just the Elo number
        ELO_VALUE=$(echo "$ELO_LINE" | grep -oE '[+-]?[0-9]+\.[0-9]+' | head -1)
        if [ -n "$ELO_VALUE" ]; then
            echo
            echo "Performance Analysis:"
            echo "  Original regression: -70 Elo"
            echo "  Current performance: $ELO_VALUE Elo"
            echo "  Recovery: ~$(echo "$ELO_VALUE + 70" | bc) Elo points recovered"
        fi
    fi
    
    # Extract SPRT result
    SPRT_LINE=$(grep -E "SPRT:.*accepted|SPRT:.*failed" "$OUTPUT_DIR/console_output.txt" | tail -1)
    if [ -n "$SPRT_LINE" ]; then
        echo
        echo "SPRT Result: $SPRT_LINE"
        
        if echo "$SPRT_LINE" | grep -q "H1 accepted"; then
            echo
            echo "✅ SPRT TEST PASSED!"
            echo "Stage 9b vector operations fix is successful!"
            echo "The performance regression has been resolved."
        elif echo "$SPRT_LINE" | grep -q "H0 accepted"; then
            echo
            echo "❌ SPRT TEST FAILED"
            echo "Fix did not achieve expected improvement"
            echo "Further investigation needed"
        fi
    fi
    
    # Game statistics
    echo
    echo "=== Game Statistics ==="
    GAMES_PLAYED=$(grep -c "Result" "$OUTPUT_DIR/games.pgn" 2>/dev/null || echo "0")
    echo "Total games played: $GAMES_PLAYED"
    
    if [ -f "$OUTPUT_DIR/games.pgn" ]; then
        DRAWS=$(grep -c "1/2-1/2" "$OUTPUT_DIR/games.pgn" || echo "0")
        if [ "$GAMES_PLAYED" -gt 0 ]; then
            DRAW_PCT=$(echo "scale=1; $DRAWS * 100 / $GAMES_PLAYED" | bc)
            echo "Draw rate: $DRAW_PCT%"
        fi
    fi
fi

echo
echo "Test complete. Full results in: $OUTPUT_DIR"
echo
echo "Next steps:"
echo "  1. Review detailed logs: cat $OUTPUT_DIR/fastchess.log"
echo "  2. Analyze games: $OUTPUT_DIR/games.pgn"
echo "  3. If passed, commit the fix!"

exit $SPRT_RESULT