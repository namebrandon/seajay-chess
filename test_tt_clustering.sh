#!/bin/bash

# Test script for TT clustering validation
# Compares performance with and without clustering

echo "=== TT Clustering Validation Test ==="
echo "Date: $(date)"
echo ""

# Test positions (mix of opening, middlegame, endgame, tactical)
FENS=(
    "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"
    "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1"
    "r1bqkb1r/pp1ppppp/2n2n2/8/3P4/5N2/PPP1PPPP/RNBQKB1R w KQkq - 4 4"
    "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1"
    "4r1k1/r1q2ppp/ppp2n2/4P3/5Rb1/1N1BQ3/PPP3PP/R5K1 w - - 1 1"
    "2kr3r/p1ppqpb1/bn2Qnp1/3PN3/1p2P3/2N5/PPPBBPPP/R3K2R b KQ - 0 1"
    "rnb2k1r/pp1Pbppp/2p5/q7/2B5/8/PPPQNpPP/RNB1K2R w KQ - 3 9"
    "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1"
)

FEN_NAMES=(
    "startpos"
    "kiwipete"
    "italian"
    "endgame"
    "tactical1"
    "tactical2"
    "tactical3"
    "tactical4"
)

# Time control for each position (milliseconds)
TIME_MS=3000

# Output files
RESULT_FILE="tt_clustering_results.txt"
CSV_FILE="tt_clustering_comparison.csv"

# Clear result files
> $RESULT_FILE
> $CSV_FILE

# CSV header
echo "Position,FEN,Mode,Depth,Nodes,Time_ms,NPS,TT_Hits,TT_Hit_Rate" >> $CSV_FILE

echo "Testing ${#FENS[@]} positions with ${TIME_MS}ms per position..."
echo ""

for i in ${!FENS[@]}; do
    FEN="${FENS[$i]}"
    NAME="${FEN_NAMES[$i]}"
    
    echo "Position $((i+1))/${#FENS[@]}: $NAME"
    echo "----------------------------------------" | tee -a $RESULT_FILE
    echo "Position: $NAME" | tee -a $RESULT_FILE
    echo "FEN: $FEN" | tee -a $RESULT_FILE
    echo "" | tee -a $RESULT_FILE
    
    # Test WITHOUT clustering
    echo "Testing WITHOUT clustering..." | tee -a $RESULT_FILE
    OUTPUT=$(echo -e "uci\nsetoption name Hash value 128\nsetoption name UseClusteredTT value false\nucinewgame\nposition fen $FEN\ngo movetime $TIME_MS\ndebug tt\nquit" | ./bin/seajay 2>&1)
    
    # Extract info from output
    DEPTH=$(echo "$OUTPUT" | grep -oE "info depth [0-9]+" | tail -1 | grep -oE "[0-9]+$")
    NODES=$(echo "$OUTPUT" | grep -oE "nodes [0-9]+" | tail -1 | grep -oE "[0-9]+$")
    NPS=$(echo "$OUTPUT" | grep -oE "nps [0-9]+" | tail -1 | grep -oE "[0-9]+$")
    TT_PROBES=$(echo "$OUTPUT" | grep "TT probes:" | grep -oE "[0-9]+")
    TT_HITS=$(echo "$OUTPUT" | grep "TT hits:" | grep -oE "[0-9]+")
    
    if [ -n "$TT_PROBES" ] && [ -n "$TT_HITS" ] && [ "$TT_PROBES" -gt 0 ]; then
        TT_HIT_RATE=$(echo "scale=2; $TT_HITS * 100 / $TT_PROBES" | bc)
    else
        TT_HIT_RATE="0"
    fi
    
    echo "  Non-clustered: Depth=$DEPTH, Nodes=$NODES, NPS=$NPS, TT_Hit_Rate=${TT_HIT_RATE}%" | tee -a $RESULT_FILE
    echo "$NAME,$FEN,non-clustered,$DEPTH,$NODES,$TIME_MS,$NPS,$TT_HITS,$TT_HIT_RATE" >> $CSV_FILE
    
    # Test WITH clustering
    echo "Testing WITH clustering..." | tee -a $RESULT_FILE
    OUTPUT=$(echo -e "uci\nsetoption name Hash value 128\nsetoption name UseClusteredTT value true\nucinewgame\nposition fen $FEN\ngo movetime $TIME_MS\ndebug tt\nquit" | ./bin/seajay 2>&1)
    
    # Extract info from output
    DEPTH=$(echo "$OUTPUT" | grep -oE "info depth [0-9]+" | tail -1 | grep -oE "[0-9]+$")
    NODES=$(echo "$OUTPUT" | grep -oE "nodes [0-9]+" | tail -1 | grep -oE "[0-9]+$")
    NPS=$(echo "$OUTPUT" | grep -oE "nps [0-9]+" | tail -1 | grep -oE "[0-9]+$")
    TT_PROBES=$(echo "$OUTPUT" | grep "TT probes:" | grep -oE "[0-9]+")
    TT_HITS=$(echo "$OUTPUT" | grep "TT hits:" | grep -oE "[0-9]+")
    
    if [ -n "$TT_PROBES" ] && [ -n "$TT_HITS" ] && [ "$TT_PROBES" -gt 0 ]; then
        TT_HIT_RATE=$(echo "scale=2; $TT_HITS * 100 / $TT_PROBES" | bc)
    else
        TT_HIT_RATE="0"
    fi
    
    echo "  Clustered:     Depth=$DEPTH, Nodes=$NODES, NPS=$NPS, TT_Hit_Rate=${TT_HIT_RATE}%" | tee -a $RESULT_FILE
    echo "$NAME,$FEN,clustered,$DEPTH,$NODES,$TIME_MS,$NPS,$TT_HITS,$TT_HIT_RATE" >> $CSV_FILE
    echo "" | tee -a $RESULT_FILE
done

echo ""
echo "=== Summary ==="
echo "Results saved to: $RESULT_FILE"
echo "CSV data saved to: $CSV_FILE"
echo ""

# Analyze results
echo "Analyzing improvements..."
python3 -c "
import csv
import sys

with open('$CSV_FILE', 'r') as f:
    reader = csv.DictReader(f)
    data = list(reader)

positions = {}
for row in data:
    pos = row['Position']
    if pos not in positions:
        positions[pos] = {}
    positions[pos][row['Mode']] = row

improvements = []
for pos, modes in positions.items():
    if 'non-clustered' in modes and 'clustered' in modes:
        nc = modes['non-clustered']
        c = modes['clustered']
        
        nodes_diff = int(c['Nodes']) - int(nc['Nodes']) if c['Nodes'] and nc['Nodes'] else 0
        nodes_pct = (nodes_diff / int(nc['Nodes']) * 100) if nc['Nodes'] and int(nc['Nodes']) > 0 else 0
        
        hit_rate_diff = float(c['TT_Hit_Rate']) - float(nc['TT_Hit_Rate']) if c['TT_Hit_Rate'] and nc['TT_Hit_Rate'] else 0
        
        print(f'{pos:12} - Nodes: {nodes_pct:+6.1f}%, TT Hit Rate: {hit_rate_diff:+5.2f}%')
        improvements.append((nodes_pct, hit_rate_diff))

if improvements:
    avg_node_change = sum(n for n, _ in improvements) / len(improvements)
    avg_hit_rate_change = sum(h for _, h in improvements) / len(improvements)
    print(f'')
    print(f'Average node change: {avg_node_change:+.1f}%')
    print(f'Average TT hit rate improvement: {avg_hit_rate_change:+.2f}%')
"