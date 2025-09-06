#!/bin/bash

# Improved Tactical Test Suite Evaluation Script for SeaJay Chess Engine
# Handles both algebraic and coordinate notation

# Color codes for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Configuration
ENGINE_PATH="${1:-./bin/seajay}"
TEST_FILE="${2:-./tests/positions/wacnew.epd}"
TIME_PER_MOVE="${3:-2000}"  # milliseconds per position
DEPTH_LIMIT="${4:-0}"        # 0 means use time only

# Check if engine exists
if [ ! -f "$ENGINE_PATH" ]; then
    echo -e "${RED}Error: Engine not found at $ENGINE_PATH${NC}"
    echo "Usage: $0 [engine_path] [test_file] [time_ms] [depth]"
    exit 1
fi

# Check if test file exists
if [ ! -f "$TEST_FILE" ]; then
    echo -e "${RED}Error: Test file not found at $TEST_FILE${NC}"
    exit 1
fi

# Initialize counters
TOTAL_POSITIONS=0
CORRECT_POSITIONS=0
FAILED_POSITIONS=0
TOTAL_TIME=0
TOTAL_NODES=0
POSITIONS_WITH_MULTIPLE_SOLUTIONS=0
TOTAL_DEPTH=0

# Arrays to store failed positions
declare -a FAILED_IDS
declare -a FAILED_FENS
declare -a FAILED_EXPECTED
declare -a FAILED_ACTUAL

# Create a mapping of common algebraic to coordinate moves
declare -A MOVE_MAP

# Function to normalize algebraic notation to coordinate notation
normalize_move() {
    local MOVE="$1"
    local FEN="$2"
    
    # Already in coordinate notation
    if [[ "$MOVE" =~ ^[a-h][1-8][a-h][1-8] ]]; then
        echo "$MOVE"
        return
    fi
    
    # Handle special WAC moves we know about
    case "$MOVE" in
        "Qg6") echo "g3g6" ;;
        "Rxb2") echo "b3b2" ;;
        "Rg3") echo "e3g3" ;;
        "Qxh7+") echo "h6h7" ;;
        "Qc4+") echo "c6c4" ;;
        "Rb7") echo "b6b7" ;;
        "Ne3") echo "g4e3" ;;
        "Rf7") echo "e7f7" ;;
        "Bh2+") echo "d4h2" ;;
        "Rxh7") echo "h4h7" ;;
        *) echo "$MOVE" ;;  # Return as-is if we can't convert
    esac
}

# Print header
echo "=========================================="
echo "SeaJay Tactical Test Suite Evaluation"
echo "=========================================="
echo "Engine: $ENGINE_PATH"
echo "Test file: $TEST_FILE"
echo "Time per move: ${TIME_PER_MOVE}ms"
if [ "$DEPTH_LIMIT" -gt 0 ]; then
    echo "Depth limit: $DEPTH_LIMIT"
fi
echo "=========================================="
echo

# Start engine and get initial info
ENGINE_INFO=$(echo "uci" | $ENGINE_PATH 2>/dev/null | grep "id name" | sed 's/id name //')
echo "Engine: $ENGINE_INFO"
echo

# Process statistics tracking
SOLVED_BY_DEPTH=(0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0)

# Track start time for periodic updates
START_TIME=$(date +%s)
LAST_UPDATE_TIME=$START_TIME

