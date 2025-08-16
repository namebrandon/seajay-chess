#!/bin/bash
# Stage 14 vs Stage 15 Progression Test
# 200 games with diverse openings at TC 10+0.1

echo "========================================================"
echo "Stage 14 vs Stage 15 Progression Test"
echo "========================================================"
echo ""
echo "Stage 14: Quiescence search"
echo "Stage 15: SEE parameter tuning"
echo ""
echo "Using 8moves_v3.pgn for opening diversity"
echo "200 games (100 with each color)"
echo "Time control: 10+0.1"
echo ""

# Configuration
OUTPUT_DIR="/workspace/tools/scripts/sprt_tests/historical_progression/results"
OPENING_BOOK="/workspace/external/books/8moves_v3.pgn"
TC="10+0.1"
GAMES=200
ROUNDS=$((GAMES / 2))

# Binaries
STAGE14="/workspace/binaries/seajay-stage14-final"
STAGE15="/workspace/binaries/seajay-stage15-properly-fixed"

# Create output directory
mkdir -p "$OUTPUT_DIR"

# Check for fast-chess
FASTCHESS="/workspace/external/testers/fast-chess/fastchess"
if [ ! -f "$FASTCHESS" ]; then
    echo "ERROR: fast-chess not found. Run setup-external-tools.sh first"
    exit 1
fi

# Verify binaries
if [ ! -f "$STAGE14" ]; then
    echo "ERROR: Stage 14 binary not found: $STAGE14"
    exit 1
fi
if [ ! -f "$STAGE15" ]; then
    echo "ERROR: Stage 15 binary not found: $STAGE15"
    exit 1
fi

# Verify opening book
if [ ! -f "$OPENING_BOOK" ]; then
    echo "ERROR: Opening book not found: $OPENING_BOOK"
    exit 1
fi

# Display checksums
echo "Binary checksums:"
echo "  Stage 14: $(md5sum $STAGE14 | awk '{print $1}')"
echo "  Stage 15: $(md5sum $STAGE15 | awk '{print $1}')"
echo ""

# Output files
PGN_FILE="$OUTPUT_DIR/stage14_vs_stage15.pgn"
LOG_FILE="$OUTPUT_DIR/stage14_vs_stage15.log"

echo "Starting match..."
echo "Output: $(basename $PGN_FILE)"
echo ""

# Run the match
"$FASTCHESS" \
    -engine cmd="$STAGE14" name="Stage14-QS" \
    -engine cmd="$STAGE15" name="Stage15-Tuned" \
    -each tc="$TC" \
    -openings file="$OPENING_BOOK" format=pgn order=sequential \
    -games 2 -rounds $ROUNDS -repeat \
    -pgnout file="$PGN_FILE" \
    -log file="$LOG_FILE" level=info \
    -recover \
    -event "Stage 14 vs Stage 15 Progression Test" \
    -site "8moves_v3 Diverse Openings"

# Display results
echo ""
echo "========================================================"
echo "Match Complete!"
echo "========================================================"
echo ""

if [ -f "$LOG_FILE" ]; then
    # Extract final score
    SCORE_LINE=$(grep -E "Score of Stage14-QS vs Stage15-Tuned" "$LOG_FILE" | tail -1)
    
    if [ -n "$SCORE_LINE" ]; then
        echo "Final Result:"
        echo "$SCORE_LINE"
        echo ""
        
        # Parse and display statistics
        if [[ "$SCORE_LINE" =~ ([0-9]+)[[:space:]]-[[:space:]]([0-9]+)[[:space:]]-[[:space:]]([0-9]+) ]]; then
            S14_WINS="${BASH_REMATCH[1]}"
            S15_WINS="${BASH_REMATCH[2]}"
            DRAWS="${BASH_REMATCH[3]}"
            
            TOTAL=$((S14_WINS + S15_WINS + DRAWS))
            
            echo "Statistics:"
            echo "  Stage 14 wins: $S14_WINS ($(( S14_WINS * 100 / TOTAL ))%)"
            echo "  Stage 15 wins: $S15_WINS ($(( S15_WINS * 100 / TOTAL ))%)"
            echo "  Draws:         $DRAWS ($(( DRAWS * 100 / TOTAL ))%)"
            
            # Simple Elo estimate
            if [ $((S14_WINS + S15_WINS)) -gt 0 ]; then
                WIN_RATE=$(echo "scale=1; $S15_WINS * 100 / ($S14_WINS + $S15_WINS)" | bc)
                echo ""
                echo "Stage 15 win rate (excl. draws): ${WIN_RATE}%"
                
                if (( $(echo "$WIN_RATE > 55" | bc -l) )); then
                    echo "Stage 15 appears stronger (SEE parameter tuning)"
                elif (( $(echo "$WIN_RATE < 45" | bc -l) )); then
                    echo "Stage 14 appears stronger (unexpected result)"
                else
                    echo "Stages appear roughly equal in strength"
                fi
            fi
        fi
    fi
fi

echo ""
echo "Files saved:"
echo "  PGN: $PGN_FILE"
echo "  Log: $LOG_FILE"
echo ""
echo "To analyze games:"
echo "  less $PGN_FILE"
echo "  grep \"Result\" $PGN_FILE | sort | uniq -c"
