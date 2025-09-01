#!/bin/bash

# Quick test to verify SPSA parameters actually affect evaluation
# This tests a late endgame position where PST matters significantly

echo "=== SPSA Impact Test ==="
echo "Testing if PST parameters actually change evaluation..."
echo

# Endgame position: K+P endgame where pawn advancement matters
ENDGAME_FEN="8/4k3/8/8/3P4/8/8/4K3 w - - 0 1"

# Test 1: Default values
echo "Test 1: Default values"
echo -e "position fen $ENDGAME_FEN\neval\nquit" | ./bin/seajay 2>/dev/null | grep "Total:" | tail -1

# Test 2: Extreme pawn value
echo -e "\nTest 2: Setting pawn_eg_r7_center to 150 (max)"
echo -e "setoption name pawn_eg_r7_center value 150\nposition fen $ENDGAME_FEN\neval\nquit" | ./bin/seajay 2>/dev/null | grep "Total:" | tail -1

# Test 3: Minimum pawn value  
echo -e "\nTest 3: Setting pawn_eg_r7_center to 50 (min)"
echo -e "setoption name pawn_eg_r7_center value 50\nposition fen $ENDGAME_FEN\neval\nquit" | ./bin/seajay 2>/dev/null | grep "Total:" | tail -1

# Rook endgame position
ROOK_FEN="8/8/8/8/8/8/r7/R3K2k w - - 0 1"

echo -e "\n=== Rook Endgame Test ==="
echo "Test 4: Default rook_eg_7th"
echo -e "position fen $ROOK_FEN\neval\nquit" | ./bin/seajay 2>/dev/null | grep "Total:" | tail -1

echo -e "\nTest 5: Setting rook_eg_7th to 40 (max)"
echo -e "setoption name rook_eg_7th value 40\nposition fen $ROOK_FEN\neval\nquit" | ./bin/seajay 2>/dev/null | grep "Total:" | tail -1

echo -e "\n=== Results ==="
echo "If the evaluations changed between tests, SPSA parameters are working!"
echo "You should see different 'Total:' values for each parameter change."