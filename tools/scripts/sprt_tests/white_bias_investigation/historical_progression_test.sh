#!/bin/bash
# Historical Progression Test - Stage 9 through Stage 15
# Tests each stage against the next using diverse opening book
# 200 games per match at TC 10+0.1

echo "========================================================"
echo "Historical Progression Test - Stages 9-15"
echo "========================================================"
echo ""
echo "Testing progression between consecutive stages"
echo "Using 8moves_v3.pgn for opening diversity"
echo "200 games per match (100 with each color)"
echo "Time control: 10+0.1"
echo ""

# Configuration
OUTPUT_DIR="/workspace/tools/scripts/sprt_tests/white_bias_investigation/historical_progression"
OPENING_BOOK="/workspace/external/books/8moves_v3.pgn"
TC="10+0.1"
GAMES=200
ROUNDS=$((GAMES / 2))  # 100 rounds x 2 games (color swap) = 200 games

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
for binary in "$STAGE9" "$STAGE10" "$STAGE11" "$STAGE12" "$STAGE13" "$STAGE14" "$STAGE15"; do
    if [ ! -f "$binary" ]; then
        echo "ERROR: Binary not found: $binary"
        exit 1
    fi
done

echo "All binaries verified. Starting tests..."
echo ""
echo "========================================================"

# Function to run a match between two stages
run_match() {
    local stage1=$1
    local stage1_name=$2
    local stage2=$3
    local stage2_name=$4
    local pgn_file=$5
    local log_file=$6
    
    echo ""
    echo "------------------------------------------------------"
    echo "Testing: $stage1_name vs $stage2_name"
    echo "------------------------------------------------------"
    
    # Display binary checksums
    echo "Binaries:"
    echo "  $stage1_name: $(md5sum $stage1 | awk '{print $1}')"
    echo "  $stage2_name: $(md5sum $stage2 | awk '{print $1}')"
    echo ""
    
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
        -event "$stage1_name vs $stage2_name Progression Test" \
        -site "8moves_v3 Opening Book"
    
    # Extract and display results
    echo ""
    echo "Results for $stage1_name vs $stage2_name:"
    echo "------------------------------------------------------"
    
    if [ -f "$log_file" ]; then
        # Look for the final score line
        SCORE_LINE=$(grep -E "Score of $stage1_name vs $stage2_name" "$log_file" | tail -1)
        
        if [ -n "$SCORE_LINE" ]; then
            echo "$SCORE_LINE"
            
            # Extract wins/losses/draws
            if [[ "$SCORE_LINE" =~ ([0-9]+)[[:space:]]-[[:space:]]([0-9]+)[[:space:]]-[[:space:]]([0-9]+) ]]; then
                WINS="${BASH_REMATCH[1]}"
                LOSSES="${BASH_REMATCH[2]}"
                DRAWS="${BASH_REMATCH[3]}"
                
                TOTAL=$((WINS + LOSSES + DRAWS))
                if [ $TOTAL -gt 0 ]; then
                    WIN_PCT=$((WINS * 100 / TOTAL))
                    LOSS_PCT=$((LOSSES * 100 / TOTAL))
                    DRAW_PCT=$((DRAWS * 100 / TOTAL))
                    
                    echo ""
                    echo "Statistics:"
                    echo "  $stage1_name wins: $WINS ($WIN_PCT%)"
                    echo "  $stage2_name wins: $LOSSES ($LOSS_PCT%)"
                    echo "  Draws: $DRAWS ($DRAW_PCT%)"
                    
                    # Calculate Elo difference estimate (simplified)
                    if [ $((WINS + LOSSES)) -gt 0 ]; then
                        SCORE_PCT=$((WINS * 100 / (WINS + LOSSES)))
                        echo ""
                        echo "  Win rate for $stage1_name: $SCORE_PCT% (excluding draws)"
                    fi
                fi
            fi
        else
            echo "  [Results pending or match incomplete]"
        fi
    else
        echo "  [Log file not found]"
    fi
    
    echo ""
}

# Run all progression tests
echo ""
echo "Starting all progression tests..."
echo "This will take several hours to complete."
echo ""

# Stage 9 vs 10
run_match "$STAGE9" "Stage9" "$STAGE10" "Stage10" \
    "$OUTPUT_DIR/stage9_vs_stage10.pgn" \
    "$OUTPUT_DIR/stage9_vs_stage10.log"

# Stage 10 vs 11
run_match "$STAGE10" "Stage10" "$STAGE11" "Stage11" \
    "$OUTPUT_DIR/stage10_vs_stage11.pgn" \
    "$OUTPUT_DIR/stage10_vs_stage11.log"

# Stage 11 vs 12
run_match "$STAGE11" "Stage11" "$STAGE12" "Stage12" \
    "$OUTPUT_DIR/stage11_vs_stage12.pgn" \
    "$OUTPUT_DIR/stage11_vs_stage12.log"

# Stage 12 vs 13
run_match "$STAGE12" "Stage12" "$STAGE13" "Stage13" \
    "$OUTPUT_DIR/stage12_vs_stage13.pgn" \
    "$OUTPUT_DIR/stage12_vs_stage13.log"

# Stage 13 vs 14
run_match "$STAGE13" "Stage13" "$STAGE14" "Stage14" \
    "$OUTPUT_DIR/stage13_vs_stage14.pgn" \
    "$OUTPUT_DIR/stage13_vs_stage14.log"

# Stage 14 vs 15
run_match "$STAGE14" "Stage14" "$STAGE15" "Stage15" \
    "$OUTPUT_DIR/stage14_vs_stage15.pgn" \
    "$OUTPUT_DIR/stage14_vs_stage15.log"

# Final summary
echo ""
echo "========================================================"
echo "Historical Progression Test Complete"
echo "========================================================"
echo ""
echo "All results saved to: $OUTPUT_DIR"
echo ""
echo "Summary of all matches:"
echo "------------------------------------------------------"

for log in "$OUTPUT_DIR"/*.log; do
    if [ -f "$log" ]; then
        match=$(basename "$log" .log)
        echo ""
        echo "$match:"
        grep -E "Score of Stage[0-9]+ vs Stage[0-9]+" "$log" | tail -1 || echo "  [No results]"
    fi
done

echo ""
echo "PGN files:"
ls -lh "$OUTPUT_DIR"/*.pgn 2>/dev/null || echo "No PGN files generated yet"

echo ""
echo "To analyze specific games, use:"
echo "  pgn-extract $OUTPUT_DIR/<stage_vs_stage>.pgn"
echo ""
echo "To run additional analysis:"
echo "  grep \"Result\" $OUTPUT_DIR/*.pgn | sort | uniq -c"