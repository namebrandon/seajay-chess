#!/bin/bash

# UCI-based test suite runner for SeaJay
# Usage: ./run_test_suite.sh [wac|bk|tactical] [depth] [movetime_ms]

SUITE=${1:-wac}
DEPTH=${2:-8}
MOVETIME=${3:-1000}
ENGINE="/workspace/bin/seajay"

# Check if engine exists
if [ ! -f "$ENGINE" ]; then
    echo "Error: Engine not found at $ENGINE"
    echo "Please run ./build_testing.sh first"
    exit 1
fi

# Select test suite file
case $SUITE in
    wac)
        EPD_FILE="/workspace/tests/positions/wac.epd"
        echo "Running WAC Test Suite (300 positions)"
        ;;
    bk)
        EPD_FILE="/workspace/tests/positions/bratko_kopec.epd"
        echo "Running Bratko-Kopec Test Suite (24 positions)"
        ;;
    tactical)
        EPD_FILE="/workspace/tests/positions/tactical_suite.epd"
        echo "Running Tactical Test Suite (42 positions)"
        ;;
    *)
        echo "Unknown suite: $SUITE"
        echo "Usage: $0 [wac|bk|tactical] [depth] [movetime_ms]"
        exit 1
        ;;
esac

# Check if EPD file exists
if [ ! -f "$EPD_FILE" ]; then
    echo "Error: Test file not found at $EPD_FILE"
    exit 1
fi

echo "Settings: Depth=$DEPTH, Movetime=${MOVETIME}ms"
echo "Engine: $ENGINE"
echo "----------------------------------------"

# Initialize counters
TOTAL=0
SOLVED=0
FAILED_POSITIONS=""

# Function to test a single position
test_position() {
    local fen="$1"
    local best_move="$2"
    local id="$3"
    
    # Send UCI commands to engine
    RESPONSE=$(echo -e "position fen $fen\ngo depth $DEPTH movetime $MOVETIME\nquit" | $ENGINE 2>/dev/null)
    
    # Extract best move from response
    FOUND_MOVE=$(echo "$RESPONSE" | grep "bestmove" | awk '{print $2}')
    
    # Compare moves (handle both formats)
    if [ "$FOUND_MOVE" = "$best_move" ] || [ "$FOUND_MOVE" = "${best_move}+" ] || [ "$FOUND_MOVE" = "${best_move}#" ]; then
        echo "✓ $id: SOLVED (found $FOUND_MOVE)"
        return 0
    else
        echo "✗ $id: FAILED (expected $best_move, found $FOUND_MOVE)"
        return 1
    fi
}

# Process EPD file
echo "Starting test run..."
echo ""

while IFS= read -r line; do
    # Skip comments and empty lines
    if [[ "$line" =~ ^# ]] || [ -z "$line" ]; then
        continue
    fi
    
    # Parse EPD line
    # Format: FEN bm <move>; id "name";
    FEN=$(echo "$line" | sed 's/ bm .*//')
    BEST_MOVE=$(echo "$line" | sed -n 's/.*bm \([^;]*\);.*/\1/p' | tr -d ' ')
    ID=$(echo "$line" | sed -n 's/.*id "\([^"]*\)".*/\1/p')
    
    if [ -z "$FEN" ] || [ -z "$BEST_MOVE" ]; then
        continue
    fi
    
    TOTAL=$((TOTAL + 1))
    
    # Test the position
    if test_position "$FEN" "$BEST_MOVE" "$ID"; then
        SOLVED=$((SOLVED + 1))
    else
        FAILED_POSITIONS="$FAILED_POSITIONS\n  $ID: $BEST_MOVE"
    fi
    
    # Show progress every 10 positions
    if [ $((TOTAL % 10)) -eq 0 ]; then
        echo "  Progress: $TOTAL positions tested, $SOLVED solved..."
    fi
    
done < "$EPD_FILE"

# Print summary
echo ""
echo "========================================"
echo "Test Suite Results"
echo "========================================"
echo "Suite: $SUITE"
echo "Total Positions: $TOTAL"
echo "Solved: $SOLVED"
echo "Failed: $((TOTAL - SOLVED))"
echo "Success Rate: $(echo "scale=1; $SOLVED * 100 / $TOTAL" | bc)%"
echo "Settings: Depth=$DEPTH, Time=${MOVETIME}ms"

if [ $((TOTAL - SOLVED)) -gt 0 ]; then
    echo ""
    echo "Failed positions:"
    echo -e "$FAILED_POSITIONS"
fi

echo ""
echo "Quiescence Search Statistics:"
$ENGINE << EOF 2>/dev/null | grep -E "qsearch|Quiescence|nodes"
uci
debug on
quit
EOF

exit 0