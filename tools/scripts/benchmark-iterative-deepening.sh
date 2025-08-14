#!/bin/bash

# Benchmark script for Stage 13: Iterative Deepening
# Tracks NPS performance to ensure no regression

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Configuration
SEAJAY_BIN="${1:-./build/bin/seajay}"
DEPTH="${2:-10}"
ITERATIONS="${3:-5}"

# Test positions (from the plan)
declare -a POSITIONS=(
    "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"  # Startpos
    "r1bqkb1r/pppp1ppp/2n2n2/1B2p3/4P3/5N2/PPPP1PPP/RNBQK2R w KQkq - 4 4"  # Lasker's Sacrifice
    "r1b1k2r/2qnbppp/p2ppn2/1p4B1/3NP3/2N2Q2/PPP2PPP/2KR1B1R w kq - 0 11"  # Wild Sicilian
    "8/8/4kpp1/3p1b2/p6P/2B5/6P1/6K1 b - - 0 47"  # Karpov's Fortress
    "r3k2r/pb1nbppp/1p2pn2/q3N3/2BP4/2N1P3/PP2QPPP/R3K2R w KQkq - 0 12"  # Horizon Effect
)

declare -a POSITION_NAMES=(
    "Starting Position"
    "Lasker's Sacrifice"
    "Wild Sicilian"
    "Karpov's Fortress"
    "Horizon Effect Test"
)

# Check if binary exists
if [ ! -f "$SEAJAY_BIN" ]; then
    echo -e "${RED}Error: SeaJay binary not found at $SEAJAY_BIN${NC}"
    echo "Please build the project first: cd build && cmake .. && make"
    exit 1
fi

echo "========================================="
echo "Stage 13: Iterative Deepening Benchmark"
echo "========================================="
echo "Binary: $SEAJAY_BIN"
echo "Depth: $DEPTH"
echo "Iterations per position: $ITERATIONS"
echo ""

# Create results file
RESULTS_FILE="benchmark_results_$(date +%Y%m%d_%H%M%S).txt"
echo "Results will be saved to: $RESULTS_FILE"
echo ""

# Function to run a single benchmark
run_benchmark() {
    local fen="$1"
    local name="$2"
    local total_nodes=0
    local total_time=0
    local min_nps=999999999
    local max_nps=0
    
    echo -e "${YELLOW}Testing: $name${NC}"
    echo "FEN: $fen"
    
    for i in $(seq 1 $ITERATIONS); do
        # Run search and capture output
        output=$(echo -e "position fen $fen\ngo depth $DEPTH\nquit" | $SEAJAY_BIN 2>/dev/null | grep "info depth $DEPTH")
        
        # Extract nodes and time from last depth line
        if [ ! -z "$output" ]; then
            nodes=$(echo "$output" | grep -oP 'nodes \K\d+' | tail -1)
            time_ms=$(echo "$output" | grep -oP 'time \K\d+' | tail -1)
            nps=$(echo "$output" | grep -oP 'nps \K\d+' | tail -1)
            
            if [ ! -z "$nodes" ] && [ ! -z "$time_ms" ] && [ ! -z "$nps" ]; then
                total_nodes=$((total_nodes + nodes))
                total_time=$((total_time + time_ms))
                
                if [ $nps -lt $min_nps ]; then
                    min_nps=$nps
                fi
                if [ $nps -gt $max_nps ]; then
                    max_nps=$nps
                fi
                
                echo "  Iteration $i: $nodes nodes in ${time_ms}ms = $nps nps"
            else
                echo -e "  ${RED}Iteration $i: Failed to parse output${NC}"
            fi
        else
            echo -e "  ${RED}Iteration $i: No output received${NC}"
        fi
    done
    
    # Calculate averages
    if [ $total_time -gt 0 ]; then
        avg_nps=$((total_nodes * 1000 / total_time))
        avg_nodes=$((total_nodes / ITERATIONS))
        avg_time=$((total_time / ITERATIONS))
        
        echo -e "${GREEN}  Average: $avg_nodes nodes in ${avg_time}ms = $avg_nps nps${NC}"
        echo "  Range: $min_nps - $max_nps nps"
        
        # Save to results file
        echo "$name|$fen|$avg_nodes|$avg_time|$avg_nps|$min_nps|$max_nps" >> $RESULTS_FILE
    else
        echo -e "${RED}  No valid measurements obtained${NC}"
    fi
    
    echo ""
}

# Write header to results file
echo "Position|FEN|Avg_Nodes|Avg_Time_ms|Avg_NPS|Min_NPS|Max_NPS" > $RESULTS_FILE

# Store baseline NPS for comparison
BASELINE_TOTAL_NPS=0
POSITION_COUNT=0

# Run benchmarks for each position
for i in "${!POSITIONS[@]}"; do
    run_benchmark "${POSITIONS[$i]}" "${POSITION_NAMES[$i]}"
    
    # Extract average NPS for baseline calculation
    if [ -f $RESULTS_FILE ]; then
        last_nps=$(tail -1 $RESULTS_FILE | cut -d'|' -f5)
        if [ ! -z "$last_nps" ] && [ "$last_nps" != "Avg_NPS" ]; then
            BASELINE_TOTAL_NPS=$((BASELINE_TOTAL_NPS + last_nps))
            POSITION_COUNT=$((POSITION_COUNT + 1))
        fi
    fi
done

# Calculate and display overall baseline
if [ $POSITION_COUNT -gt 0 ]; then
    BASELINE_AVG_NPS=$((BASELINE_TOTAL_NPS / POSITION_COUNT))
    echo "========================================="
    echo -e "${GREEN}Overall Average NPS: $BASELINE_AVG_NPS${NC}"
    echo "========================================="
    
    # Save baseline to file for future comparison
    echo "$BASELINE_AVG_NPS" > baseline_nps.txt
    echo "Baseline saved to baseline_nps.txt"
    
    # Check if we have a previous baseline to compare
    if [ -f baseline_nps_previous.txt ]; then
        PREVIOUS_NPS=$(cat baseline_nps_previous.txt)
        DIFF=$((BASELINE_AVG_NPS - PREVIOUS_NPS))
        PERCENT=$((DIFF * 100 / PREVIOUS_NPS))
        
        if [ $PERCENT -lt -5 ]; then
            echo -e "${RED}WARNING: NPS decreased by ${PERCENT}% from baseline!${NC}"
            echo "Previous: $PREVIOUS_NPS nps"
            echo "Current:  $BASELINE_AVG_NPS nps"
            exit 1
        elif [ $PERCENT -gt 5 ]; then
            echo -e "${GREEN}NPS improved by ${PERCENT}% from baseline${NC}"
        else
            echo -e "${YELLOW}NPS within 5% of baseline (${PERCENT}% change)${NC}"
        fi
    fi
fi

echo ""
echo "Benchmark complete. Results saved to $RESULTS_FILE"