#!/bin/bash

# eval_position.sh - Evaluate a chess position with detailed breakdown
# Usage: ./eval_position.sh "FEN_STRING"
#        ./eval_position.sh startpos
#        ./eval_position.sh "startpos moves e2e4 e7e5"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
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
    echo "  $0 \"FEN_STRING\""
    echo "  $0 startpos"
    echo "  $0 \"startpos moves e2e4 e7e5\""
    echo ""
    echo -e "${GREEN}Examples:${NC}"
    echo "  $0 \"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1\""
    echo "  $0 \"r1bqkbnr/pppp1ppp/2n5/4p3/4P3/5N2/PPPP1PPP/RNBQKB1R w KQkq - 4 4\""
    echo "  $0 startpos"
    echo "  $0 \"startpos moves e2e4 e7e5 g1f3\""
    exit 1
fi

# Determine position command based on input
POSITION_CMD=""
if [ "$1" == "startpos" ]; then
    POSITION_CMD="position startpos"
elif [[ "$1" == startpos* ]]; then
    # Handle "startpos moves ..." format
    POSITION_CMD="position $1"
else
    # Assume it's a FEN string
    POSITION_CMD="position fen $1"
fi

# Create UCI command sequence
UCI_COMMANDS=$(cat <<EOF
uci
setoption name EvalExtended value true
$POSITION_CMD
d eval
quit
EOF
)

# Show what position we're evaluating
echo -e "${GREEN}Evaluating position:${NC} $1"
echo -e "${YELLOW}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
echo ""

# Run the evaluation, filter out UCI initialization output
echo "$UCI_COMMANDS" | $BINARY 2>/dev/null | \
    sed -n '/=== Evaluation Breakdown ===/,/===========================/p'

# Add a note about the scoring convention
echo ""
echo -e "${YELLOW}Note:${NC} Scores are from the side-to-move perspective."
echo "      Positive = side to move is winning"
echo "      Negative = side to move is losing"