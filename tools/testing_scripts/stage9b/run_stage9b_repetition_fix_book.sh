#!/bin/bash

# SPRT Test Script for SeaJay Stage 9b - Repetition Detection Fix
# Tests Stage 9b with corrected repetition detection using 4moves_varied.pgn
# This test validates the fix across diverse opening positions

TEST_ID="SPRT-2025-011-REPETITION-FIX-BOOK"
TEST_NAME="Stage9b-RepetitionFix"
BASE_NAME="Stage9-Base"

# SPRT Parameters - expecting strong improvement with the fix
# The repetition bug caused -70 Elo loss, we expect significant recovery
ELO0=40     # Minimum expected improvement
ELO1=70     # Expected improvement bound (full recovery)
ALPHA=0.05  # Type I error probability
BETA=0.05   # Type II error probability

# Test configuration
TC="10+0.1"     # Standard time control
ROUNDS=2000     # Full rounds for comprehensive testing
CONCURRENCY=1   # Single thread for consistency

# Paths
TEST_BIN="/workspace/build/seajay"  # Current build with repetition fix
BASE_BIN="/workspace/bin/seajay_stage9_base"
BOOK="/workspace/external/books/4moves_varied.pgn"
FASTCHESS="/workspace/external/testers/fast-chess/fastchess"

# Create unique output directory
TIMESTAMP=$(date +"%Y%m%d_%H%M%S")
OUTPUT_DIR="/workspace/sprt_results/${TEST_ID}_${TIMESTAMP}"

echo "=== Stage 9b Repetition Fix SPRT Test (WITH OPENING BOOK) ==="
echo "Testing Stage 9b with corrected repetition detection"
echo "Using 4moves_varied.pgn for diverse opening positions"
echo

# Build info
echo "Build Information:"
echo "  Test binary: Stage 9b with repetition detection fix"
echo "  Base binary: Stage 9 (PST evaluation only)"
echo "  Key fix: Corrected parity calculation in isRepetitionDraw()"
echo "  Opening book: 4moves_varied.pgn"
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

# Check for opening book
if [ ! -f "$BOOK" ]; then
    # Try alternate locations
    BOOK="/workspace/external/books/varied_4moves.pgn"
    if [ ! -f "$BOOK" ]; then
        BOOK="/workspace/external/books/4moves_test.pgn"
        if [ ! -f "$BOOK" ]; then
            echo "WARNING: No opening book found!"
            echo "Attempted paths:"
            echo "  - /workspace/external/books/4moves_varied.pgn"
            echo "  - /workspace/external/books/varied_4moves.pgn"
            echo "  - /workspace/external/books/4moves_test.pgn"
            echo
            echo "Will use startpos only. Consider installing opening books."
            USE_BOOK=false
        else
            echo "Using opening book: $BOOK"
            USE_BOOK=true
        fi
    else
        echo "Using opening book: $BOOK"
        USE_BOOK=true
    fi
else
    echo "Using opening book: $BOOK"
    USE_BOOK=true
fi

# Create output directory
mkdir -p "$OUTPUT_DIR"

# Save test configuration
cat > "$OUTPUT_DIR/test_config.txt" <<EOF
Stage 9b Repetition Fix SPRT Test Configuration (WITH BOOK)
===========================================================
Test ID: $TEST_ID
Timestamp: $TIMESTAMP
Testing: $TEST_NAME vs $BASE_NAME
SPRT bounds: [$ELO0, $ELO1] Elo
Significance: α=$ALPHA, β=$BETA
Time control: $TC
Max rounds: $ROUNDS
Opening book: $([ "$USE_BOOK" = true ] && echo "$BOOK" || echo "None (startpos)")

Fix Description:
- Fixed parity calculation in isRepetitionDraw() (line 1853)
- Changed from: (m_gameHistory.size() % 2 == 0) ? 0 : 1
- Changed to: historyEven ? 0 : 1 (correct parity logic)
- This fixes incorrect repetition detection affecting both colors

Expected Results:
- Significant Elo improvement (40-70 points)
- Proper draw detection across all positions
- Consistent performance regardless of color
- Lower repetition draw rate
EOF

echo "SPRT Test Configuration:"
echo "  Test ID: $TEST_ID"
echo "  Testing: $TEST_NAME vs $BASE_NAME"
echo "  SPRT bounds: [$ELO0, $ELO1] Elo"
echo "  Significance: α=$ALPHA, β=$BETA"
echo "  Time control: $TC"
echo "  Max rounds: $ROUNDS"
echo "  Opening book: $([ "$USE_BOOK" = true ] && echo "$(basename $BOOK)" || echo "None")"
echo "  Output: $OUTPUT_DIR"
echo

echo "SUCCESS CRITERIA:"
echo "  1. SPRT passes with Elo gain > $ELO0"
echo "  2. Consistent performance across openings"
echo "  3. Proper draw detection functionality"
echo "  4. Reduced repetition draw rate"
echo

read -p "Press Enter to start SPRT test with opening book..."

echo
echo "Starting SPRT test at $(date)"
echo "Results will be saved to: $OUTPUT_DIR"
echo

