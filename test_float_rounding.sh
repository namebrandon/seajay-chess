#!/bin/bash

echo "=== Testing Float Rounding in SPSA Parameters ==="
echo

# Test that fractional values are properly rounded
echo "1. Testing pawn_eg_r7_center with 90.4 (should round to 90):"
echo -e "uci\nsetoption name pawn_eg_r7_center value 90.4\ndumpPST\nquit" | ./bin/seajay 2>&1 | grep "^7 |" | head -1

echo
echo "2. Testing pawn_eg_r7_center with 90.6 (should round to 91):"
echo -e "uci\nsetoption name pawn_eg_r7_center value 90.6\ndumpPST\nquit" | ./bin/seajay 2>&1 | grep "^7 |" | head -1

echo
echo "3. Testing rook_eg_7th with 25.5 (should round to 26):"
echo -e "uci\nsetoption name rook_eg_7th value 25.5\ndumpPST\nquit" | ./bin/seajay 2>&1 | grep -A 3 "=== Rook ===" | grep "^7 |"

echo
echo "4. Testing negative rounding: pawn_eg_r7_center with 89.5 (should round to 90):"
echo -e "uci\nsetoption name pawn_eg_r7_center value 89.5\ndumpPST\nquit" | ./bin/seajay 2>&1 | grep "^7 |" | head -1

echo
echo "=== Evaluation Impact Test ==="
ENDGAME_FEN="8/4P3/4K3/8/8/8/4k3/8 w - - 0 1"

echo "5. Evaluation with pawn_eg_r7_center = 90 (default):"
echo -e "uci\nposition fen $ENDGAME_FEN\neval\nquit" | ./bin/seajay 2>&1 | grep "Total:" | tail -1

echo "6. Evaluation with pawn_eg_r7_center = 90.6 (rounds to 91):"
echo -e "uci\nsetoption name pawn_eg_r7_center value 90.6\nposition fen $ENDGAME_FEN\neval\nquit" | ./bin/seajay 2>&1 | grep "Total:" | tail -1

echo "7. Evaluation with pawn_eg_r7_center = 150:"
echo -e "uci\nsetoption name pawn_eg_r7_center value 150\nposition fen $ENDGAME_FEN\neval\nquit" | ./bin/seajay 2>&1 | grep "Total:" | tail -1

echo
echo "=== Results ==="
echo "✓ Float values should now round correctly (90.6 → 91, not 90)"
echo "✓ This allows SPSA to explore values above 90"
echo "✓ Evaluation should change with different parameter values"