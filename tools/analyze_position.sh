#!/bin/bash

# analyze_position.sh - Four-engine position analysis utility for SeaJay debugging
# Usage: ./analyze_position.sh "FEN" depth|time [value] [--save-report]
# Example: ./analyze_position.sh "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1" depth 10
# Example: ./analyze_position.sh "r1b1k2r/pp3ppp/3Bp3/3p4/6q1/8/1PP2PPP/22R1R1K1 b kq - 5 17" time 5 --save-report

SEAJAY="/workspace/bin/seajay"
STASH="/workspace/external/engines/stash-bot/stash"
KOMODO="/workspace/external/engines/komodo/komodo-14.1-linux"  # Gold standard
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
    echo "Usage: $0 \"FEN\" depth|time [value] [--save-report]"
    echo "  depth mode: $0 \"FEN\" depth 10"
    echo "  time mode:  $0 \"FEN\" time 5"
    echo "  Save report: $0 \"FEN\" depth 10 --save-report"
    exit 1
fi

FEN="$1"
MODE="$2"
VALUE="${3:-10}"  # Default depth 10 or time 10 seconds
SAVE_REPORT=false

# Check for --save-report flag
for arg in "$@"; do
    if [ "$arg" = "--save-report" ]; then
        SAVE_REPORT=true
    fi
done

# Validate mode
if [ "$MODE" != "depth" ] && [ "$MODE" != "time" ]; then
    echo "Error: Mode must be 'depth' or 'time'"
    exit 1
fi

# Create temp directory for this analysis session
TIMESTAMP=$(date +%Y%m%d_%H%M%S)
TEMP_DIR="/tmp/chess_analysis_${TIMESTAMP}_$$"
mkdir -p "$TEMP_DIR"

# Create report directory if saving
if [ "$SAVE_REPORT" = true ]; then
    REPORT_DIR="/workspace/analysis_reports"
    mkdir -p "$REPORT_DIR"
    REPORT_FILE="$REPORT_DIR/analysis_${TIMESTAMP}.txt"
fi

# Cleanup function
cleanup() {
    if [ "$SAVE_REPORT" = false ]; then
        rm -rf "$TEMP_DIR"
    fi
}
trap cleanup EXIT

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
echo -e "${BOLD}Temp directory:${NC} $TEMP_DIR"
if [ "$SAVE_REPORT" = true ]; then
    echo -e "${BOLD}Report will be saved to:${NC} $REPORT_FILE"
fi
echo

# Function to format score as pawns (divide by 100)
format_as_pawns() {
    local score=$1
    if [ -z "$score" ] || [ "$score" = "N/A" ]; then
        echo "N/A"
    else
        # Use awk for floating point division
        echo "$score" | awk '{printf "%+.2f", $1/100}'
    fi
}

