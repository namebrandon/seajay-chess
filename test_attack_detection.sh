#!/bin/bash

# Test script specifically for attack detection validation
# Tests positions that heavily exercise isSquareAttacked

echo "==================================================="
echo "Attack Detection Validation Test"
echo "==================================================="
echo ""

# Test a position with many checks and pins (exercises isSquareAttacked heavily)
echo "Testing position with checks and pins..."
./bin/perft_tool "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -" 3 2>&1 | grep -E "Result:|Error"

echo ""
echo "Testing position with discovered checks..."
./bin/perft_tool "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ -" 3 2>&1 | grep -E "Result:|Error"

echo ""
echo "Running standard test suite to depth 4..."
./bin/perft_tool --suite --max-depth 4 2>&1 | tail -5

echo ""
echo "==================================================="
echo "If all tests show results, attack detection is working correctly."
echo "==================================================="