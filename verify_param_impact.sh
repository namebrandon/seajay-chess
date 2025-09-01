#!/bin/bash

echo "=== Verifying Parameter Impact on Endgame Evaluation ==="
echo

# Pure pawn endgame - should be highly sensitive to pawn_eg values
PAWN_EG="8/4P3/4K3/8/8/8/4k3/8 w - - 0 1"

echo "Pawn on 7th rank endgame position:"
echo "$PAWN_EG"
echo

echo "Evaluation with pawn_eg_r7_center = 90 (default):"
echo -e "position fen $PAWN_EG\neval\nquit" | ./bin/seajay 2>&1 | grep "Total:" | tail -1

echo "Evaluation with pawn_eg_r7_center = 50 (minimum):"
echo -e "setoption name pawn_eg_r7_center value 50\nposition fen $PAWN_EG\neval\nquit" | ./bin/seajay 2>&1 | grep "Total:" | tail -1

echo "Evaluation with pawn_eg_r7_center = 150 (maximum):"
echo -e "setoption name pawn_eg_r7_center value 150\nposition fen $PAWN_EG\neval\nquit" | ./bin/seajay 2>&1 | grep "Total:" | tail -1

# Rook endgame
ROOK_EG="8/r7/8/8/8/8/8/R3K2k w - - 0 1"

echo
echo "Rook on 7th rank endgame position:"
echo "$ROOK_EG"
echo

echo "Evaluation with rook_eg_7th = 25 (default):"
echo -e "position fen $ROOK_EG\neval\nquit" | ./bin/seajay 2>&1 | grep "Total:" | tail -1

echo "Evaluation with rook_eg_7th = 40 (maximum):"
echo -e "setoption name rook_eg_7th value 40\nposition fen $ROOK_EG\neval\nquit" | ./bin/seajay 2>&1 | grep "Total:" | tail -1

echo
echo "=== Analysis ==="
echo "If evaluations don't change with parameter changes, there's an implementation issue."
echo "If they do change but SPSA isn't moving parameters, consider:"
echo "1. Reducing C_end values (try 2-3 instead of 6)"
echo "2. Increasing R_end (try 0.003 or 0.004)"
echo "3. The defaults might already be near-optimal"