#!/bin/bash

echo "Testing static null move margin at shallow depths (where it triggers)"
echo "======================================================================"
echo ""

# Test at depths 1-3 where static null move actually triggers
positions=(
    "rnbqkbnr/pppp1ppp/8/4p3/4P3/5N2/PPPP1PPP/RNBQKB1R b KQkq - 1 2"
    "r1bqkb1r/pppp1ppp/2n2n2/4p3/2B1P3/3P1N2/PPP2PPP/RNBQK2R b KQkq - 0 5"
    "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1"
)

descriptions=(
    "Opening position"
    "Italian game"
    "Complex middle"
)

for depth in 2 3 6 8; do
    echo "DEPTH $depth:"
    echo "----------"
    
    # Test with 120
    git checkout -- src/uci/uci.h src/search/types.h 2>/dev/null
    ./build.sh > /dev/null 2>&1
    
    echo "  120cp margin:"
    for i in ${!positions[@]}; do
        nodes=$(echo "position fen ${positions[$i]}
go depth $depth" | ./bin/seajay 2>/dev/null | grep "info depth $depth" | tail -1 | awk '{for(i=1;i<=NF;i++) if($i=="nodes") print $(i+1)}')
        printf "    %-15s: %8s nodes\n" "${descriptions[$i]}" "$nodes"
    done
    
    # Test with 90
    sed -i 's/int m_nullMoveStaticMargin = 120/int m_nullMoveStaticMargin = 90/' src/uci/uci.h
    sed -i 's/int nullMoveStaticMargin = 120/int nullMoveStaticMargin = 90/' src/search/types.h
    ./build.sh > /dev/null 2>&1
    
    echo "  90cp margin:"
    for i in ${!positions[@]}; do
        nodes=$(echo "position fen ${positions[$i]}
go depth $depth" | ./bin/seajay 2>/dev/null | grep "info depth $depth" | tail -1 | awk '{for(i=1;i<=NF;i++) if($i=="nodes") print $(i+1)}')
        printf "    %-15s: %8s nodes\n" "${descriptions[$i]}" "$nodes"
    done
    
    echo ""
done

# Restore 90cp
sed -i 's/int m_nullMoveStaticMargin = 120/int m_nullMoveStaticMargin = 90/' src/uci/uci.h
sed -i 's/int nullMoveStaticMargin = 120/int nullMoveStaticMargin = 90/' src/search/types.h