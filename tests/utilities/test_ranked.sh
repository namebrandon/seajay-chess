#!/bin/bash
# Test with legacy ordering
echo "Testing with legacy ordering (UseRankedMovePicker=false)..."
python3 tools/depth_vs_time.py --engine ./bin/seajay --time-ms 1000 --fen "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1" --out legacy.csv

# Test with RankedMovePicker
echo "Testing with RankedMovePicker (UseRankedMovePicker=true)..."
(echo "setoption name UseRankedMovePicker value true"; sleep 0.1; echo "position startpos"; echo "go movetime 1000"; sleep 1.5; echo "quit") | ./bin/seajay 2>/dev/null | grep "info depth" | tail -1

echo "Done"
