#!/bin/bash
# Historical Progression Quick Test - Stage 9 through Stage 15
# Quick version with only 20 games per match for faster results
# Use the full test for statistically significant results

echo "========================================================"
echo "Historical Progression QUICK Test - Stages 9-15"
echo "========================================================"
echo ""
echo "QUICK TEST: Only 20 games per match"
echo "For statistically significant results, use:"
echo "  ./historical_progression_test.sh"
echo ""
echo "Using 8moves_v3.pgn for opening diversity"
echo "Time control: 10+0.1"
echo ""

# Configuration
OUTPUT_DIR="/workspace/tools/scripts/sprt_tests/white_bias_investigation/historical_progression_quick"
OPENING_BOOK="/workspace/external/books/8moves_v3.pgn"
TC="10+0.1"
GAMES=20
ROUNDS=$((GAMES / 2))  # 10 rounds x 2 games (color swap) = 20 games

# Create output directory
mkdir -p "$OUTPUT_DIR"

# Check for fast-chess
FASTCHESS="/workspace/external/testers/fast-chess/fastchess"
if [ ! -f "$FASTCHESS" ]; then
    echo "fast-chess not found, downloading..."
    /workspace/tools/scripts/setup-external-tools.sh
fi

# Verify opening book exists
if [ ! -f "$OPENING_BOOK" ]; then
    echo "ERROR: Opening book not found: $OPENING_BOOK"
    exit 1
fi

# Define binaries for each stage
STAGE9="/workspace/binaries/seajay-stage9-fe33035"
STAGE10="/workspace/binaries/seajay-stage10-e3c59e2"
STAGE11="/workspace/binaries/seajay-stage11-f2ad9b5"
STAGE12="/workspace/binaries/seajay-stage12-ffa0e44"
STAGE13="/workspace/binaries/seajay-stage13-869495e"
STAGE14="/workspace/binaries/seajay-stage14-final"
STAGE15="/workspace/binaries/seajay-stage15-properly-fixed"

# Verify all binaries exist
echo "Verifying binaries..."
for binary in "$STAGE9" "$STAGE10" "$STAGE11" "$STAGE12" "$STAGE13" "$STAGE14" "$STAGE15"; do
    if [ ! -f "$binary" ]; then
        echo "ERROR: Binary not found: $binary"
        exit 1
    fi
done

echo "All binaries verified. Starting quick tests..."
echo ""

# Function to run a match between two stages
run_quick_match() {
    local stage1=$1
    local stage1_name=$2
    local stage2=$3
    local stage2_name=$4
    
    local pgn_file="$OUTPUT_DIR/${stage1_name,,}_vs_${stage2_name,,}.pgn"
    local log_file="$OUTPUT_DIR/${stage1_name,,}_vs_${stage2_name,,}.log"
    
    echo "Testing: $stage1_name vs $stage2_name (20 games)..."
    
    # Run the match
    "$FASTCHESS" \
        -engine cmd="$stage1" name="$stage1_name" \
        -engine cmd="$stage2" name="$stage2_name" \
        -each tc="$TC" \
        -openings file="$OPENING_BOOK" format=pgn order=sequential \
        -games 2 -rounds $ROUNDS -repeat \
        -pgnout file="$pgn_file" \
        -log file="$log_file" level=info \
        -recover \
        -event "$stage1_name vs $stage2_name Quick Test" \
        -site "8moves_v3 Opening Book" \
        2>/dev/null
    
    # Quick results
    if [ -f "$log_file" ]; then
        SCORE_LINE=$(grep -E "Score of $stage1_name vs $stage2_name" "$log_file" | tail -1)
        if [ -n "$SCORE_LINE" ]; then
            echo "  Result: $SCORE_LINE"
        else
            echo "  [Match in progress or failed]"
        fi
    fi
}

# Run all progression tests
echo "------------------------------------------------------"
run_quick_match "$STAGE9" "Stage9" "$STAGE10" "Stage10"
run_quick_match "$STAGE10" "Stage10" "$STAGE11" "Stage11"
run_quick_match "$STAGE11" "Stage11" "$STAGE12" "Stage12"
run_quick_match "$STAGE12" "Stage12" "$STAGE13" "Stage13"
run_quick_match "$STAGE13" "Stage13" "$STAGE14" "Stage14"
run_quick_match "$STAGE14" "Stage14" "$STAGE15" "Stage15"

# Summary
echo ""
echo "========================================================"
echo "Quick Test Complete"
echo "========================================================"
echo ""
echo "Quick Summary:"
echo "------------------------------------------------------"

for log in "$OUTPUT_DIR"/*.log; do
    if [ -f "$log" ]; then
        match=$(basename "$log" .log | sed 's/_/ /g')
        score=$(grep -E "Score of Stage[0-9]+ vs Stage[0-9]+" "$log" | tail -1 | sed 's/.*Score of //')
        if [ -n "$score" ]; then
            echo "$match: $score"
        fi
    fi
done

echo ""
echo "Results saved to: $OUTPUT_DIR"
echo ""
echo "Note: 20 games is NOT statistically significant!"
echo "Run ./historical_progression_test.sh for 200 games per match"