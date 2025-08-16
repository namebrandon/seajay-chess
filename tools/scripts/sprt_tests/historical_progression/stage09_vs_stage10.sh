#!/bin/bash
# Stage 9 vs Stage 10 Progression Test
# 200 games with diverse openings at TC 10+0.1

echo "========================================================"
echo "Stage 9 vs Stage 10 Progression Test"
echo "========================================================"
echo ""
echo "Stage 9:  PST evaluation added"
echo "Stage 10: Magic bitboards for move generation"
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
STAGE9="/workspace/binaries/seajay-stage9-fe33035"
STAGE10="/workspace/binaries/seajay-stage10-e3c59e2"

# Create output directory
mkdir -p "$OUTPUT_DIR"

# Check for fast-chess
FASTCHESS="/workspace/external/testers/fast-chess/fastchess"
if [ ! -f "$FASTCHESS" ]; then
    echo "ERROR: fast-chess not found. Run setup-external-tools.sh first"
    exit 1
fi

# Verify binaries
if [ ! -f "$STAGE9" ]; then
    echo "ERROR: Stage 9 binary not found: $STAGE9"
    exit 1
fi
if [ ! -f "$STAGE10" ]; then
    echo "ERROR: Stage 10 binary not found: $STAGE10"
    exit 1
fi

# Verify opening book
if [ ! -f "$OPENING_BOOK" ]; then
    echo "ERROR: Opening book not found: $OPENING_BOOK"
    exit 1
fi

# Display checksums
echo "Binary checksums:"
echo "  Stage 9:  $(md5sum $STAGE9 | awk '{print $1}')"
echo "  Stage 10: $(md5sum $STAGE10 | awk '{print $1}')"
echo ""

# Output files
PGN_FILE="$OUTPUT_DIR/stage09_vs_stage10.pgn"
LOG_FILE="$OUTPUT_DIR/stage09_vs_stage10.log"

echo "Starting match..."
echo "Output: $(basename $PGN_FILE)"
echo ""

# Run the match
"$FASTCHESS" \
    -engine cmd="$STAGE9" name="Stage09-PST" \
    -engine cmd="$STAGE10" name="Stage10-Magic" \
    -each tc="$TC" \
    -openings file="$OPENING_BOOK" format=pgn order=sequential \
    -games 2 -rounds $ROUNDS -repeat \
    -pgnout file="$PGN_FILE" \
    -log file="$LOG_FILE" level=info \
    -recover \
    -event "Stage 9 vs Stage 10 Progression Test" \
    -site "8moves_v3 Diverse Openings"

# Display results
echo ""
echo "========================================================"
echo "Match Complete!"
echo "========================================================"
echo ""

if [ -f "$LOG_FILE" ]; then
    # Extract final score
    SCORE_LINE=$(grep -E "Score of Stage09-PST vs Stage10-Magic" "$LOG_FILE" | tail -1)
    
    if [ -n "$SCORE_LINE" ]; then
        echo "Final Result:"
        echo "$SCORE_LINE"
        echo ""
        
        # Parse and display statistics
        if [[ "$SCORE_LINE" =~ ([0-9]+)[[:space:]]-[[:space:]]([0-9]+)[[:space:]]-[[:space:]]([0-9]+) ]]; then
            S9_WINS="${BASH_REMATCH[1]}"
            S10_WINS="${BASH_REMATCH[2]}"
            DRAWS="${BASH_REMATCH[3]}"
            
            TOTAL=$((S9_WINS + S10_WINS + DRAWS))
            
            echo "Statistics:"
            echo "  Stage 9 wins:  $S9_WINS ($(( S9_WINS * 100 / TOTAL ))%)"
            echo "  Stage 10 wins: $S10_WINS ($(( S10_WINS * 100 / TOTAL ))%)"
            echo "  Draws:         $DRAWS ($(( DRAWS * 100 / TOTAL ))%)"
            
            # Simple Elo estimate
            if [ $((S9_WINS + S10_WINS)) -gt 0 ]; then
                WIN_RATE=$(echo "scale=1; $S10_WINS * 100 / ($S9_WINS + $S10_WINS)" | bc)
                echo ""
                echo "Stage 10 win rate (excl. draws): ${WIN_RATE}%"
                
                # Rough Elo difference estimate
                if (( $(echo "$WIN_RATE > 55" | bc -l) )); then
                    echo "Stage 10 appears stronger (magic bitboards improvement)"
                elif (( $(echo "$WIN_RATE < 45" | bc -l) )); then
                    echo "Stage 9 appears stronger (unexpected result)"
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