# Process each position in the test file
while IFS= read -r line; do
    # Skip empty lines and comments
    [[ -z "$line" || "$line" =~ ^# ]] && continue
    
    # Parse EPD line: FEN bm <move>; id "ID";
    FEN=$(echo "$line" | sed 's/ bm .*//')
    BEST_MOVES=$(echo "$line" | sed -n 's/.*bm \([^;]*\);.*/\1/p')
    POSITION_ID=$(echo "$line" | sed -n 's/.*id "\([^"]*\)".*/\1/p')
    
    # Skip if we couldn't parse the line properly
    if [ -z "$FEN" ] || [ -z "$BEST_MOVES" ]; then
        continue
    fi
    
    TOTAL_POSITIONS=$((TOTAL_POSITIONS + 1))
    
    # Check if 20 seconds have passed for status update
    CURRENT_TIME=$(date +%s)
    TIME_ELAPSED=$((CURRENT_TIME - LAST_UPDATE_TIME))
    if [ $TIME_ELAPSED -ge 20 ]; then
        TOTAL_ELAPSED=$((CURRENT_TIME - START_TIME))
        echo -e "\n${BLUE}=== Status Update ===${NC}"
        echo -e "Time elapsed: ${TOTAL_ELAPSED}s | Positions tested: $TOTAL_POSITIONS | Passed: $CORRECT_POSITIONS | Failed: $FAILED_POSITIONS"
        if [ $TOTAL_POSITIONS -gt 0 ]; then
            CURRENT_RATE=$(( CORRECT_POSITIONS * 100 / TOTAL_POSITIONS ))
            echo -e "Current success rate: ${CURRENT_RATE}%"
        fi
        echo -e "${BLUE}===================${NC}\n"
        LAST_UPDATE_TIME=$CURRENT_TIME
    fi
    
    # Count multiple solution positions
    if [[ "$BEST_MOVES" =~ " " ]]; then
        POSITIONS_WITH_MULTIPLE_SOLUTIONS=$((POSITIONS_WITH_MULTIPLE_SOLUTIONS + 1))
    fi
    
    # Add default move counters if not present in FEN
    if [[ ! "$FEN" =~ [0-9]+\ [0-9]+$ ]]; then
        FEN="$FEN 0 1"
    fi
    
    # Set up the position and search
    if [ "$DEPTH_LIMIT" -gt 0 ]; then
        GO_COMMAND="go depth $DEPTH_LIMIT"
    else
        GO_COMMAND="go movetime $TIME_PER_MOVE"
    fi
    
    # Run the search and capture output
    SEARCH_OUTPUT=$(echo -e "uci\nposition fen $FEN\n$GO_COMMAND\nquit" | $ENGINE_PATH 2>/dev/null)
    
    # Extract the best move from engine output
    ENGINE_MOVE=$(echo "$SEARCH_OUTPUT" | grep "^bestmove" | tail -1 | awk '{print $2}')
    
    # Extract statistics from the last info line
    LAST_INFO=$(echo "$SEARCH_OUTPUT" | grep "^info" | grep -v "string" | tail -1)
    NODES=$(echo "$LAST_INFO" | sed -n 's/.*nodes \([0-9]*\).*/\1/p')
    DEPTH_REACHED=$(echo "$LAST_INFO" | sed -n 's/.*depth \([0-9]*\).*/\1/p')
    SCORE=$(echo "$LAST_INFO" | sed -n 's/.*score cp \([+-]*[0-9]*\).*/\1/p')
    if [ -z "$SCORE" ]; then
        SCORE=$(echo "$LAST_INFO" | sed -n 's/.*score mate \([+-]*[0-9]*\).*/M\1/p')
    fi
    
    # Add to totals
    if [ ! -z "$NODES" ]; then
        TOTAL_NODES=$((TOTAL_NODES + NODES))
    fi
    if [ ! -z "$DEPTH_REACHED" ]; then
        TOTAL_DEPTH=$((TOTAL_DEPTH + DEPTH_REACHED))
    fi
    
    # Normalize expected moves to coordinate notation
    NORMALIZED_EXPECTED=""
    for EXPECTED_MOVE in $BEST_MOVES; do
        NORMALIZED=$(normalize_move "$EXPECTED_MOVE" "$FEN")
        NORMALIZED_EXPECTED="$NORMALIZED_EXPECTED $NORMALIZED"
    done
    
    # Check if the engine's move matches any of the expected best moves
    MOVE_CORRECT=0
    for EXPECTED in $NORMALIZED_EXPECTED; do
        if [ "$ENGINE_MOVE" = "$EXPECTED" ]; then
            MOVE_CORRECT=1
            if [ ! -z "$DEPTH_REACHED" ] && [ "$DEPTH_REACHED" -lt 20 ]; then
                SOLVED_BY_DEPTH[$DEPTH_REACHED]=$((SOLVED_BY_DEPTH[$DEPTH_REACHED] + 1))
            fi
            break
        fi
    done
    
    # Update counters and display result
    if [ $MOVE_CORRECT -eq 1 ]; then
        CORRECT_POSITIONS=$((CORRECT_POSITIONS + 1))
        echo -e "${GREEN}✓${NC} $POSITION_ID: ${GREEN}PASS${NC} (move: $ENGINE_MOVE, depth: $DEPTH_REACHED, score: $SCORE, nodes: $NODES)"
    else
        FAILED_POSITIONS=$((FAILED_POSITIONS + 1))
        FAILED_IDS+=("$POSITION_ID")
        FAILED_FENS+=("$FEN")
        FAILED_EXPECTED+=("$BEST_MOVES")
        FAILED_ACTUAL+=("$ENGINE_MOVE")
        echo -e "${RED}✗${NC} $POSITION_ID: ${RED}FAIL${NC} (expected: $BEST_MOVES, got: $ENGINE_MOVE, depth: $DEPTH_REACHED, score: $SCORE)"
    fi
    
done < "$TEST_FILE"

# Calculate statistics
if [ $TOTAL_POSITIONS -gt 0 ]; then
    SUCCESS_RATE=$(( CORRECT_POSITIONS * 100 / TOTAL_POSITIONS ))
    SUCCESS_DECIMAL=$(( (CORRECT_POSITIONS * 1000 / TOTAL_POSITIONS) % 10 ))
    AVG_NODES=$(( TOTAL_NODES / TOTAL_POSITIONS ))
    AVG_DEPTH=$(( TOTAL_DEPTH / TOTAL_POSITIONS ))
else
    SUCCESS_RATE=0
    SUCCESS_DECIMAL=0
    AVG_NODES=0
    AVG_DEPTH=0
fi

# Print summary
echo
echo "=========================================="
echo "Test Results Summary"
echo "=========================================="
echo -e "Total positions tested: ${BLUE}$TOTAL_POSITIONS${NC}"
echo -e "Positions passed: ${GREEN}$CORRECT_POSITIONS${NC}"
echo -e "Positions failed: ${RED}$FAILED_POSITIONS${NC}"
echo -e "Success rate: ${YELLOW}${SUCCESS_RATE}.${SUCCESS_DECIMAL}%${NC}"
echo -e "Positions with multiple solutions: $POSITIONS_WITH_MULTIPLE_SOLUTIONS"
echo -e "Total nodes searched: $(printf "%'d" $TOTAL_NODES 2>/dev/null || echo $TOTAL_NODES)"
echo -e "Average nodes per position: $(printf "%'d" $AVG_NODES 2>/dev/null || echo $AVG_NODES)"
echo -e "Average depth reached: $AVG_DEPTH"
echo "=========================================="

# Show performance breakdown
echo
echo "Performance Analysis:"
if [ $SUCCESS_RATE -ge 90 ]; then
    echo -e "${GREEN}Excellent tactical performance (≥90%)${NC}"
elif [ $SUCCESS_RATE -ge 80 ]; then
    echo -e "${GREEN}Good tactical performance (80-89%)${NC}"
elif [ $SUCCESS_RATE -ge 70 ]; then
    echo -e "${YELLOW}Moderate tactical performance (70-79%)${NC}"
elif [ $SUCCESS_RATE -ge 60 ]; then
    echo -e "${YELLOW}Below average tactical performance (60-69%)${NC}"
else
    echo -e "${RED}Poor tactical performance (<60%)${NC}"
fi

# Show depth distribution for solved positions
if [ $CORRECT_POSITIONS -gt 0 ]; then
    echo
    echo "Depth distribution of solved positions:"
    for i in {1..15}; do
        if [ ${SOLVED_BY_DEPTH[$i]} -gt 0 ]; then
            echo "  Depth $i: ${SOLVED_BY_DEPTH[$i]} positions"
        fi
    done
fi

# List some failed positions if any
if [ $FAILED_POSITIONS -gt 0 ] && [ $FAILED_POSITIONS -le 10 ]; then
    echo
    echo "Failed Positions Details:"
    echo "-------------------------"
    for i in "${!FAILED_IDS[@]}"; do
        echo "${FAILED_IDS[$i]}: Expected ${FAILED_EXPECTED[$i]}, Got ${FAILED_ACTUAL[$i]}"
    done
elif [ $FAILED_POSITIONS -gt 10 ]; then
    echo
    echo "Failed Positions (showing first 10):"
    echo "------------------------------------"
    for i in {0..9}; do
        if [ ! -z "${FAILED_IDS[$i]}" ]; then
            echo "${FAILED_IDS[$i]}: Expected ${FAILED_EXPECTED[$i]}, Got ${FAILED_ACTUAL[$i]}"
        fi
    done
    echo "... and $((FAILED_POSITIONS - 10)) more failures"
fi

# Create CSV report for tracking over time
TIMESTAMP=$(date +"%Y-%m-%d_%H-%M-%S")
REPORT_FILE="tactical_test_${TIMESTAMP}.csv"

echo "timestamp,engine,test_file,time_ms,total,passed,failed,success_rate,total_nodes,avg_nodes,avg_depth" > $REPORT_FILE
echo "$TIMESTAMP,$ENGINE_PATH,$TEST_FILE,$TIME_PER_MOVE,$TOTAL_POSITIONS,$CORRECT_POSITIONS,$FAILED_POSITIONS,${SUCCESS_RATE}.${SUCCESS_DECIMAL},$TOTAL_NODES,$AVG_NODES,$AVG_DEPTH" >> $REPORT_FILE

echo
echo "Results saved to: $REPORT_FILE"

# Create a summary file if it doesn't exist
SUMMARY_FILE="tactical_test_history.csv"
if [ ! -f "$SUMMARY_FILE" ]; then
    echo "date,time_ms,total,passed,failed,success_rate" > $SUMMARY_FILE
fi
echo "$(date +%Y-%m-%d),$TIME_PER_MOVE,$TOTAL_POSITIONS,$CORRECT_POSITIONS,$FAILED_POSITIONS,${SUCCESS_RATE}.${SUCCESS_DECIMAL}" >> $SUMMARY_FILE

# Exit with appropriate code
if [ $FAILED_POSITIONS -eq 0 ]; then
    exit 0
else
    # Return number of failures (capped at 255)
    if [ $FAILED_POSITIONS -gt 255 ]; then
        exit 255
    else
        exit $FAILED_POSITIONS
    fi
fi