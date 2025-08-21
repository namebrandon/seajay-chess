#!/bin/bash

# Test script to demonstrate PVS bug
# This shows that re-search condition is wrong

echo "==================================="
echo "PVS Bug Demonstration Test"
echo "==================================="
echo ""

# First, let's see current behavior
echo "1. Testing CURRENT implementation (with bug):"
echo "----------------------------------------------"
echo ""

# Test tactical position where multiple moves might be good
echo "Tactical position test (expecting 10-20% re-search rate):"
./bin/seajay << EOF | grep -E "(PVS|re-search)"
uci
setoption name ShowPVSStats value true
position fen r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1
go depth 6
quit
EOF

echo ""
echo "Middle game position test (expecting 8-15% re-search rate):"
./bin/seajay << EOF | grep -E "(PVS|re-search)"
uci
setoption name ShowPVSStats value true
position fen r1bq1rk1/pp2ppbp/2np1np1/8/3PP3/2N2N2/PPP2PPP/R1BQKB1R w KQ - 0 8
go depth 7
quit
EOF

echo ""
echo "2. To apply the fix:"
echo "--------------------"
echo "Edit src/search/negamax.cpp line 531:"
echo "  CHANGE: if (score > alpha && score < beta)"
echo "  TO:     if (score > alpha)"
echo ""
echo "Then rebuild and run this test again."
echo ""

echo "3. Expected results after fix:"
echo "-------------------------------"
echo "- Tactical positions: 10-20% re-search rate"
echo "- Middle game: 8-15% re-search rate"  
echo "- Quiet positions: 2-8% re-search rate"
echo "- Overall average: 5-15% re-search rate"
echo ""

echo "4. Why this matters:"
echo "---------------------"
echo "Current: +19.55 ELO gain (with bug)"
echo "Expected: +30-40 ELO gain (after fix)"
echo "Missing: ~15-20 ELO due to wrong re-search condition"
echo ""

# Add diagnostic to understand what's happening
echo "5. Diagnostic: What scores are being returned?"
echo "-----------------------------------------------"
echo "Adding diagnostic output to see scout search results..."
echo ""

# Create a simple position where we can trace the logic
echo "Simple position with clear best move:"
./bin/seajay << EOF 2>&1 | head -50
uci
setoption name ShowPVSStats value true
position startpos moves e2e4 e7e5 g1f3 b8c6
go depth 5
quit
EOF