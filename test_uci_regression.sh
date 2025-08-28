#!/bin/bash

# DEBUG_UCI_REGRESSION: Test script to measure performance impact

echo "===== UCI Regression Performance Test ====="
echo "Testing position: r1bqkbnr/pppp1ppp/2n5/4p3/4P3/5N2/PPPP1PPP/RNBQKB1R w KQkq - 0 4"
echo ""

# Run the test position
echo "Running search with depth 10..."
echo "position fen r1bqkbnr/pppp1ppp/2n5/4p3/4P3/5N2/PPPP1PPP/RNBQKB1R w KQkq - 0 4
go depth 10
quit" | ./seajay 2>&1 | grep -E "(DEBUG_UCI_REGRESSION|info depth|nps|nodes)"

echo ""
echo "===== Another test position (Black to move) ====="
echo "Testing position: r1bqkb1r/pppp1ppp/2n2n2/4p3/2B1P3/5N2/PPPP1PPP/RNBQK2R b KQkq - 0 4"
echo ""

echo "Running search with depth 10..."
echo "position fen r1bqkb1r/pppp1ppp/2n2n2/4p3/2B1P3/5N2/PPPP1PPP/RNBQK2R b KQkq - 0 4
go depth 10
quit" | ./seajay 2>&1 | grep -E "(DEBUG_UCI_REGRESSION|info depth|nps|nodes)"

echo ""
echo "===== Performance Summary ====="
echo "Check the output above for:"
echo "1. Dynamic cast overhead (CastTime)"
echo "2. Info building overhead (BuilderTime + StreamTime)"
echo "3. Total overhead percentage"
echo "4. Object sizes and alignment"
echo "5. NPS (nodes per second) - compare to baseline"