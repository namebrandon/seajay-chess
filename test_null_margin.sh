#!/bin/bash

echo "Testing impact of null move static margin change (120 -> 90)"
echo "============================================================"

# Test positions that should show difference
positions=(
    # Position 1: Quiet middlegame where null move is likely
    "rnbqkbnr/pppp1ppp/8/4p3/4P3/5N2/PPPP1PPP/RNBQKB1R b KQkq - 1 2"
    
    # Position 2: Side with space advantage
    "r1bqkb1r/pppp1ppp/2n2n2/4p3/2B1P3/3P1N2/PPP2PPP/RNBQK2R b KQkq - 0 5"
    
    # Position 3: Material imbalance position
    "r1bqkb1r/ppp2ppp/2n5/3p4/2BPn3/5N2/PPP2PPP/RNBQK2R w KQkq - 0 6"
    
    # Position 4: Early endgame
    "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1"
    
    # Position 5: Complex tactical position
    "r2q1rk1/ppp2ppp/2n1bn2/2bpp3/3P4/2N1PN2/PPP1BPPP/R1BQK2R w KQ - 0 8"
)

descriptions=(
    "Quiet middlegame"
    "Space advantage" 
    "Material imbalance"
    "Early endgame"
    "Complex tactical"
)

# First test with original setting (120)
echo "Testing with ORIGINAL setting (120cp margin):"
echo ""
git checkout -- src/uci/uci.h src/search/types.h 2>/dev/null
./build.sh > /dev/null 2>&1

for i in ${!positions[@]}; do
    echo -n "${descriptions[$i]}: "
    nodes=$(echo "position fen ${positions[$i]}
go depth 10" | ./bin/seajay 2>/dev/null | grep "info depth 10" | awk '{for(i=1;i<=NF;i++) if($i=="nodes") print $(i+1)}')
    echo "$nodes nodes"
done

echo ""
echo "Testing with NEW setting (90cp margin):"
echo ""

# Apply the change
sed -i 's/int m_nullMoveStaticMargin = 120/int m_nullMoveStaticMargin = 90/' src/uci/uci.h
sed -i 's/int nullMoveStaticMargin = 120/int nullMoveStaticMargin = 90/' src/search/types.h
./build.sh > /dev/null 2>&1

for i in ${!positions[@]}; do
    echo -n "${descriptions[$i]}: "
    nodes=$(echo "position fen ${positions[$i]}
go depth 10" | ./bin/seajay 2>/dev/null | grep "info depth 10" | awk '{for(i=1;i<=NF;i++) if($i=="nodes") print $(i+1)}')
    echo "$nodes nodes"
done

# Restore the 90cp change (as committed)
sed -i 's/int m_nullMoveStaticMargin = 120/int m_nullMoveStaticMargin = 90/' src/uci/uci.h
sed -i 's/int nullMoveStaticMargin = 120/int nullMoveStaticMargin = 90/' src/search/types.h