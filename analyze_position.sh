#!/bin/bash

# analyze_position.sh - Four-engine position analysis utility for SeaJay debugging
# Usage: ./analyze_position.sh "FEN" depth|time [value]
# Example: ./analyze_position.sh "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1" depth 10
# Example: ./analyze_position.sh "r1b1k2r/pp4pp/3Bpp2/3p4/6q1/8/PQ3PPP/2R1R1K1 b kq - 3 17" time 5

SEAJAY="/workspace/bin/seajay"
STOCKFISH="/workspace/external/engines/stockfish/stockfish"
STASH="/workspace/external/engines/stash-bot/stash"
LASER="/workspace/external/engines/laser/laser"

# Color codes for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
MAGENTA='\033[0;35m'
CYAN='\033[0;36m'
BOLD='\033[1m'
NC='\033[0m' # No Color

# Parse arguments
if [ $# -lt 2 ]; then
    echo "Usage: $0 \"FEN\" depth|time [value]"
    echo "  depth mode: $0 \"FEN\" depth 10"
    echo "  time mode:  $0 \"FEN\" time 5"
    exit 1
fi

FEN="$1"
MODE="$2"
VALUE="${3:-10}"  # Default depth 10 or time 10 seconds

# Validate mode
if [ "$MODE" != "depth" ] && [ "$MODE" != "time" ]; then
    echo "Error: Mode must be 'depth' or 'time'"
    exit 1
fi

# Extract side to move from FEN
SIDE_TO_MOVE=$(echo "$FEN" | awk '{print $2}')
if [ "$SIDE_TO_MOVE" = "w" ]; then
    SIDE_TEXT="White"
    SIDE_COLOR="$BOLD"
else
    SIDE_TEXT="Black"
    SIDE_COLOR="$CYAN"
fi

echo -e "${BOLD}=========================================="
echo -e "Four-Engine Position Analysis"
echo -e "==========================================${NC}"
echo
echo -e "${BOLD}FEN:${NC} $FEN"
echo -e "${BOLD}Side to move:${NC} ${SIDE_COLOR}$SIDE_TEXT${NC}"
echo -e "${BOLD}Search mode:${NC} $MODE = $VALUE"
echo

# Function to interpret score from White's perspective
interpret_score() {
    local score=$1
    local abs_score=${score#-}  # Remove negative sign if present
    
    if [ "$score" -eq 0 ] 2>/dev/null; then
        echo "Equal"
    elif [ "$score" -gt 0 ] 2>/dev/null; then
        if [ "$abs_score" -lt 20 ]; then
            echo "White +0.00-0.20"
        elif [ "$abs_score" -lt 50 ]; then
            echo "White +0.20-0.50"
        elif [ "$abs_score" -lt 150 ]; then
            echo "White +0.50-1.50"
        elif [ "$abs_score" -lt 300 ]; then
            echo "White +1.50-3.00"
        elif [ "$abs_score" -lt 500 ]; then
            echo "White +3.00-5.00"
        else
            echo "White winning"
        fi
    else
        if [ "$abs_score" -lt 20 ]; then
            echo "Black -0.00-0.20"
        elif [ "$abs_score" -lt 50 ]; then
            echo "Black -0.20-0.50"
        elif [ "$abs_score" -lt 150 ]; then
            echo "Black -0.50-1.50"
        elif [ "$abs_score" -lt 300 ]; then
            echo "Black -1.50-3.00"
        elif [ "$abs_score" -lt 500 ]; then
            echo "Black -3.00-5.00"
        else
            echo "Black winning"
        fi
    fi
}

# Prepare search commands
if [ "$MODE" = "depth" ]; then
    SEARCH_CMD="go depth $VALUE"
    SEARCH_FILTER="info depth $VALUE"
else
    SEARCH_CMD="go movetime $((VALUE * 1000))"
    SEARCH_FILTER="info depth"
fi

echo -e "${BOLD}----------------------------------------"
echo -e "Analyzing position with all engines..."
echo -e "----------------------------------------${NC}"
echo

# Run SeaJay analysis (give more time for deep searches)
echo -e "${YELLOW}[1/4] Running SeaJay (negamax)...${NC}"
if [ "$MODE" = "depth" ]; then
    # For depth mode, wait longer to allow reaching target depth
    SEAJAY_OUTPUT=$(echo -e "position fen $FEN\n$SEARCH_CMD\nquit" | timeout 60 $SEAJAY 2>&1)
else
    SEAJAY_OUTPUT=$(echo -e "position fen $FEN\n$SEARCH_CMD\nquit" | $SEAJAY 2>&1)
fi
# Always get the last info line, regardless of depth reached
SEAJAY_INFO=$(echo "$SEAJAY_OUTPUT" | grep "^info depth" | tail -1)
SEAJAY_BESTMOVE=$(echo "$SEAJAY_OUTPUT" | grep "^bestmove" | awk '{print $2}')
SEAJAY_DEPTH=$(echo "$SEAJAY_INFO" | grep -oP "depth \K[0-9]+")
SEAJAY_SCORE=$(echo "$SEAJAY_INFO" | grep -oP "score cp \K[-+]?[0-9]+")
SEAJAY_NODES=$(echo "$SEAJAY_INFO" | grep -oP "nodes \K[0-9]+")
SEAJAY_NPS=$(echo "$SEAJAY_INFO" | grep -oP "nps \K[0-9]+")
SEAJAY_PV=$(echo "$SEAJAY_INFO" | grep -oP "pv \K[^ ]+" | head -1)

# Convert SeaJay score to UCI perspective
if [ -n "$SEAJAY_SCORE" ]; then
    if [ "$SIDE_TO_MOVE" = "w" ]; then
        SEAJAY_UCI_SCORE=$SEAJAY_SCORE
    else
        SEAJAY_UCI_SCORE=$((-SEAJAY_SCORE))
    fi
else
    SEAJAY_UCI_SCORE=""
fi

# Run Stockfish analysis
echo -e "${YELLOW}[2/4] Running Stockfish...${NC}"
if [ "$MODE" = "depth" ]; then
    # For depth searches, give more time
    SF_OUTPUT=$( (echo "uci"; sleep 0.1; echo "isready"; sleep 0.1; echo "position fen $FEN"; sleep 0.1; echo "$SEARCH_CMD"; sleep 10; echo "quit") | timeout 15 $STOCKFISH 2>&1)
else
    SF_OUTPUT=$( (echo "uci"; sleep 0.1; echo "isready"; sleep 0.1; echo "position fen $FEN"; sleep 0.1; echo "$SEARCH_CMD"; sleep 2; echo "quit") | timeout 5 $STOCKFISH 2>&1)
fi
# Get the last info line produced
SF_INFO=$(echo "$SF_OUTPUT" | grep "^info depth" | grep -v "currmove" | tail -1)
SF_BESTMOVE=$(echo "$SF_OUTPUT" | grep "^bestmove" | awk '{print $2}')
SF_DEPTH=$(echo "$SF_INFO" | grep -oP "depth \K[0-9]+")
SF_SCORE=$(echo "$SF_INFO" | grep -oP "score cp \K[-+]?[0-9]+")
SF_NODES=$(echo "$SF_INFO" | grep -oP "nodes \K[0-9]+")
SF_NPS=$(echo "$SF_INFO" | grep -oP "nps \K[0-9]+")
SF_PV=$(echo "$SF_INFO" | grep -oP "pv \K[^ ]+" | head -1)

# Run Stash analysis
echo -e "${YELLOW}[3/4] Running Stash...${NC}"
if [ "$MODE" = "depth" ]; then
    STASH_OUTPUT=$( (echo "uci"; sleep 0.1; echo "isready"; sleep 0.1; echo "position fen $FEN"; sleep 0.1; echo "$SEARCH_CMD"; sleep 10; echo "quit") | timeout 15 $STASH 2>&1)
else
    STASH_OUTPUT=$( (echo "uci"; sleep 0.1; echo "isready"; sleep 0.1; echo "position fen $FEN"; sleep 0.1; echo "$SEARCH_CMD"; sleep 2; echo "quit") | timeout 5 $STASH 2>&1)
fi
# Get the last info line produced
STASH_INFO=$(echo "$STASH_OUTPUT" | grep "^info depth" | tail -1)
STASH_BESTMOVE=$(echo "$STASH_OUTPUT" | grep "^bestmove" | awk '{print $2}')
STASH_DEPTH=$(echo "$STASH_INFO" | grep -oP "depth \K[0-9]+")
STASH_SCORE=$(echo "$STASH_INFO" | grep -oP "score cp \K[-+]?[0-9]+")
STASH_NODES=$(echo "$STASH_INFO" | grep -oP "nodes \K[0-9]+")
STASH_NPS=$(echo "$STASH_INFO" | grep -oP "nps \K[0-9]+")
STASH_PV=$(echo "$STASH_INFO" | grep -oP "pv \K[^ ]+" | head -1)

# Run Laser analysis
echo -e "${YELLOW}[4/4] Running Laser...${NC}"
if [ "$MODE" = "depth" ]; then
    LASER_OUTPUT=$( (echo "uci"; sleep 0.1; echo "isready"; sleep 0.1; echo "position fen $FEN"; sleep 0.1; echo "$SEARCH_CMD"; sleep 10; echo "quit") | timeout 15 $LASER 2>&1)
else
    LASER_OUTPUT=$( (echo "uci"; sleep 0.1; echo "isready"; sleep 0.1; echo "position fen $FEN"; sleep 0.1; echo "$SEARCH_CMD"; sleep 2; echo "quit") | timeout 5 $LASER 2>&1)
fi
# Get the last info line produced (excluding bound lines)
LASER_INFO=$(echo "$LASER_OUTPUT" | grep "^info depth" | grep -v "lowerbound\|upperbound" | tail -1)
LASER_BESTMOVE=$(echo "$LASER_OUTPUT" | grep "^bestmove" | awk '{print $2}')
LASER_DEPTH=$(echo "$LASER_INFO" | grep -oP "depth \K[0-9]+")
LASER_SCORE=$(echo "$LASER_INFO" | grep -oP "score cp \K[-+]?[0-9]+")
LASER_NODES=$(echo "$LASER_INFO" | grep -oP "nodes \K[0-9]+")
LASER_NPS=$(echo "$LASER_INFO" | grep -oP "nps \K[0-9]+")
LASER_PV=$(echo "$LASER_INFO" | grep -oP "pv \K[^ ]+" | head -1)

echo
echo -e "${BOLD}=========================================="
echo -e "Engine Results Summary"
echo -e "==========================================${NC}"
echo
echo -e "${BOLD}Scores (White's perspective):"
echo -e "----------------------------------------${NC}"
printf "%-12s %+7scp  %-18s %s\n" "SeaJay:" "${SEAJAY_UCI_SCORE:-N/A}" "$([ -n "$SEAJAY_UCI_SCORE" ] && interpret_score "$SEAJAY_UCI_SCORE" || echo "")" "[negamax→UCI]"
printf "%-12s %+7scp  %-18s\n" "Stockfish:" "${SF_SCORE:-N/A}" "$([ -n "$SF_SCORE" ] && interpret_score "$SF_SCORE" || echo "")"
printf "%-12s %+7scp  %-18s\n" "Stash:" "${STASH_SCORE:-N/A}" "$([ -n "$STASH_SCORE" ] && interpret_score "$STASH_SCORE" || echo "")"
printf "%-12s %+7scp  %-18s\n" "Laser:" "${LASER_SCORE:-N/A}" "$([ -n "$LASER_SCORE" ] && interpret_score "$LASER_SCORE" || echo "")"

echo
echo -e "${BOLD}Best Moves:"
echo -e "----------------------------------------${NC}"
printf "%-12s %-8s (depth %s)\n" "SeaJay:" "${SEAJAY_BESTMOVE:-N/A}" "${SEAJAY_DEPTH:-?}"
printf "%-12s %-8s (depth %s)\n" "Stockfish:" "${SF_BESTMOVE:-N/A}" "${SF_DEPTH:-?}"
printf "%-12s %-8s (depth %s)\n" "Stash:" "${STASH_BESTMOVE:-N/A}" "${STASH_DEPTH:-?}"
printf "%-12s %-8s (depth %s)\n" "Laser:" "${LASER_BESTMOVE:-N/A}" "${LASER_DEPTH:-?}"

echo
echo -e "${BOLD}Performance:"
echo -e "----------------------------------------${NC}"
printf "%-12s %10s nodes @ %10s nps\n" "SeaJay:" "${SEAJAY_NODES:-N/A}" "${SEAJAY_NPS:-N/A}"
printf "%-12s %10s nodes @ %10s nps\n" "Stockfish:" "${SF_NODES:-N/A}" "${SF_NPS:-N/A}"
printf "%-12s %10s nodes @ %10s nps\n" "Stash:" "${STASH_NODES:-N/A}" "${STASH_NPS:-N/A}"
printf "%-12s %10s nodes @ %10s nps\n" "Laser:" "${LASER_NODES:-N/A}" "${LASER_NPS:-N/A}"

# Score agreement analysis
echo
echo -e "${BOLD}=========================================="
echo -e "Analysis & Agreement"
echo -e "==========================================${NC}"

# Check if UCI engines agree on score sign
if [ -n "$SF_SCORE" ] && [ -n "$STASH_SCORE" ] && [ -n "$LASER_SCORE" ]; then
    SF_SIGN=$([ "$SF_SCORE" -ge 0 ] && echo "+" || echo "-")
    STASH_SIGN=$([ "$STASH_SCORE" -ge 0 ] && echo "+" || echo "-")
    LASER_SIGN=$([ "$LASER_SCORE" -ge 0 ] && echo "+" || echo "-")
    
    echo -e "${BOLD}Score perspective check:${NC}"
    if [ "$SF_SIGN" = "$STASH_SIGN" ] && [ "$STASH_SIGN" = "$LASER_SIGN" ]; then
        echo -e "  ${GREEN}✓ All UCI engines agree on who's better${NC}"
        
        # Check SeaJay alignment
        if [ -n "$SEAJAY_UCI_SCORE" ]; then
            SEAJAY_SIGN=$([ "$SEAJAY_UCI_SCORE" -ge 0 ] && echo "+" || echo "-")
            if [ "$SEAJAY_SIGN" = "$SF_SIGN" ]; then
                echo -e "  ${GREEN}✓ SeaJay (converted) agrees with UCI engines${NC}"
            else
                echo -e "  ${RED}✗ SeaJay (converted) disagrees on who's better!${NC}"
            fi
        fi
    else
        echo -e "  ${RED}⚠ UCI engines disagree on who's better!${NC}"
    fi
    
    # Calculate score variance
    if [ -n "$SEAJAY_UCI_SCORE" ]; then
        # Find min and max scores
        MIN_SCORE=$SF_SCORE
        MAX_SCORE=$SF_SCORE
        
        for score in $SEAJAY_UCI_SCORE $STASH_SCORE $LASER_SCORE; do
            [ "$score" -lt "$MIN_SCORE" ] && MIN_SCORE=$score
            [ "$score" -gt "$MAX_SCORE" ] && MAX_SCORE=$score
        done
        
        VARIANCE=$((MAX_SCORE - MIN_SCORE))
        echo
        echo -e "${BOLD}Score variance:${NC}"
        echo -e "  Range: ${MIN_SCORE}cp to ${MAX_SCORE}cp (${VARIANCE}cp spread)"
        
        if [ "$VARIANCE" -lt 20 ]; then
            echo -e "  ${GREEN}✓ Excellent agreement${NC}"
        elif [ "$VARIANCE" -lt 50 ]; then
            echo -e "  ${GREEN}✓ Good agreement${NC}"
        elif [ "$VARIANCE" -lt 100 ]; then
            echo -e "  ${YELLOW}⚠ Some disagreement${NC}"
        else
            echo -e "  ${RED}✗ Major disagreement - investigate${NC}"
        fi
    fi
fi

# Move agreement analysis
echo
echo -e "${BOLD}Move consensus:${NC}"
# Count how many engines choose each move
declare -A move_count
declare -A move_engines

for engine_move in "$SEAJAY_BESTMOVE:SeaJay" "$SF_BESTMOVE:Stockfish" "$STASH_BESTMOVE:Stash" "$LASER_BESTMOVE:Laser"; do
    move="${engine_move%%:*}"
    engine="${engine_move#*:}"
    if [ -n "$move" ] && [ "$move" != "N/A" ]; then
        ((move_count[$move]++))
        if [ -z "${move_engines[$move]}" ]; then
            move_engines[$move]="$engine"
        else
            move_engines[$move]="${move_engines[$move]}, $engine"
        fi
    fi
done

# Display move consensus
for move in "${!move_count[@]}"; do
    count="${move_count[$move]}"
    engines="${move_engines[$move]}"
    if [ "$count" -eq 4 ]; then
        echo -e "  ${GREEN}✓ Unanimous: $move (all engines)${NC}"
    elif [ "$count" -eq 3 ]; then
        echo -e "  ${GREEN}✓ Strong consensus: $move ($engines)${NC}"
    elif [ "$count" -eq 2 ]; then
        echo -e "  ${YELLOW}Split: $move ($engines)${NC}"
    else
        echo -e "  $move ($engines)"
    fi
done

# Static evaluation comparison
echo
echo -e "${BOLD}----------------------------------------"
echo -e "Static Evaluation Comparison"
echo -e "----------------------------------------${NC}"

# Get SeaJay eval (from Total line, side-to-move perspective)
SEAJAY_EVAL=$(echo -e "position fen $FEN\neval\nquit" | $SEAJAY 2>&1 | grep "^Total:" | grep -oP "[-+]?[0-9]+" | tail -1)

# Get Stockfish eval
SF_EVAL=$(echo -e "position fen $FEN\neval\nquit" | timeout 1 $STOCKFISH 2>&1 | grep "Final evaluation" | grep -oP "[-+]?[0-9]+(?:\.[0-9]+)?")

# Get Laser eval (from Static evaluation line)
LASER_EVAL=$(echo -e "uci\nisready\nposition fen $FEN\neval\nquit" | timeout 1 $LASER 2>&1 | grep "Static evaluation:" | grep -oP "[-+]?[0-9]+" | tail -1)

# Note: Stash doesn't appear to have an eval command

# Convert SF eval from pawns to centipawns if needed
if [ -n "$SF_EVAL" ]; then
    if [[ "$SF_EVAL" == *"."* ]]; then
        # Handle sign
        if [[ "$SF_EVAL" == "-"* ]]; then
            SIGN="-"
            SF_EVAL="${SF_EVAL#-}"
        else
            SIGN=""
        fi
        # Extract parts
        INTEGER_PART=${SF_EVAL%%.*}
        DECIMAL_PART=${SF_EVAL#*.}
        # Pad decimal
        DECIMAL_PART="${DECIMAL_PART}00"
        DECIMAL_PART="${DECIMAL_PART:0:2}"
        # Remove leading zeros
        INTEGER_PART=${INTEGER_PART#0}
        INTEGER_PART=${INTEGER_PART:-0}
        DECIMAL_PART=${DECIMAL_PART#0}
        DECIMAL_PART=${DECIMAL_PART:-0}
        SF_EVAL_CP=$((INTEGER_PART * 100 + DECIMAL_PART))
        if [ "$SIGN" = "-" ]; then
            SF_EVAL_CP=$((-SF_EVAL_CP))
        fi
    else
        SF_EVAL_CP=$SF_EVAL
    fi
fi

echo -e "${BOLD}Static evaluations (in centipawns):${NC}"
echo -e "----------------------------------------"

if [ -n "$SEAJAY_EVAL" ]; then
    # Convert to White's perspective
    if [ "$SIDE_TO_MOVE" = "w" ]; then
        SEAJAY_EVAL_WHITE=$SEAJAY_EVAL
    else
        SEAJAY_EVAL_WHITE=$((-SEAJAY_EVAL))
    fi
    printf "%-12s %+7dcp (side-to-move: %+dcp)\n" "SeaJay:" "$SEAJAY_EVAL_WHITE" "$SEAJAY_EVAL"
fi

if [ -n "$SF_EVAL_CP" ]; then
    printf "%-12s %+7dcp (White's view)\n" "Stockfish:" "$SF_EVAL_CP"
fi

if [ -n "$LASER_EVAL" ]; then
    # Laser eval appears to be in centipawns from White's perspective already
    printf "%-12s %+7dcp (White's view)\n" "Laser:" "$LASER_EVAL"
fi

echo -e "%-12s (no eval command)" "Stash:"

# Calculate static eval variance if we have multiple values
if [ -n "$SEAJAY_EVAL_WHITE" ] && [ -n "$SF_EVAL_CP" -o -n "$LASER_EVAL" ]; then
    echo
    echo -e "${BOLD}Static eval analysis:${NC}"
    
    # Find min/max among available evals
    MIN_EVAL=$SEAJAY_EVAL_WHITE
    MAX_EVAL=$SEAJAY_EVAL_WHITE
    
    if [ -n "$SF_EVAL_CP" ]; then
        [ "$SF_EVAL_CP" -lt "$MIN_EVAL" ] && MIN_EVAL=$SF_EVAL_CP
        [ "$SF_EVAL_CP" -gt "$MAX_EVAL" ] && MAX_EVAL=$SF_EVAL_CP
    fi
    
    if [ -n "$LASER_EVAL" ]; then
        [ "$LASER_EVAL" -lt "$MIN_EVAL" ] && MIN_EVAL=$LASER_EVAL
        [ "$LASER_EVAL" -gt "$MAX_EVAL" ] && MAX_EVAL=$LASER_EVAL
    fi
    
    EVAL_VARIANCE=$((MAX_EVAL - MIN_EVAL))
    echo -e "  Range: ${MIN_EVAL}cp to ${MAX_EVAL}cp (${EVAL_VARIANCE}cp spread)"
    
    if [ "$EVAL_VARIANCE" -gt 100 ]; then
        echo -e "  ${RED}⚠ Large static eval difference - check evaluation function${NC}"
    elif [ "$EVAL_VARIANCE" -gt 50 ]; then
        echo -e "  ${YELLOW}⚠ Some static eval disagreement${NC}"
    else
        echo -e "  ${GREEN}✓ Good static eval agreement${NC}"
    fi
fi

echo
echo -e "${BOLD}==========================================${NC}"
echo -e "${BOLD}Key Insights:${NC}"
echo -e "• SeaJay uses negamax (converted to UCI above)"
echo -e "• Compare all four engines to identify:"
echo -e "  - Evaluation function issues"
echo -e "  - Search depth problems"
echo -e "  - Move ordering deficiencies"
echo -e "• Large disagreements suggest bugs to investigate"
echo -e "==========================================${NC}"