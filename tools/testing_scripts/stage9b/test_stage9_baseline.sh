#!/bin/bash

# SPRT Test Script for SeaJay Stage 9 Baseline Self-Play
# Tests Stage 9 against itself from STARTPOS to establish baseline behavior
# This will tell us if 100% draws is normal for identical deterministic engines

TEST_ID="STAGE9-BASELINE-SELFPLAY"
TEST_NAME="Stage9-Base"
OPPONENT_NAME="Stage9-Base-Copy"

# SPRT Parameters - testing for equality (no regression)
# We expect 0 Elo difference between identical engines
ELO0=-10    # Accept up to -10 Elo difference as equal
ELO1=10     # Accept up to +10 Elo difference as equal
ALPHA=0.05  # Type I error probability
BETA=0.05   # Type II error probability

# Test configuration
TC="10+0.1"     # Standard time control
ROUNDS=100      # Reduced rounds for baseline testing
CONCURRENCY=1   # Single thread for consistency

# Paths - both use the same Stage 9 binary
TEST_BIN="/workspace/bin/seajay_stage9_base"
OPPONENT_BIN="/workspace/bin/seajay_stage9_base"
FASTCHESS="/workspace/external/testers/fast-chess/fastchess"

# Create unique output directory
TIMESTAMP=$(date +"%Y%m%d_%H%M%S")
OUTPUT_DIR="/workspace/sprt_results/${TEST_ID}_${TIMESTAMP}"

echo "=== Stage 9 Baseline Self-Play Test (STARTPOS) ==="
echo "Testing Stage 9 against itself to establish baseline behavior"
echo "This will show us what draw rate to expect from identical engines"
echo

# Build info
echo "Test Information:"
echo "  Both engines: Stage 9 (PST evaluation only, NO draw detection)"
echo "  Starting position: STARTPOS only (no opening book)"
echo "  Purpose: Establish baseline draw rate for identical engines"
echo

# Verify prerequisites
if [ ! -f "$TEST_BIN" ]; then
    echo "ERROR: Stage 9 binary not found: $TEST_BIN"
    echo "Please ensure Stage 9 binary exists"
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
Stage 9 Baseline Self-Play Test Configuration
=============================================
Test ID: $TEST_ID
Timestamp: $TIMESTAMP
Testing: Stage 9 vs Stage 9 (identical engines)
SPRT bounds: [$ELO0, $ELO1] Elo
Significance: α=$ALPHA, β=$BETA
Time control: $TC
Max rounds: $ROUNDS
Opening: STARTPOS ONLY (no book)

Purpose:
Establish baseline behavior for identical deterministic engines.
If these produce 100% draws, then Stage 9b's 100% draw rate is normal.

Expected Results:
- Identical engines should produce identical games
- From startpos, likely 100% same result (probably draws)
- This establishes whether Stage 9b behavior is abnormal
EOF

echo "Test Configuration:"
echo "  Test ID: $TEST_ID"
echo "  Both engines: Stage 9 baseline"
echo "  SPRT bounds: [$ELO0, $ELO1] Elo (testing for equality)"
echo "  Time control: $TC"
echo "  Max rounds: $ROUNDS"
echo "  Opening: STARTPOS ONLY"
echo "  Output: $OUTPUT_DIR"
echo

echo "KEY QUESTIONS THIS TEST ANSWERS:"
echo "  1. Do identical Stage 9 engines draw 100% from startpos?"
echo "  2. Is the game always the same (deterministic)?"
echo "  3. What is the 'normal' baseline we should expect?"
echo

read -p "Press Enter to start baseline test..."

echo
echo "Starting test at $(date)"
echo "Results will be saved to: $OUTPUT_DIR"
echo

# Run fast-chess with SPRT - NO OPENING BOOK, just startpos
$FASTCHESS \
    -engine name="$TEST_NAME" cmd="$TEST_BIN" \
    -engine name="$OPPONENT_NAME" cmd="$OPPONENT_BIN" \
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
echo "=== Baseline Test Completed at $(date) ==="
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
    fi
    
    # Game statistics
    echo
    echo "=== Game Analysis ==="
    if [ -f "$OUTPUT_DIR/games.pgn" ]; then
        TOTAL_GAMES=$(grep -c "Result" "$OUTPUT_DIR/games.pgn" 2>/dev/null || echo "0")
        WINS_WHITE=$(grep -c "1-0" "$OUTPUT_DIR/games.pgn" 2>/dev/null || echo "0")
        WINS_BLACK=$(grep -c "0-1" "$OUTPUT_DIR/games.pgn" 2>/dev/null || echo "0")
        DRAWS=$(grep -c "1/2-1/2" "$OUTPUT_DIR/games.pgn" 2>/dev/null || echo "0")
        
        echo "Total games: $TOTAL_GAMES"
        echo "White wins: $WINS_WHITE"
        echo "Black wins: $WINS_BLACK"
        echo "Draws: $DRAWS"
        
        if [ "$TOTAL_GAMES" -gt 0 ]; then
            DRAW_PCT=$(echo "scale=1; $DRAWS * 100 / $TOTAL_GAMES" | bc)
            echo "Draw rate: $DRAW_PCT%"
            
            # Check if all games are identical
            echo
            echo "Checking game variety..."
            UNIQUE_GAMES=$(grep -A20 "^1\." "$OUTPUT_DIR/games.pgn" | grep -E "^[0-9]+\." | sort -u | wc -l)
            echo "Unique game patterns: $UNIQUE_GAMES"
            
            if [ "$UNIQUE_GAMES" -eq 1 ]; then
                echo "✓ All games are IDENTICAL (deterministic behavior confirmed)"
            else
                echo "⚠ Games show variety (unexpected for identical engines)"
            fi
        fi
        
        # Check for repetition draws
        REP_DRAWS=$(grep -c "repetition" "$OUTPUT_DIR/games.pgn" 2>/dev/null || echo "0")
        if [ "$REP_DRAWS" -gt 0 ]; then
            echo
            echo "Repetition draws: $REP_DRAWS"
            REP_PCT=$(echo "scale=1; $REP_DRAWS * 100 / $DRAWS" | bc 2>/dev/null || echo "N/A")
            echo "Repetition rate among draws: $REP_PCT%"
        fi
    fi
fi

echo
echo "=== INTERPRETATION GUIDE ==="
echo
echo "If Stage 9 vs Stage 9 shows:"
echo "  • 100% draws → Stage 9b's 100% draws is NORMAL"
echo "  • Varied results → Stage 9b's 100% draws indicates a PROBLEM"
echo "  • Always same game → Deterministic behavior is EXPECTED"
echo
echo "Compare these results with Stage 9b to determine if draw detection"
echo "is affecting playing strength or maintaining baseline behavior."
echo

exit $SPRT_RESULT