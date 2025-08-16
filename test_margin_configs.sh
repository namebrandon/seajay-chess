#!/bin/bash
# Stage 15 Day 8.1: Build and test different margin configurations

CUTECHESS="/workspace/external/cutechess-cli/cutechess-cli"
BASE_BINARY="/workspace/binaries/seajay_stage15_bias_bugfix2"
OUTPUT_DIR="/workspace/project_docs/stage_implementations/stage_15_see/tuning_results"
BUILD_DIR="/workspace/build"
TIME_CONTROL="10+0.1"
GAMES=20  # Quick test with 20 games per config
THREADS=2

mkdir -p "$OUTPUT_DIR"

echo "=== Stage 15 Day 8.1: SEE Margin Tuning ==="
echo "Building and testing different margin configurations"
echo ""

# Function to modify margins and rebuild
build_with_margins() {
    local name=$1
    local conservative=$2
    local aggressive=$3
    local endgame=$4
    
    echo "Building configuration: $name"
    echo "  Conservative: $conservative"
    echo "  Aggressive: $aggressive"
    echo "  Endgame: $endgame"
    
    # Backup original file
    cp /workspace/src/search/quiescence.h /workspace/src/search/quiescence.h.backup
    
    # Modify the thresholds
    sed -i "s/SEE_PRUNE_THRESHOLD_CONSERVATIVE = -[0-9]*/SEE_PRUNE_THRESHOLD_CONSERVATIVE = $conservative/" \
        /workspace/src/search/quiescence.h
    sed -i "s/SEE_PRUNE_THRESHOLD_AGGRESSIVE = -[0-9]*/SEE_PRUNE_THRESHOLD_AGGRESSIVE = $aggressive/" \
        /workspace/src/search/quiescence.h
    sed -i "s/SEE_PRUNE_THRESHOLD_ENDGAME = -[0-9]*/SEE_PRUNE_THRESHOLD_ENDGAME = $endgame/" \
        /workspace/src/search/quiescence.h
    
    # Build
    cd "$BUILD_DIR"
    make clean > /dev/null 2>&1
    cmake .. -DCMAKE_BUILD_TYPE=Release > /dev/null 2>&1
    make -j4 seajay > /dev/null 2>&1
    
    if [ $? -eq 0 ]; then
        # Copy binary with name
        cp /workspace/bin/seajay "/workspace/binaries/seajay_stage15_margin_${name}"
        echo "  Built successfully: /workspace/binaries/seajay_stage15_margin_${name}"
        return 0
    else
        echo "  Build failed!"
        return 1
    fi
}

# Function to run a match between two configurations
run_match() {
    local engine1=$1
    local engine2=$2
    local name=$3
    
    echo ""
    echo "Running match: $name"
    echo "  Engine 1: $engine1"
    echo "  Engine 2: $engine2"
    
    if [ ! -f "$CUTECHESS" ]; then
        echo "  ERROR: cutechess-cli not found. Please run setup-external-tools.sh"
        return 1
    fi
    
    OUTPUT_FILE="${OUTPUT_DIR}/match_${name}.pgn"
    
    $CUTECHESS -engine cmd="$engine1" name="Config1" \
               -engine cmd="$engine2" name="Config2" \
               -each proto=uci tc=$TIME_CONTROL \
               -games $GAMES -repeat -concurrency $THREADS \
               -pgnout "$OUTPUT_FILE" \
               -recover -draw movenumber=40 movecount=5 score=10 \
               2>&1 | grep -E "Score|Elo|Games"
    
    echo "  Results saved to: $OUTPUT_FILE"
}

# Test configurations
echo "=== Building Test Configurations ==="
echo ""

# 1. Baseline (current values)
build_with_margins "baseline" -100 -50 -25

# 2. More conservative
build_with_margins "conservative" -125 -75 -40

# 3. More aggressive
build_with_margins "aggressive" -75 -25 0

# 4. Very aggressive
build_with_margins "very_aggressive" -50 0 25

# 5. Stockfish-like
build_with_margins "stockfish" -90 -40 -20

# Restore original file
cp /workspace/src/search/quiescence.h.backup /workspace/src/search/quiescence.h

echo ""
echo "=== Running Test Matches ==="

# If we have cutechess, run matches
if [ -f "$CUTECHESS" ]; then
    # Test each configuration against baseline
    run_match "/workspace/binaries/seajay_stage15_margin_baseline" \
              "/workspace/binaries/seajay_stage15_margin_conservative" \
              "baseline_vs_conservative"
    
    run_match "/workspace/binaries/seajay_stage15_margin_baseline" \
              "/workspace/binaries/seajay_stage15_margin_aggressive" \
              "baseline_vs_aggressive"
    
    run_match "/workspace/binaries/seajay_stage15_margin_baseline" \
              "/workspace/binaries/seajay_stage15_margin_very_aggressive" \
              "baseline_vs_very_aggressive"
    
    run_match "/workspace/binaries/seajay_stage15_margin_baseline" \
              "/workspace/binaries/seajay_stage15_margin_stockfish" \
              "baseline_vs_stockfish"
else
    echo "Cutechess-cli not found. Skipping matches."
    echo "To run matches, install cutechess-cli first:"
    echo "  ./tools/scripts/setup-external-tools.sh"
fi

echo ""
echo "=== Summary ==="
echo ""
echo "Built configurations:"
ls -la /workspace/binaries/seajay_stage15_margin_* 2>/dev/null | awk '{print $9}'

echo ""
echo "To run more comprehensive tests:"
echo "1. Increase GAMES variable (currently $GAMES)"
echo "2. Use longer time control (currently $TIME_CONTROL)"
echo "3. Run round-robin tournament between all configs"

echo ""
echo "Recommended next steps:"
echo "1. Analyze game results for win rates"
echo "2. Check pruning statistics from each engine"
echo "3. Select best performing configuration"
echo "4. Run SPRT test with selected values"