#!/bin/bash
# Simple A/B comparison of SEE pruning modes

echo "=== SEE Pruning Mode Comparison ==="
echo ""

SEAJAY="/workspace/bin/seajay"
DEPTH=8

# Test positions for comparison
declare -a POSITIONS=(
    "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"
    "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1"
    "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1"
    "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1"
    "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8"
)

echo "Testing each mode at depth $DEPTH..."
echo ""

for mode in "off" "conservative" "aggressive"; do
    echo "Mode: $mode"
    total_nodes=0
    total_time=0
    
    for pos in "${POSITIONS[@]}"; do
        result=$(echo -e "uci\nsetoption name SEEPruning value $mode\nsetoption name SEEMode value production\nposition fen $pos\ngo depth $DEPTH\nquit" | $SEAJAY 2>&1)
        
        nodes=$(echo "$result" | grep "info depth $DEPTH" | tail -1 | sed -n 's/.*nodes \([0-9]*\).*/\1/p')
        time=$(echo "$result" | grep "info depth $DEPTH" | tail -1 | sed -n 's/.*time \([0-9]*\).*/\1/p')
        
        if [ -n "$nodes" ]; then
            total_nodes=$((total_nodes + nodes))
        fi
        if [ -n "$time" ]; then
            total_time=$((total_time + time))
        fi
    done
    
    echo "  Total nodes: $total_nodes"
    echo "  Total time: ${total_time}ms"
    if [ $total_time -gt 0 ]; then
        avg_nps=$((total_nodes * 1000 / total_time))
        echo "  Average NPS: $avg_nps"
    fi
    echo ""
done

echo "Comparison complete!"
