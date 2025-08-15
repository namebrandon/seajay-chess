#!/bin/bash
# Stage 15 Day 6.4: SEE Pruning Performance Validation

echo "=== Stage 15 Day 6.4: SEE Pruning Performance Validation ==="
echo ""

SEAJAY="/workspace/bin/seajay"

# Test positions
declare -a POSITIONS=(
    "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"
    "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1"
    "r1bqkb1r/pppp1ppp/2n2n2/1B2p3/4P3/5N2/PPPP1PPP/RNBQK2R w KQkq - 4 4"
    "r3k2r/ppp1ppbp/2n3p1/8/3PP1b1/2N2N2/PPP2PPP/R1B1KB1R w KQkq - 0 1"
)

declare -a NAMES=(
    "Starting position"
    "Kiwipete (tactical)"
    "Italian Game"
    "Complex tactical"
)

# Test each pruning mode
declare -a MODES=("off" "conservative" "aggressive")

echo "Testing with depth 6..."
echo ""

for mode in "${MODES[@]}"; do
    echo "=== SEE Pruning Mode: $mode ==="
    echo "----------------------------------------"
    
    for i in "${!POSITIONS[@]}"; do
        echo "Position: ${NAMES[$i]}"
        
        # Run search and capture output
        result=$(echo -e "uci\nsetoption name SEEPruning value $mode\nsetoption name SEEMode value production\nposition fen ${POSITIONS[$i]}\ngo depth 6\nquit" | $SEAJAY 2>&1)
        
        # Extract statistics
        nodes=$(echo "$result" | grep "info depth 6" | tail -1 | sed -n 's/.*nodes \([0-9]*\).*/\1/p')
        nps=$(echo "$result" | grep "info depth 6" | tail -1 | sed -n 's/.*nps \([0-9]*\).*/\1/p')
        time=$(echo "$result" | grep "info depth 6" | tail -1 | sed -n 's/.*time \([0-9]*\).*/\1/p')
        pruned=$(echo "$result" | grep "SEE pruned" | tail -1 | sed -n 's/.*SEE pruned \([0-9]*\).*/\1/p')
        prune_rate=$(echo "$result" | grep "SEE pruned" | tail -1 | sed -n 's/.*(\([0-9.]*\)%).*/\1/p')
        
        echo "  Nodes: $nodes, Time: ${time}ms, NPS: $nps"
        if [ "$mode" != "off" ] && [ -n "$pruned" ]; then
            echo "  Pruned: $pruned captures ($prune_rate%)"
        fi
        echo ""
    done
    echo ""
done

echo "=== Tactical Suite Test ==="
echo "Testing tactical awareness with each mode..."
echo ""

# Simple tactical position - white wins material
TACTICAL_FEN="r2qkbnr/ppp2ppp/2n5/3p4/2BPp3/5N2/PPP2PPP/RNBQK2R w KQkq - 0 1"

for mode in "${MODES[@]}"; do
    echo "Mode: $mode"
    result=$(echo -e "uci\nsetoption name SEEPruning value $mode\nsetoption name SEEMode value production\nposition fen $TACTICAL_FEN\ngo depth 8\nquit" | $SEAJAY 2>&1)
    
    bestmove=$(echo "$result" | grep "bestmove" | awk '{print $2}')
    score=$(echo "$result" | grep "info depth 8" | tail -1 | sed -n 's/.*score cp \([0-9-]*\).*/\1/p')
    
    echo "  Best move: $bestmove, Score: ${score}cp"
done

echo ""
echo "Performance validation complete!"