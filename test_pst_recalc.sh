#!/bin/bash

echo "=== Testing PST Recalculation Fix ==="
echo

# Endgame position where PST matters
ENDGAME_FEN="8/4P3/4K3/8/8/8/4k3/8 w - - 0 1"

echo "1. Start engine, set position, then eval (default PST):"
echo -e "uci\nposition fen $ENDGAME_FEN\neval\nquit" | ./bin/seajay 2>/dev/null | grep "Total:" | tail -1

echo
echo "2. Start engine, set position, THEN change PST, then eval:"
echo -e "uci\nposition fen $ENDGAME_FEN\nsetoption name pawn_eg_r7_center value 150\neval\nquit" | ./bin/seajay 2>/dev/null | grep "Total:" | tail -1

echo
echo "3. Start engine, change PST FIRST, then set position, then eval:"
echo -e "uci\nsetoption name pawn_eg_r7_center value 150\nposition fen $ENDGAME_FEN\neval\nquit" | ./bin/seajay 2>/dev/null | grep "Total:" | tail -1

echo
echo "=== Test Results ==="
echo "Test 2 and 3 should show different values from Test 1"
echo "Test 2 tests that recalculatePSTScore() works after setoption"
echo "Test 3 simulates OpenBench (options before position)"

# Test rank mirroring fix
echo
echo "=== Testing Rank Mirroring Fix ==="
ROOK_FEN="8/r7/8/8/8/8/R7/4K2k w - - 0 1"

echo "Default rook values:"
echo -e "dumpPST\nquit" | ./bin/seajay 2>&1 | grep -A 9 "=== Rook ===" | tail -8

echo
echo "After setting rook_eg_7th to 40 (should only affect rank 7, not rank 2):"
echo -e "setoption name rook_eg_7th value 40\ndumpPST\nquit" | ./bin/seajay 2>&1 | grep -A 9 "=== Rook ===" | tail -8