# Run fast-chess with SPRT
if [ "$USE_BOOK" = true ]; then
    echo "Running with opening book: $BOOK"
    $FASTCHESS \
        -engine name="$TEST_NAME" cmd="$TEST_BIN" \
        -engine name="$BASE_NAME" cmd="$BASE_BIN" \
        -each proto=uci tc="$TC" \
        -sprt elo0="$ELO0" elo1="$ELO1" alpha="$ALPHA" beta="$BETA" \
        -rounds "$ROUNDS" \
        -repeat \
        -recover \
        -openings file="$BOOK" format=pgn order=random \
        -concurrency "$CONCURRENCY" \
        -pgnout "$OUTPUT_DIR/games.pgn" \
        -log file="$OUTPUT_DIR/fastchess.log" level=info \
        2>&1 | tee "$OUTPUT_DIR/console_output.txt"
else
    echo "Running without opening book (startpos only)"
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
            
            # Calculate recovery
            if command -v bc &> /dev/null; then
                RECOVERY=$(echo "$ELO_VALUE + 70" | bc)
                echo "  Recovery: ~$RECOVERY Elo points recovered"
                
                if (( $(echo "$ELO_VALUE > 30" | bc -l) )); then
                    echo "  ✅ Significant recovery achieved!"
                fi
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
            echo "The repetition detection fix is successful!"
            echo "Stage 9b now performs correctly with proper draw detection."
        elif echo "$SPRT_LINE" | grep -q "H0 accepted"; then
            echo
            echo "❌ SPRT TEST FAILED"
            echo "Fix did not achieve expected improvement"
            echo "Further investigation needed"
        else
            echo "⏸️ SPRT test inconclusive or stopped early"
        fi
    fi
    
    # Game statistics
    echo
    echo "=== Game Statistics ==="
    if [ -f "$OUTPUT_DIR/games.pgn" ]; then
        TOTAL_GAMES=$(grep -c "Result" "$OUTPUT_DIR/games.pgn" 2>/dev/null || echo "0")
        echo "Total games played: $TOTAL_GAMES"
        
        if [ "$TOTAL_GAMES" -gt 0 ]; then
            # Count results
            WINS=$(grep -c "1-0" "$OUTPUT_DIR/games.pgn" 2>/dev/null || echo "0")
            LOSSES=$(grep -c "0-1" "$OUTPUT_DIR/games.pgn" 2>/dev/null || echo "0")
            DRAWS=$(grep -c "1/2-1/2" "$OUTPUT_DIR/games.pgn" 2>/dev/null || echo "0")
            
            # Calculate percentages
            if command -v bc &> /dev/null; then
                WIN_PCT=$(echo "scale=1; $WINS * 100 / $TOTAL_GAMES" | bc)
                LOSS_PCT=$(echo "scale=1; $LOSSES * 100 / $TOTAL_GAMES" | bc)
                DRAW_PCT=$(echo "scale=1; $DRAWS * 100 / $TOTAL_GAMES" | bc)
                
                echo "Results distribution:"
                echo "  Wins: $WINS ($WIN_PCT%)"
                echo "  Losses: $LOSSES ($LOSS_PCT%)"
                echo "  Draws: $DRAWS ($DRAW_PCT%)"
            else
                echo "  Wins: $WINS"
                echo "  Losses: $LOSSES"
                echo "  Draws: $DRAWS"
            fi
            
            # Analyze draw types
            REP_DRAWS=$(grep -c "repetition" "$OUTPUT_DIR/games.pgn" 2>/dev/null || echo "0")
            FIFTY_DRAWS=$(grep -c "50-move" "$OUTPUT_DIR/games.pgn" 2>/dev/null || echo "0")
            INSUF_DRAWS=$(grep -c "insufficient" "$OUTPUT_DIR/games.pgn" 2>/dev/null || echo "0")
            STALE_DRAWS=$(grep -c "stalemate" "$OUTPUT_DIR/games.pgn" 2>/dev/null || echo "0")
            
            echo
            echo "Draw breakdown:"
            echo "  Repetition: $REP_DRAWS"
            echo "  50-move rule: $FIFTY_DRAWS"
            echo "  Insufficient material: $INSUF_DRAWS"
            echo "  Stalemate: $STALE_DRAWS"
            
            if [ "$REP_DRAWS" -gt 0 ] && [ "$DRAWS" -gt 0 ] && command -v bc &> /dev/null; then
                REP_PCT=$(echo "scale=1; $REP_DRAWS * 100 / $DRAWS" | bc)
                echo "  Repetition draws: $REP_PCT% of all draws"
                
                if (( $(echo "$REP_PCT > 50" | bc -l) )); then
                    echo "  ⚠️ High repetition rate among draws"
                else
                    echo "  ✅ Repetition rate is reasonable"
                fi
            fi
        fi
    fi
fi

echo
echo "Test complete. Full results in: $OUTPUT_DIR"
echo
echo "Analysis commands:"
echo "  View games: less $OUTPUT_DIR/games.pgn"
echo "  Check logs: tail -f $OUTPUT_DIR/fastchess.log"
echo "  Analyze repetitions: grep -B5 -A5 'repetition' $OUTPUT_DIR/games.pgn"
echo
echo "Next steps:"
echo "  1. If passed: Commit the fix and update documentation"
echo "  2. If failed: Analyze games for remaining issues"
echo "  3. Run performance profiling to verify no regression"

exit $SPRT_RESULT