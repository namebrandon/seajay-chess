#!/bin/bash
# Stage 10 Self-Play Test for White Bias Investigation
# Magic Bitboards implementation - Expected: WHITE BIAS PRESENT (if PST bug persists)

echo "================================================"
echo "Stage 10 Self-Play Test - White Bias Investigation"
echo "================================================"
echo ""
echo "Testing: Stage 10 (Magic Bitboards) self-play from startpos"
echo "Expected: WHITE BIAS - If PST bug persists from Stage 9"
echo "Time control: 10+0.1"
echo ""

# Configuration
ENGINE="/workspace/binaries/seajay-stage10-e3c59e2"
OUTPUT_DIR="/workspace/tools/scripts/sprt_tests/white_bias_investigation"
LOG_FILE="$OUTPUT_DIR/stage10_selfplay_results.log"

# Time control
TC="10+0.1"

# Number of games
GAMES=100

# Create output directory if needed
mkdir -p "$OUTPUT_DIR"

# Verify binary exists
if [ ! -f "$ENGINE" ]; then
    echo "ERROR: Stage 10 binary not found: $ENGINE"
    exit 1
fi

# Check for fast-chess
FASTCHESS="/workspace/external/testers/fast-chess/fastchess"
if [ ! -f "$FASTCHESS" ]; then
    echo "fast-chess not found, downloading..."
    /workspace/tools/scripts/setup-external-tools.sh
fi

# Display binary info
echo "Binary checksum:"
md5sum "$ENGINE"
echo ""

# Run self-play test
echo "Starting Stage 10 self-play test..."
echo "Running $GAMES games from starting position"
echo "NOTE: Magic bitboards shouldn't affect bias, but may change game patterns"
echo ""

# Use fast-chess for self-play
"$FASTCHESS" \
    -engine cmd="$ENGINE" name=Stage10-A \
    -engine cmd="$ENGINE" name=Stage10-B \
    -each tc="$TC" \
    -games 2 -rounds 50 -repeat \
    -pgnout file="$OUTPUT_DIR/stage10_selfplay.pgn" \
    -log file="$LOG_FILE" level=info \
    -recover \
    -event "Stage 10 Self-Play White Bias Test" \
    -site "Starting Position Only"

# Extract and display results
echo ""
echo "================================================"
echo "Stage 10 Self-Play Results:"
echo "================================================"

# Parse the log for results
if [ -f "$LOG_FILE" ]; then
    echo "Win Statistics:"
    grep -A5 "Finished match" "$LOG_FILE" | tail -6
    
    # Extract specific win counts from fast-chess output
    MATCH_LINE=$(grep -E "Score of Stage10-A vs Stage10-B" "$LOG_FILE" | tail -1)
    
    # Extract wins/losses/draws from format: "Score of A vs B: X - Y - Z [...]"
    if [[ "$MATCH_LINE" =~ ([0-9]+)[[:space:]]-[[:space:]]([0-9]+)[[:space:]]-[[:space:]]([0-9]+) ]]; then
        A_SCORE="${BASH_REMATCH[1]}"
        B_SCORE="${BASH_REMATCH[2]}"
        DRAWS="${BASH_REMATCH[3]}"
    else
        A_SCORE="0"
        B_SCORE="0"
        DRAWS="0"
    fi
    
    echo ""
    echo "Summary (Engine A vs Engine B):"
    echo "  Engine A score: $A_SCORE"
    echo "  Engine B score: $B_SCORE"
    echo "  Draws: $DRAWS"
    
    # Calculate percentages if possible
    TOTAL=$((A_SCORE + B_SCORE + DRAWS))
    if [ $TOTAL -gt 0 ]; then
        A_PCT=$((A_SCORE * 100 / TOTAL))
        B_PCT=$((B_SCORE * 100 / TOTAL))
        DRAW_PCT=$((DRAWS * 100 / TOTAL))
        echo ""
        echo "Percentages:"
        echo "  Engine A: ${A_PCT}%"
        echo "  Engine B: ${B_PCT}%"
        echo "  Draws: ${DRAW_PCT}%"
        
        # Check for engine imbalance (which would indicate color bias)
        echo ""
        echo "Note: Since both engines play both colors equally,"
        echo "      a significant imbalance between A and B scores"
        echo "      indicates a color bias in the engine."
        
        # Check if there's significant imbalance
        DIFF=$((A_SCORE > B_SCORE ? A_SCORE - B_SCORE : B_SCORE - A_SCORE))
        if [ $DIFF -gt 20 ]; then
            echo ""
            echo "⚠️  COLOR BIAS CONFIRMED: Significant imbalance detected"
            echo "    PST bug likely persists in Stage 10!"
            echo "    (One engine consistently wins more, despite alternating colors)"
        else
            echo ""
            echo "✓ Engines have balanced scores"
            echo "    Magic bitboards implementation may have changed game dynamics."
        fi
    fi
else
    echo "ERROR: Log file not found: $LOG_FILE"
fi

echo ""
echo "Full log saved to: $LOG_FILE"
echo "PGN saved to: $OUTPUT_DIR/stage10_selfplay.pgn"