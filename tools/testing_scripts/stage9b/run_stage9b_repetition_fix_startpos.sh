#!/bin/bash

# SPRT Test Script for SeaJay Stage 9b - Repetition Detection Fix
# Tests Stage 9b with corrected repetition detection from STARTPOS ONLY
# This test specifically validates the fix for the White repetition bug

TEST_ID="SPRT-2025-010-REPETITION-FIX-STARTPOS"
TEST_NAME="Stage9b-RepetitionFix"
BASE_NAME="Stage9-Base"

# SPRT Parameters - expecting full recovery from the bug
# The repetition bug caused -70 Elo loss, we expect full recovery
ELO0=50     # Minimum expected improvement (most of the -70 Elo back)
ELO1=80     # Expected improvement bound (full recovery)
ALPHA=0.05  # Type I error probability
BETA=0.05   # Type II error probability

# Test configuration
TC="10+0.1"     # Standard time control
ROUNDS=500      # Reduced rounds for startpos testing
CONCURRENCY=1   # Single thread for consistency

# Paths
TEST_BIN="/workspace/build/seajay"  # Current build with repetition fix
BASE_BIN="/workspace/bin/seajay_stage9_base"
FASTCHESS="/workspace/external/testers/fast-chess/fastchess"

# Create unique output directory
TIMESTAMP=$(date +"%Y%m%d_%H%M%S")
OUTPUT_DIR="/workspace/sprt_results/${TEST_ID}_${TIMESTAMP}"

echo "=== Stage 9b Repetition Fix SPRT Test (STARTPOS) ==="
echo "Testing Stage 9b with corrected repetition detection"
echo "This test uses STARTPOS ONLY to validate the White repetition bug fix"
echo

# Build info
echo "Build Information:"
echo "  Test binary: Stage 9b with repetition detection fix"
echo "  Base binary: Stage 9 (PST evaluation only)"
echo "  Key fix: Corrected parity calculation in isRepetitionDraw()"
echo "  Test focus: Startpos only (no opening book)"
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

# Create output directory
mkdir -p "$OUTPUT_DIR"

# Save test configuration
cat > "$OUTPUT_DIR/test_config.txt" <<EOF
Stage 9b Repetition Fix SPRT Test Configuration (STARTPOS)
=========================================================
Test ID: $TEST_ID
Timestamp: $TIMESTAMP
Testing: $TEST_NAME vs $BASE_NAME
SPRT bounds: [$ELO0, $ELO1] Elo
Significance: α=$ALPHA, β=$BETA
Time control: $TC
Max rounds: $ROUNDS
Opening: STARTPOS ONLY (no book)

Fix Description:
- Fixed parity calculation in isRepetitionDraw() (line 1853)
- Changed from: (m_gameHistory.size() % 2 == 0) ? 0 : 1
- Changed to: historyEven ? 0 : 1 (correct parity logic)
- This fixes White incorrectly detecting repetitions from startpos

Expected Results:
- No more systematic draws when playing White
- Full recovery of the -70 Elo regression
- Balanced win/loss/draw rates for both colors
EOF

echo "SPRT Test Configuration:"
echo "  Test ID: $TEST_ID"
echo "  Testing: $TEST_NAME vs $BASE_NAME"
echo "  SPRT bounds: [$ELO0, $ELO1] Elo"
echo "  Significance: α=$ALPHA, β=$BETA"
echo "  Time control: $TC"
echo "  Max rounds: $ROUNDS"
echo "  Opening: STARTPOS ONLY"
echo "  Output: $OUTPUT_DIR"
echo

echo "SUCCESS CRITERIA:"
echo "  1. SPRT passes with Elo gain > $ELO0"
echo "  2. No systematic White draws by repetition"
echo "  3. Balanced results for both colors"
echo

read -p "Press Enter to start SPRT test from startpos..."

echo
echo "Starting SPRT test at $(date)"
echo "Results will be saved to: $OUTPUT_DIR"
echo

# Run fast-chess with SPRT - NO OPENING BOOK
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
            echo "  Original regression: -70 Elo (due to repetition bug)"
            echo "  Current performance: $ELO_VALUE Elo"
            
            # Check if positive (recovery successful)
            if (( $(echo "$ELO_VALUE > 0" | bc -l) )); then
                echo "  ✅ Bug fix successful! Recovery achieved."
            fi
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
            echo "The repetition detection bug has been successfully fixed!"
            echo "Stage 9b now correctly handles repetitions from startpos."
        elif echo "$SPRT_LINE" | grep -q "H0 accepted"; then
            echo
            echo "❌ SPRT TEST FAILED"
            echo "Fix did not achieve expected improvement"
            echo "Further investigation needed"
        fi
    fi
    
    # Analyze repetition draws specifically
    echo
    echo "=== Repetition Analysis ==="
    if [ -f "$OUTPUT_DIR/games.pgn" ]; then
        TOTAL_GAMES=$(grep -c "Result" "$OUTPUT_DIR/games.pgn" 2>/dev/null || echo "0")
        REP_DRAWS=$(grep -c "repetition" "$OUTPUT_DIR/games.pgn" 2>/dev/null || echo "0")
        
        # Count White/Black repetition draws
        echo "Analyzing repetition patterns..."
        
        # Extract games where Stage9b played White and drew by repetition
        WHITE_REP=$(grep -B10 "repetition" "$OUTPUT_DIR/games.pgn" | grep -c "\[White \"$TEST_NAME\"\]" || echo "0")
        BLACK_REP=$(grep -B10 "repetition" "$OUTPUT_DIR/games.pgn" | grep -c "\[Black \"$TEST_NAME\"\]" || echo "0")
        
        echo "Total games: $TOTAL_GAMES"
        echo "Repetition draws: $REP_DRAWS"
        echo "  Stage9b as White repetition draws: $WHITE_REP"
        echo "  Stage9b as Black repetition draws: $BLACK_REP"
        
        if [ "$WHITE_REP" -gt 0 ] && [ "$TOTAL_GAMES" -gt 20 ]; then
            WHITE_REP_PCT=$(echo "scale=1; $WHITE_REP * 100 / ($TOTAL_GAMES / 2)" | bc)
            echo "  White repetition rate: $WHITE_REP_PCT%"
            
            if (( $(echo "$WHITE_REP_PCT > 30" | bc -l) )); then
                echo "  ⚠️ WARNING: High White repetition rate detected!"
                echo "  The bug may not be fully fixed."
            else
                echo "  ✅ White repetition rate is normal"
            fi
        fi
    fi
fi

echo
echo "Test complete. Full results in: $OUTPUT_DIR"
echo
echo "Next steps:"
echo "  1. Review game PGNs: $OUTPUT_DIR/games.pgn"
echo "  2. Check for repetition patterns in games"
echo "  3. Run with opening book test: ./run_stage9b_repetition_fix_book.sh"

exit $SPRT_RESULT