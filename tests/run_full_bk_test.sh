#!/bin/bash

# Full Bratko-Kopec Test Suite Runner with Stockfish Validation
# Tests all 24 BK positions and cross-references with Stockfish

ENGINE="/workspace/bin/seajay"
STOCKFISH="/workspace/external/engines/stockfish/stockfish"
EPD_FILE="/workspace/tests/positions/bratko_kopec.epd"

# Check if engines exist
if [ ! -f "$ENGINE" ]; then
    echo "Error: SeaJay not found. Run ./build_testing.sh first"
    exit 1
fi

if [ ! -f "$STOCKFISH" ]; then
    echo "Setting up Stockfish..."
    /workspace/tools/scripts/setup-external-tools.sh 2>&1 | grep -E "Stockfish|Download"
fi

# Parameters
DEPTH=${1:-6}
SF_DEPTH=${2:-15}

echo "============================================"
echo "Bratko-Kopec Full Test Suite (24 positions)"
echo "============================================"
echo "SeaJay depth: $DEPTH"
echo "Stockfish validation depth: $SF_DEPTH"
echo "============================================"
echo ""

# Initialize counters
TOTAL=0
SEAJAY_CORRECT=0
STOCKFISH_AGREES_WITH_BK=0
SEAJAY_MATCHES_SF=0
RESULTS=""

# Process each position
while IFS= read -r line; do
    # Skip comments and empty lines
    if [[ "$line" =~ ^# ]] || [ -z "$line" ]; then
        continue
    fi
    
    # Parse EPD line
    FEN=$(echo "$line" | sed 's/ bm .*//')
    EXPECTED=$(echo "$line" | sed -n 's/.*bm \([^;]*\);.*/\1/p' | tr -d ' ' | awk '{print $1}')
    ID=$(echo "$line" | sed -n 's/.*id "\([^"]*\)".*/\1/p')
    
    if [ -z "$FEN" ] || [ -z "$EXPECTED" ]; then
        continue
    fi
    
    TOTAL=$((TOTAL + 1))
    
    # Test with SeaJay (add move counters if missing)
    if [[ ! "$FEN" =~ [0-9]+$ ]]; then
        FEN="$FEN 0 1"
    fi
    SEAJAY_MOVE=$(echo -e "position fen $FEN\ngo depth $DEPTH\nquit" | $ENGINE 2>/dev/null | grep "bestmove" | awk '{print $2}')
    
    # Test with Stockfish
    SF_MOVE=$(echo -e "position fen $FEN\ngo depth $SF_DEPTH\nquit" | $STOCKFISH 2>/dev/null | grep "bestmove" | awk '{print $2}')
    
    # Analyze results
    SEAJAY_STATUS="✗"
    if [ "$SEAJAY_MOVE" = "$EXPECTED" ]; then
        SEAJAY_STATUS="✓"
        SEAJAY_CORRECT=$((SEAJAY_CORRECT + 1))
    fi
    
    SF_STATUS="✗"
    if [ "$SF_MOVE" = "$EXPECTED" ]; then
        SF_STATUS="✓"
        STOCKFISH_AGREES_WITH_BK=$((STOCKFISH_AGREES_WITH_BK + 1))
    fi
    
    MATCH_STATUS=""
    if [ "$SEAJAY_MOVE" = "$SF_MOVE" ]; then
        MATCH_STATUS="="
        SEAJAY_MATCHES_SF=$((SEAJAY_MATCHES_SF + 1))
    fi
    
    # Store result
    RESULT_LINE=$(printf "%-8s | %-12s | %-12s | %-12s | %s %s %s" \
        "$ID" "$EXPECTED" "$SEAJAY_MOVE" "$SF_MOVE" "$SEAJAY_STATUS" "$SF_STATUS" "$MATCH_STATUS")
    RESULTS="$RESULTS\n$RESULT_LINE"
    
    # Progress indicator
    echo -n "."
    if [ $((TOTAL % 6)) -eq 0 ]; then
        echo " $TOTAL/24"
    fi
done < "$EPD_FILE"

echo ""
echo ""
echo "============================================"
echo "Detailed Results"
echo "============================================"
echo "Position | BK Expected  | SeaJay Found | Stockfish    | Status"
echo "---------|--------------|--------------|--------------|--------"
echo -e "$RESULTS"

echo ""
echo "============================================"
echo "Summary Statistics"
echo "============================================"
echo "Total Positions:                     24"
echo "SeaJay matches BK expected:          $SEAJAY_CORRECT/24 ($(echo "scale=1; $SEAJAY_CORRECT * 100 / 24" | bc)%)"
echo "Stockfish agrees with BK:            $STOCKFISH_AGREES_WITH_BK/24 ($(echo "scale=1; $STOCKFISH_AGREES_WITH_BK * 100 / 24" | bc)%)"
echo "SeaJay matches Stockfish:            $SEAJAY_MATCHES_SF/24 ($(echo "scale=1; $SEAJAY_MATCHES_SF * 100 / 24" | bc)%)"
echo ""

if [ $STOCKFISH_AGREES_WITH_BK -lt 20 ]; then
    echo "⚠️  WARNING: Stockfish disagrees with many BK expected moves."
    echo "    The BK test suite may have outdated analysis."
    echo "    Consider SeaJay vs Stockfish agreement more important."
fi

echo ""
echo "============================================"
echo "Performance Analysis"
echo "============================================"

# Test one position to show quiescence extension
SAMPLE_FEN="1k1r4/pp1b1R2/3q2pp/4p3/2B5/4Q3/PPP2B2/2K5 b - -"
echo "Sample position (BK.01) quiescence analysis:"
RESULT=$(echo -e "position fen $SAMPLE_FEN\ngo depth 6\nquit" | $ENGINE 2>/dev/null | grep "depth 6" | tail -1)
DEPTH=$(echo "$RESULT" | grep -oP "depth \K\d+")
SELDEPTH=$(echo "$RESULT" | grep -oP "seldepth \K\d+")
if [ -n "$SELDEPTH" ] && [ -n "$DEPTH" ]; then
    echo "Depth: $DEPTH, Selective depth: $SELDEPTH (extension: $((SELDEPTH - DEPTH)) ply)"
fi

echo ""
echo "Build mode:"
$ENGINE 2>/dev/null | head -3 | grep -E "Quiescence|Stage"

exit 0