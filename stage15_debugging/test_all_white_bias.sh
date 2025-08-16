#!/bin/bash
# Test all available binaries for White bias from starting position

echo "=========================================="
echo "White Bias Historical Test"
echo "Testing all available binaries from startpos"
echo "=========================================="
echo ""

FASTCHESS="/workspace/external/testers/fast-chess/fastchess"
OUTPUT_BASE="/workspace/stage15_debugging/white_bias_historical"
mkdir -p "$OUTPUT_BASE"

# Number of games per binary (keep small for quick testing)
GAMES=20  # 10 pairs
TC="1+0.01"  # Very fast time control

# Define binaries to test in chronological order
BINARIES=(
    "seajay-stage10-x86-64:Stage 10"
    "seajay-stage11-x86-64:Stage 11"
    "seajay-stage12-baseline:Stage 12"
    "seajay-stage13-sprt-fixed:Stage 13"
    "seajay-stage14-sprt-candidate1-GOLDEN:Stage 14 C1"
    "seajay-stage14-final:Stage 14 Final"
    "seajay-stage15-original:Stage 15 Original"
    "seajay-stage15-properly-fixed:Stage 15 Fixed"
)

# Results file
RESULTS_FILE="$OUTPUT_BASE/summary.txt"
echo "White Bias Test Results - $(date)" > "$RESULTS_FILE"
echo "===========================================" >> "$RESULTS_FILE"
echo "" >> "$RESULTS_FILE"

# Function to test one binary
test_binary() {
    local BINARY_PATH="/workspace/binaries/$1"
    local STAGE_NAME="$2"
    local OUTPUT_DIR="$OUTPUT_BASE/$1"
    
    if [ ! -f "$BINARY_PATH" ]; then
        echo "  ⚠️  Binary not found: $BINARY_PATH"
        echo "$STAGE_NAME: Binary not found" >> "$RESULTS_FILE"
        return
    fi
    
    echo "Testing $STAGE_NAME..."
    mkdir -p "$OUTPUT_DIR"
    
    # Run self-play test
    $FASTCHESS \
        -engine cmd="$BINARY_PATH" name=White \
        -engine cmd="$BINARY_PATH" name=Black \
        -each tc="$TC" \
        -games 2 -rounds $((GAMES/2)) -repeat \
        -pgnout file="$OUTPUT_DIR/games.pgn" notation=san \
        -log file="$OUTPUT_DIR/test.log" level=error \
        -recover 2>&1 | grep -q "Score of" || true
    
    # Count results
    if [ -f "$OUTPUT_DIR/games.pgn" ]; then
        WHITE_WINS=$(grep -c "\[Result \"1-0\"\]" "$OUTPUT_DIR/games.pgn" 2>/dev/null || echo "0")
        BLACK_WINS=$(grep -c "\[Result \"0-1\"\]" "$OUTPUT_DIR/games.pgn" 2>/dev/null || echo "0")
        DRAWS=$(grep -c "\[Result \"1/2-1/2\"\]" "$OUTPUT_DIR/games.pgn" 2>/dev/null || echo "0")
        TOTAL=$((WHITE_WINS + BLACK_WINS + DRAWS))
        
        if [ $TOTAL -gt 0 ]; then
            WHITE_PCT=$((WHITE_WINS * 100 / TOTAL))
            BLACK_PCT=$((BLACK_WINS * 100 / TOTAL))
            DRAW_PCT=$((DRAWS * 100 / TOTAL))
            
            # Determine bias level
            if [ $WHITE_PCT -ge 80 ]; then
                BIAS="🔴 SEVERE"
            elif [ $WHITE_PCT -ge 70 ]; then
                BIAS="🟠 HIGH"
            elif [ $WHITE_PCT -ge 60 ]; then
                BIAS="🟡 MODERATE"
            else
                BIAS="🟢 NORMAL"
            fi
            
            printf "  %-20s: W:%3d%% B:%3d%% D:%3d%% - %s\n" "$STAGE_NAME" "$WHITE_PCT" "$BLACK_PCT" "$DRAW_PCT" "$BIAS"
            printf "%-20s: W:%3d%% B:%3d%% D:%3d%% (%d/%d/%d) - %s\n" "$STAGE_NAME" "$WHITE_PCT" "$BLACK_PCT" "$DRAW_PCT" "$WHITE_WINS" "$BLACK_WINS" "$DRAWS" "$BIAS" >> "$RESULTS_FILE"
        else
            echo "  $STAGE_NAME: No games completed"
            echo "$STAGE_NAME: No games completed" >> "$RESULTS_FILE"
        fi
    else
        echo "  $STAGE_NAME: Test failed"
        echo "$STAGE_NAME: Test failed" >> "$RESULTS_FILE"
    fi
}

# Test each binary
echo "Running tests ($GAMES games each at $TC time control)..."
echo "This will take approximately $((${#BINARIES[@]} * 2)) minutes..."
echo ""
echo "Stage                 White% Black% Draw%  Bias Level"
echo "======================================================"

for BINARY_INFO in "${BINARIES[@]}"; do
    IFS=':' read -r BINARY_NAME STAGE_NAME <<< "$BINARY_INFO"
    test_binary "$BINARY_NAME" "$STAGE_NAME"
done

echo ""
echo "======================================================"
echo "Summary saved to: $RESULTS_FILE"
echo ""
echo "Legend:"
echo "🟢 NORMAL   = White wins <60% (expected)"
echo "🟡 MODERATE = White wins 60-69% (slight bias)"
echo "🟠 HIGH     = White wins 70-79% (significant bias)"
echo "🔴 SEVERE   = White wins 80%+ (critical bias)"
echo ""

# Display summary
echo "Final Summary:"
echo "=============="
cat "$RESULTS_FILE"