# Function to interpret score from side-to-move perspective (UCI standard)
interpret_score() {
    local score=$1
    local abs_score=${score#-}  # Remove negative sign if present

    if [ "$score" -eq 0 ] 2>/dev/null; then
        echo "Equal"
    elif [ "$score" -gt 0 ] 2>/dev/null; then
        # Positive score = side to move is better
        if [ "$SIDE_TO_MOVE" = "w" ]; then
            # White to move, positive = White better
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
            # Black to move, positive = Black better
            if [ "$abs_score" -lt 20 ]; then
                echo "Black +0.00-0.20"
            elif [ "$abs_score" -lt 50 ]; then
                echo "Black +0.20-0.50"
            elif [ "$abs_score" -lt 150 ]; then
                echo "Black +0.50-1.50"
            elif [ "$abs_score" -lt 300 ]; then
                echo "Black +1.50-3.00"
            elif [ "$abs_score" -lt 500 ]; then
                echo "Black +3.00-5.00"
            else
                echo "Black winning"
            fi
        fi
    else
        # Negative score = side to move is worse
        if [ "$SIDE_TO_MOVE" = "w" ]; then
            # White to move, negative = Black better
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
        else
            # Black to move, negative = White better
            if [ "$abs_score" -lt 20 ]; then
                echo "White -0.00-0.20"
            elif [ "$abs_score" -lt 50 ]; then
                echo "White -0.20-0.50"
            elif [ "$abs_score" -lt 150 ]; then
                echo "White -0.50-1.50"
            elif [ "$abs_score" -lt 300 ]; then
                echo "White -1.50-3.00"
            elif [ "$abs_score" -lt 500 ]; then
                echo "White -3.00-5.00"
            else
                echo "White winning"
            fi
        fi
    fi
}

# Prepare search commands
if [ "$MODE" = "depth" ]; then
    SEARCH_CMD="go depth $VALUE"
    SEARCH_FILTER="info depth $VALUE"
    # Give more time for deeper searches
    if [ "$VALUE" -gt 15 ]; then
        TIMEOUT=60
    elif [ "$VALUE" -gt 10 ]; then
        TIMEOUT=30
    else
        TIMEOUT=15
    fi
else
    SEARCH_CMD="go movetime $((VALUE * 1000))"
    SEARCH_FILTER="info depth"
    TIMEOUT=$((VALUE + 5))
fi

echo -e "${BOLD}----------------------------------------"
echo -e "Analyzing position with all engines..."
echo -e "----------------------------------------${NC}"
echo

# Function to run engine and save output
run_engine() {
    local engine_name=$1
    local engine_path=$2
    local output_file=$3
    local uci_mode=${4:-true}  # Most engines use UCI

    if [ "$uci_mode" = true ]; then
        (
            echo "uci"
            sleep 0.1
            echo "isready"
            sleep 0.2
            echo "position fen $FEN"
            sleep 0.1
            echo "$SEARCH_CMD"
            if [ "$MODE" = "depth" ]; then
                sleep $((TIMEOUT - 5))
            else
                sleep $((VALUE + 1))
            fi
            echo "quit"
        ) | timeout $TIMEOUT "$engine_path" > "$output_file" 2>&1
    else
        # For engines that don't use standard UCI (if any)
        echo -e "position fen $FEN\n$SEARCH_CMD\nquit" | timeout $TIMEOUT "$engine_path" > "$output_file" 2>&1
    fi
}

# Run all engines in parallel
echo -e "${YELLOW}[1/4] Starting SeaJay...${NC}"
run_engine "SeaJay" "$SEAJAY" "$TEMP_DIR/seajay.out" false &
PID_SEAJAY=$!

echo -e "${YELLOW}[2/4] Starting Stash...${NC}"
run_engine "Stash" "$STASH" "$TEMP_DIR/stash.out" true &
PID_STASH=$!

echo -e "${YELLOW}[3/4] Starting Komodo...${NC}"
run_engine "Komodo" "$KOMODO" "$TEMP_DIR/komodo.out" true &
PID_KOMODO=$!

echo -e "${YELLOW}[4/4] Starting Laser...${NC}"
run_engine "Laser" "$LASER" "$TEMP_DIR/laser.out" true &
PID_LASER=$!

# Wait for all engines to complete
echo -e "${YELLOW}Waiting for all engines to complete...${NC}"
wait $PID_SEAJAY
wait $PID_STASH
wait $PID_KOMODO
wait $PID_LASER

echo -e "${GREEN}All engines completed!${NC}"
echo

# Function to parse engine output
parse_engine_output() {
    local engine_name=$1
    local output_file=$2
    local prefix=$3

    # Read the output file
    if [ ! -f "$output_file" ]; then
        echo "# ERROR: No output file for $engine_name"
        return
    fi

    local output=$(cat "$output_file")

    # Get the last info line for the target depth (or last info line in time mode)
    if [ "$MODE" = "depth" ]; then
        # For depth mode, try to get the exact depth requested
        local info_line=$(echo "$output" | grep "^info depth $VALUE " | tail -1)
        # If exact depth not found, get the last depth achieved
        if [ -z "$info_line" ]; then
            info_line=$(echo "$output" | grep "^info depth" | grep -v "currmove" | tail -1)
        fi
    else
        # For time mode, get the last info line
        local info_line=$(echo "$output" | grep "^info depth" | grep -v "currmove" | tail -1)
    fi

    # Get bestmove
    local bestmove=$(echo "$output" | grep "^bestmove" | tail -1 | awk '{print $2}')

    # Parse values from info line
    if [ -n "$info_line" ]; then
        # Use more precise parsing to avoid seldepth confusion
        local depth=$(echo "$info_line" | sed -n 's/.*\bdepth \([0-9]\+\).*/\1/p' | head -1)
        local score=$(echo "$info_line" | sed -n 's/.*\bscore cp \([-+]\?[0-9]\+\).*/\1/p')
        local nodes=$(echo "$info_line" | sed -n 's/.*\bnodes \([0-9]\+\).*/\1/p')
        local nps=$(echo "$info_line" | sed -n 's/.*\bnps \([0-9]\+\).*/\1/p')
        local pv=$(echo "$info_line" | sed -n 's/.*\bpv \([^ ]\+\).*/\1/p')

        # Store in global variables with prefix
        eval "${prefix}_DEPTH=\"$depth\""
        eval "${prefix}_SCORE=\"$score\""
        eval "${prefix}_NODES=\"$nodes\""
        eval "${prefix}_NPS=\"$nps\""
        eval "${prefix}_BESTMOVE=\"$bestmove\""
        eval "${prefix}_PV=\"$pv\""
        eval "${prefix}_INFO_LINE=\"$info_line\""
    else
        eval "${prefix}_DEPTH=\"\""
        eval "${prefix}_SCORE=\"\""
        eval "${prefix}_NODES=\"\""
        eval "${prefix}_NPS=\"\""
        eval "${prefix}_BESTMOVE=\"$bestmove\""
        eval "${prefix}_PV=\"\""
        eval "${prefix}_INFO_LINE=\"\""
    fi
}

# Parse all engine outputs
echo -e "${BOLD}----------------------------------------"
echo -e "Parsing engine outputs..."
echo -e "----------------------------------------${NC}"

parse_engine_output "SeaJay" "$TEMP_DIR/seajay.out" "SEAJAY"
parse_engine_output "Stash" "$TEMP_DIR/stash.out" "STASH"
parse_engine_output "Komodo" "$TEMP_DIR/komodo.out" "KOMODO"
parse_engine_output "Laser" "$TEMP_DIR/laser.out" "LASER"

# Keep all scores as-is from engines (no conversion)
# Scores are reported directly from each engine's perspective
SEAJAY_UCI_SCORE=$SEAJAY_SCORE
STASH_UCI_SCORE=$STASH_SCORE
KOMODO_UCI_SCORE=$KOMODO_SCORE
LASER_UCI_SCORE=$LASER_SCORE

echo
echo -e "${BOLD}=========================================="
echo -e "Engine Results Summary"
echo -e "==========================================${NC}"
echo
echo -e "${BOLD}Scores (as reported by engines):"
echo -e "----------------------------------------${NC}"
printf "%-12s %8s  %-18s\n" "SeaJay:" "$(format_as_pawns "$SEAJAY_UCI_SCORE")" "$([ -n "$SEAJAY_UCI_SCORE" ] && interpret_score "$SEAJAY_UCI_SCORE" || echo "")"
printf "%-12s %8s  %-18s\n" "Stash:" "$(format_as_pawns "$STASH_UCI_SCORE")" "$([ -n "$STASH_UCI_SCORE" ] && interpret_score "$STASH_UCI_SCORE" || echo "")"
printf "%-12s %8s  %-18s\n" "Komodo:" "$(format_as_pawns "$KOMODO_UCI_SCORE")" "$([ -n "$KOMODO_UCI_SCORE" ] && interpret_score "$KOMODO_UCI_SCORE" || echo "")"
printf "%-12s %8s  %-18s\n" "Laser:" "$(format_as_pawns "$LASER_UCI_SCORE")" "$([ -n "$LASER_UCI_SCORE" ] && interpret_score "$LASER_UCI_SCORE" || echo "")"

echo
echo -e "${BOLD}Best Moves:"
echo -e "----------------------------------------${NC}"
printf "%-12s %-8s (depth %s)\n" "SeaJay:" "${SEAJAY_BESTMOVE:-N/A}" "${SEAJAY_DEPTH:-?}"
printf "%-12s %-8s (depth %s)\n" "Stash:" "${STASH_BESTMOVE:-N/A}" "${STASH_DEPTH:-?}"
printf "%-12s %-8s (depth %s)\n" "Komodo:" "${KOMODO_BESTMOVE:-N/A}" "${KOMODO_DEPTH:-?}"
printf "%-12s %-8s (depth %s)\n" "Laser:" "${LASER_BESTMOVE:-N/A}" "${LASER_DEPTH:-?}"

echo
echo -e "${BOLD}Performance:"
echo -e "----------------------------------------${NC}"
printf "%-12s %10s nodes @ %10s nps\n" "SeaJay:" "${SEAJAY_NODES:-N/A}" "${SEAJAY_NPS:-N/A}"
printf "%-12s %10s nodes @ %10s nps\n" "Stash:" "${STASH_NODES:-N/A}" "${STASH_NPS:-N/A}"
printf "%-12s %10s nodes @ %10s nps\n" "Komodo:" "${KOMODO_NODES:-N/A}" "${KOMODO_NPS:-N/A}"
printf "%-12s %10s nodes @ %10s nps\n" "Laser:" "${LASER_NODES:-N/A}" "${LASER_NPS:-N/A}"

# Debug information if requested
if [ -n "$DEBUG" ]; then
    echo
    echo -e "${BOLD}=========================================="
    echo -e "Debug Information"
    echo -e "==========================================${NC}"
    echo
    echo -e "${BOLD}Last info lines captured:${NC}"
    echo -e "SeaJay: ${SEAJAY_INFO_LINE:-No info line found}"
    echo -e "Stash: ${STASH_INFO_LINE:-No info line found}"
    echo -e "Komodo: ${KOMODO_INFO_LINE:-No info line found}"
    echo -e "Laser: ${LASER_INFO_LINE:-No info line found}"
fi

# Principal Variation analysis
echo
echo -e "${BOLD}=========================================="
echo -e "Principal Variations (PV)"
echo -e "==========================================${NC}"
echo -e "${BOLD}What each engine is considering:${NC}"
echo

# Function to extract and format PV
format_pv() {
    local info_line="$1"
    local engine_name="$2"
    
    # Extract everything after "pv " until the end of line
    local pv=$(echo "$info_line" | sed -n 's/.*\bpv \(.*\)/\1/p')
    
    if [ -n "$pv" ]; then
        # Split PV into moves and show first 8 moves
        local moves=($pv)
        local display_pv=""
        local max_moves=8
        local count=0
        
        for move in "${moves[@]}"; do
            if [ $count -lt $max_moves ]; then
                display_pv="$display_pv $move"
                ((count++))
            else
                display_pv="$display_pv ..."
                break
            fi
        done
        
        echo -e "${BOLD}$engine_name:${NC}$display_pv"
    else
        echo -e "${BOLD}$engine_name:${NC} (no PV available)"
    fi
}

# Display PVs for each engine
if [ -n "$SEAJAY_INFO_LINE" ]; then
    format_pv "$SEAJAY_INFO_LINE" "SeaJay"
else
    echo -e "${BOLD}SeaJay:${NC} (no PV available)"
fi

if [ -n "$STASH_INFO_LINE" ]; then
    format_pv "$STASH_INFO_LINE" "Stash"
else
    echo -e "${BOLD}Stash:${NC} (no PV available)"
fi

if [ -n "$KOMODO_INFO_LINE" ]; then
    format_pv "$KOMODO_INFO_LINE" "Komodo"
else
    echo -e "${BOLD}Komodo:${NC} (no PV available)"
fi

if [ -n "$LASER_INFO_LINE" ]; then
    format_pv "$LASER_INFO_LINE" "Laser"
else
    echo -e "${BOLD}Laser:${NC} (no PV available)"
fi

# Move consensus analysis
echo
echo -e "${BOLD}Move consensus:${NC}"
# Count how many engines choose each move
declare -A move_count
declare -A move_engines

for engine_move in "$SEAJAY_BESTMOVE:SeaJay" "$STASH_BESTMOVE:Stash" "$KOMODO_BESTMOVE:Komodo" "$LASER_BESTMOVE:Laser"; do
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

# Optional: show full outputs location if saving
if [ "$SAVE_REPORT" = true ]; then
    echo
    echo -e "${BOLD}==========================================${NC}"
    echo -e "Full engine outputs saved in: $TEMP_DIR"
    echo -e "==========================================${NC}"
fi

# Save full report if requested
if [ "$SAVE_REPORT" = true ]; then
    {
        echo "==========================================
Four-Engine Position Analysis Report
Generated: $(date)
==========================================

FEN: $FEN
Side to move: $SIDE_TEXT
Search mode: $MODE = $VALUE

==========================================
Full Engine Outputs
==========================================

--- SeaJay ---"
        cat "$TEMP_DIR/seajay.out"
        echo "
--- Stash ---"
        cat "$TEMP_DIR/stash.out"
        echo "
--- Komodo ---"
        cat "$TEMP_DIR/komodo.out"
        echo "
--- Laser ---"
        cat "$TEMP_DIR/laser.out"
        echo "
==========================================
Summary Results
==========================================

Scores (as reported by engines):
SeaJay:     ${SEAJAY_UCI_SCORE:-N/A}cp
Stash:      ${STASH_UCI_SCORE:-N/A}cp
Komodo:     ${KOMODO_UCI_SCORE:-N/A}cp (gold standard)
Laser:      ${LASER_UCI_SCORE:-N/A}cp

Best Moves:
SeaJay:     ${SEAJAY_BESTMOVE:-N/A} (depth ${SEAJAY_DEPTH:-?})
Stash:      ${STASH_BESTMOVE:-N/A} (depth ${STASH_DEPTH:-?})
Komodo:     ${KOMODO_BESTMOVE:-N/A} (depth ${KOMODO_DEPTH:-?})
Laser:      ${LASER_BESTMOVE:-N/A} (depth ${LASER_DEPTH:-?})

Performance:
SeaJay:     ${SEAJAY_NODES:-N/A} nodes @ ${SEAJAY_NPS:-N/A} nps
Stash:      ${STASH_NODES:-N/A} nodes @ ${STASH_NPS:-N/A} nps
Komodo:     ${KOMODO_NODES:-N/A} nodes @ ${KOMODO_NPS:-N/A} nps
Laser:      ${LASER_NODES:-N/A} nodes @ ${LASER_NPS:-N/A} nps

==========================================
End of Report
=========================================="
    } > "$REPORT_FILE"

    echo
    echo -e "${GREEN}Full report saved to: $REPORT_FILE${NC}"
    echo -e "${GREEN}Raw outputs preserved in: $TEMP_DIR${NC}"
fi
