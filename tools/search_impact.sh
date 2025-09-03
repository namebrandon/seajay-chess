#!/bin/bash

# search_impact.sh - Analyze how search impacts evaluation at different depths
# Usage: ./search_impact.sh "FEN_STRING" [max_depth]

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
MAGENTA='\033[0;35m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

# Check if seajay binary exists
BINARY="./bin/seajay"
if [ ! -f "$BINARY" ]; then
    BINARY="../bin/seajay"
    if [ ! -f "$BINARY" ]; then
        echo -e "${RED}Error: seajay binary not found!${NC}"
        echo "Please build the project first with: ./build.sh Release"
        exit 1
    fi
fi

# Check for input
if [ $# -eq 0 ]; then
    echo -e "${YELLOW}Usage:${NC}"
    echo "  $0 \"FEN_STRING\" [max_depth]"
    echo "  $0 startpos [max_depth]"
    echo ""
    echo -e "${GREEN}Examples:${NC}"
    echo "  $0 \"r1b1k2r/pp4pp/3Bpp2/3p4/6q1/8/PQ3PPP/2R1R1K1 b kq - 3 17\" 10"
    echo "  $0 startpos 8"
    exit 1
fi

# Parse arguments
POSITION="$1"
MAX_DEPTH=${2:-10}  # Default to depth 10 if not specified

# Determine position command based on input
if [ "$POSITION" == "startpos" ]; then
    POSITION_CMD="position startpos"
elif [[ "$POSITION" == startpos* ]]; then
    POSITION_CMD="position $POSITION"
else
    POSITION_CMD="position fen $POSITION"
fi

echo -e "${CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
echo -e "${GREEN}Search Impact Analysis${NC}"
echo -e "${CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
echo -e "${YELLOW}Position:${NC} $POSITION"
echo ""

# First, get the static evaluation
echo -e "${MAGENTA}Getting static evaluation...${NC}"
STATIC_EVAL=$(echo -e "uci\nsetoption name EvalExtended value true\n$POSITION_CMD\nd eval\nquit" | \
    $BINARY 2>/dev/null | \
    grep "From .* perspective:" | \
    sed -E 's/.*perspective: ([+-]?[0-9.]+) cp/\1/')

if [ -z "$STATIC_EVAL" ]; then
    echo -e "${RED}Error: Could not get static evaluation${NC}"
    exit 1
fi

# Determine which side to move for proper sign interpretation
SIDE_TO_MOVE=$(echo -e "uci\n$POSITION_CMD\nd\nquit" | \
    $BINARY 2>/dev/null | \
    grep "Side to move:" | \
    awk '{print $NF}')

echo -e "${BLUE}Static Evaluation:${NC} ${STATIC_EVAL} cp (from $SIDE_TO_MOVE's perspective)"
echo ""
echo -e "${CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
echo -e "Depth │ Search Eval │ Difference │ Impact  │ Best Move"
echo -e "━━━━━━┼━━━━━━━━━━━━━┼━━━━━━━━━━━━┼━━━━━━━━━┼━━━━━━━━━━━━━━"

# Function to extract score and move from UCI output
analyze_depth() {
    local depth=$1
    local output=$(echo -e "uci\n$POSITION_CMD\ngo depth $depth\nquit" | \
        $BINARY 2>/dev/null | \
        grep "info depth $depth " | \
        tail -1)
    
    # Extract score (handling both cp and mate scores)
    local score_cp=$(echo "$output" | \
        sed -n 's/.*score cp \([+-]*[0-9]*\).*/\1/p')
    
    if [ -z "$score_cp" ]; then
        # Check for mate score
        score_cp=$(echo "$output" | \
            sed -n 's/.*score mate \([+-]*[0-9]*\).*/M\1/p')
    fi
    
    # Convert centipawns to pawns with 2 decimal places if it's a cp score
    if [[ "$score_cp" != M* ]] && [ ! -z "$score_cp" ]; then
        # Use awk for floating point math since bc might not be available
        score=$(echo "$score_cp" | awk '{printf "%.2f", $1/100.0}')
    else
        score="$score_cp"
    fi
    
    # Extract best move
    local move=$(echo "$output" | \
        sed -n 's/.*pv \([a-h][1-8][a-h][1-8][qrbn]*\).*/\1/p')
    
    echo "$score|$move"
}

# Analyze each depth
for depth in $(seq 1 $MAX_DEPTH); do
    result=$(analyze_depth $depth)
    search_score=$(echo "$result" | cut -d'|' -f1)
    best_move=$(echo "$result" | cut -d'|' -f2)
    
    if [[ "$search_score" == M* ]]; then
        # Handle mate scores
        diff="MATE"
        impact="FORCED"
        printf "  %2d  │ %11s │ %10s │ %7s │ %s\n" \
            "$depth" "$search_score" "$diff" "$impact" "$best_move"
    elif [ ! -z "$search_score" ]; then
        # Calculate difference from static eval using awk
        diff=$(echo "$search_score $STATIC_EVAL" | awk '{printf "%.2f", $1 - $2}')
        
        # Determine impact description using awk for comparisons
        diff_abs=$(echo "$diff" | awk '{print ($1 < 0) ? -$1 : $1}')
        
        if [ "$(echo "$diff" | awk '{print ($1 > 0.50)}')" = "1" ]; then
            impact="${GREEN}↑↑ HIGH${NC}"
        elif [ "$(echo "$diff" | awk '{print ($1 > 0.20)}')" = "1" ]; then
            impact="${GREEN}↑ MEDIUM${NC}"
        elif [ "$(echo "$diff" | awk '{print ($1 > 0.05)}')" = "1" ]; then
            impact="${YELLOW}↑ LOW${NC}"
        elif [ "$(echo "$diff" | awk '{print ($1 < -0.50)}')" = "1" ]; then
            impact="${RED}↓↓ HIGH${NC}"
        elif [ "$(echo "$diff" | awk '{print ($1 < -0.20)}')" = "1" ]; then
            impact="${RED}↓ MEDIUM${NC}"
        elif [ "$(echo "$diff" | awk '{print ($1 < -0.05)}')" = "1" ]; then
            impact="${YELLOW}↓ LOW${NC}"
        else
            impact="MINIMAL"
        fi
        
        # Format difference with sign
        if [ "$(echo "$diff" | awk '{print ($1 >= 0)}')" = "1" ]; then
            diff_str="+${diff}"
        else
            diff_str="${diff}"
        fi
        
        printf "  %2d  │ %+11.2f │ %10s │ " "$depth" "$search_score" "$diff_str"
        echo -ne "$impact"
        printf " │ %s\n" "$best_move"
    fi
done

echo -e "${CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
echo ""
echo -e "${YELLOW}Legend:${NC}"
echo "  Impact shows how much search changes the evaluation:"
echo "  ↑↑ HIGH   = Search finds position much better (+50+ cp)"
echo "  ↑ MEDIUM  = Search finds position better (+20 to +50 cp)"
echo "  ↑ LOW     = Search finds position slightly better (+5 to +20 cp)"
echo "  MINIMAL   = Search confirms static eval (±5 cp)"
echo "  ↓ LOW     = Search finds position slightly worse (-5 to -20 cp)"
echo "  ↓ MEDIUM  = Search finds position worse (-20 to -50 cp)"
echo "  ↓↓ HIGH   = Search finds position much worse (-50+ cp)"