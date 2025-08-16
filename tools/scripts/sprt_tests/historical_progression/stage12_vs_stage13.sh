#!/bin/bash
# Stage 12 vs Stage 13 Progression Test
# 200 games with diverse openings at TC 10+0.1

echo "========================================================"
echo "Stage 12 vs Stage 13 Progression Test"
echo "========================================================"
echo ""
echo "Stage 12: Transposition tables"
echo "Stage 13: Iterative deepening"
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
STAGE12="/workspace/binaries/seajay-stage12-ffa0e44"
STAGE13="/workspace/binaries/seajay-stage13-869495e"

# Create output directory
mkdir -p "$OUTPUT_DIR"

# Check for fast-chess
FASTCHESS="/workspace/external/testers/fast-chess/fastchess"
if [ ! -f "$FASTCHESS" ]; then
    echo "ERROR: fast-chess not found. Run setup-external-tools.sh first"
    exit 1
fi

# Verify binaries
if [ ! -f "$STAGE12" ]; then
    echo "ERROR: Stage 12 binary not found: $STAGE12"
    exit 1
fi
if [ ! -f "$STAGE13" ]; then
    echo "ERROR: Stage 13 binary not found: $STAGE13"
    exit 1
fi

# Verify opening book
if [ ! -f "$OPENING_BOOK" ]; then
    echo "ERROR: Opening book not found: $OPENING_BOOK"
    exit 1
fi

# Display checksums
echo "Binary checksums:"
echo "  Stage 12: $(md5sum $STAGE12 | awk '{print $1}')"
echo "  Stage 13: $(md5sum $STAGE13 | awk '{print $1}')"
echo ""

# Output files
PGN_FILE="$OUTPUT_DIR/stage12_vs_stage13.pgn"
LOG_FILE="$OUTPUT_DIR/stage12_vs_stage13.log"

echo "Starting match..."
echo "Output: $(basename $PGN_FILE)"
echo ""

# Run the match
"$FASTCHESS" \
    -engine cmd="$STAGE12" name="Stage12-TT" \
    -engine cmd="$STAGE13" name="Stage13-ID" \
    -each tc="$TC" \
    -openings file="$OPENING_BOOK" format=pgn order=sequential \
    -games 2 -rounds $ROUNDS -repeat \
    -pgnout file="$PGN_FILE" \
    -log file="$LOG_FILE" level=info \
    -recover \
    -event "Stage 12 vs Stage 13 Progression Test" \
    -site "8moves_v3 Diverse Openings"

# Display results
echo ""
echo "========================================================"
echo "Match Complete!"
echo "========================================================"
echo ""

if [ -f "$LOG_FILE" ]; then
    # Extract final score
    SCORE_LINE=$(grep -E "Score of Stage12-TT vs Stage13-ID" "$LOG_FILE" | tail -1)
    
    if [ -n "$SCORE_LINE" ]; then
        echo "Final Result:"
        echo "$SCORE_LINE"
        echo ""
        
        # Parse and display statistics
        if [[ "$SCORE_LINE" =~ ([0-9]+)[[:space:]]-[[:space:]]([0-9]+)[[:space:]]-[[:space:]]([0-9]+) ]]; then
            S12_WINS="${BASH_REMATCH[1]}"
            S13_WINS="${BASH_REMATCH[2]}"
            DRAWS="${BASH_REMATCH[3]}"
            
            TOTAL=$((S12_WINS + S13_WINS + DRAWS))
            
            echo "Statistics:"
            echo "  Stage 12 wins: $S12_WINS ($(( S12_WINS * 100 / TOTAL ))%)"
            echo "  Stage 13 wins: $S13_WINS ($(( S13_WINS * 100 / TOTAL ))%)"
            echo "  Draws:         $DRAWS ($(( DRAWS * 100 / TOTAL ))%)"
            
            # Simple Elo estimate
            if [ $((S12_WINS + S13_WINS)) -gt 0 ]; then
                WIN_RATE=$(echo "scale=1; $S13_WINS * 100 / ($S12_WINS + $S13_WINS)" | bc)
                echo ""
                echo "Stage 13 win rate (excl. draws): ${WIN_RATE}%"
                
                if (( $(echo "$WIN_RATE > 55" | bc -l) )); then
                    echo "Stage 13 appears stronger (Iterative deepening)"
                elif (( $(echo "$WIN_RATE < 45" | bc -l) )); then
                    echo "Stage 12 appears stronger (unexpected result)"
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
