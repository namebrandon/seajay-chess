#!/bin/bash

# Direct validation of move ordering improvement
echo "====================================="
echo "Direct Move Ordering Efficiency Test"
echo "====================================="
echo

# Test a tactical position to see move ordering efficiency
FEN="r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1"

echo "Testing Kiwipete position (very tactical):"
echo "FEN: $FEN"
echo
echo "Analyzing to depth 6 and checking efficiency:"
echo

output=$(echo -e "position fen $FEN\ngo depth 6\nquit" | ./bin/seajay 2>&1)

# Extract move ordering efficiency
echo "$output" | grep -E "moveeff|tthits" | tail -5

echo
echo "Key metrics:"
echo "- moveeff: Move ordering efficiency (target: 85%+)"
echo "- tthits: Transposition table hit rate"
echo

# Now let's look at the best efficiency achieved
best_eff=$(echo "$output" | grep "moveeff" | sed 's/.*moveeff //' | sed 's/%.*/%/' | sort -t% -k1 -rn | head -1)
echo "Best move ordering efficiency achieved: $best_eff"

echo
echo "====================================="