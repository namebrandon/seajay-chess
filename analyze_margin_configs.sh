#!/bin/bash
# Analyze pruning behavior of different margin configurations

echo "=== Stage 15 Day 8.1: Margin Configuration Analysis ==="
echo ""

# Test position with many captures available
TEST_FEN="r1bqk2r/pp2nppp/2n1p3/3p4/1bPP4/2N1PN2/PP3PPP/R1BQKB1R w KQkq - 0 8"

# Function to analyze a configuration
analyze_config() {
    local binary=$1
    local name=$2
    
    echo "Configuration: $name"
    echo "Binary: $binary"
    
    if [ ! -f "$binary" ]; then
        echo "  ERROR: Binary not found"
        return
    fi
    
    # Run analysis with aggressive SEE pruning mode
    echo "position fen $TEST_FEN
setoption name SEEPruning value aggressive
go depth 10
quit" | $binary 2>/dev/null | grep -E "info string SEE|info depth 10" | tail -2
    
    echo ""
}

echo "Test position: $TEST_FEN"
echo "This complex middlegame position has many capture possibilities"
echo ""

# Analyze each configuration
analyze_config "/workspace/binaries/seajay_stage15_margin_baseline" "Baseline (-100/-50/-25)"
analyze_config "/workspace/binaries/seajay_stage15_margin_conservative" "Conservative (-125/-75/-40)"
analyze_config "/workspace/binaries/seajay_stage15_margin_aggressive" "Aggressive (-75/-25/0)"
analyze_config "/workspace/binaries/seajay_stage15_margin_very_aggressive" "Very Aggressive (-50/0/25)"
analyze_config "/workspace/binaries/seajay_stage15_margin_stockfish" "Stockfish-like (-90/-40/-20)"

echo "=== Quick Self-Play Test ==="
echo ""

# Run a very quick match between baseline and aggressive
ENGINE1="/workspace/binaries/seajay_stage15_margin_baseline"
ENGINE2="/workspace/binaries/seajay_stage15_margin_aggressive"

echo "Quick 5-game match: Baseline vs Aggressive"
echo ""

for game in 1 2 3 4 5; do
    echo -n "Game $game: "
    
    # Random opening position
    case $game in
        1) STARTPOS="startpos" ;;
        2) STARTPOS="fen rnbqkbnr/pppp1ppp/8/4p3/4P3/8/PPPP1PPP/RNBQKBNR w KQkq - 0 2" ;;
        3) STARTPOS="fen r1bqkbnr/pppp1ppp/2n5/4p3/4P3/5N2/PPPP1PPP/RNBQKB1R w KQkq - 2 3" ;;
        4) STARTPOS="fen rnbqkb1r/pppp1ppp/5n2/4p3/2B1P3/8/PPPP1PPP/RNBQK1NR w KQkq - 2 3" ;;
        5) STARTPOS="fen r1bqkbnr/pppp1ppp/2n5/1B2p3/4P3/5N2/PPPP1PPP/RNBQK2R b KQkq - 3 3" ;;
    esac
    
    # Play game with 1 second per move
    RESULT=$(echo "position $STARTPOS
go movetime 1000
quit" | $ENGINE1 2>/dev/null | grep "bestmove" | awk '{print $2}')
    
    echo "Engine1 plays: $RESULT"
done

echo ""
echo "=== Pruning Statistics Comparison ==="
echo ""

# Compare pruning rates at different depths
for depth in 6 8 10; do
    echo "Depth $depth pruning rates:"
    
    for config in baseline aggressive very_aggressive; do
        binary="/workspace/binaries/seajay_stage15_margin_${config}"
        
        PRUNE_RATE=$(echo "position fen $TEST_FEN
setoption name SEEPruning value aggressive
go depth $depth
quit" | $binary 2>/dev/null | grep "info string SEE" | tail -1 | grep -oE "[0-9]+\.[0-9]+%")
        
        printf "  %-20s: %s\n" "$config" "${PRUNE_RATE:-N/A}"
    done
    echo ""
done