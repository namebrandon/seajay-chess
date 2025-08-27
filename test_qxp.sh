#!/bin/bash

echo "====================================="
echo "Testing QxP Capture Ordering"
echo "====================================="
echo

# Position where white queen can take black's defended pawns
FEN="r1bqk2r/pppp1ppp/2n2n2/2b1p3/2B1P3/3P1N2/PPP2PPP/RNBQ1RK1 w kq - 0 6"

echo "Position: White Queen on d1 can take d7 pawn (defended)"
echo "FEN: $FEN"
echo
echo "Generating moves to see ordering:"
echo

# Use go perft 1 to see move generation and analyze with depth
echo -e "position fen $FEN\ngo depth 1\nquit" | ./bin/seajay 2>&1 | grep -E "info depth|pv"

echo
echo "Now searching deeper to see efficiency:"
echo -e "position fen $FEN\ngo depth 6\nquit" | ./bin/seajay 2>&1 | grep -E "moveeff|info depth" | tail -6

echo
echo "====================================="
echo "Testing position with Queen and Rook captures available"
echo "====================================="
echo

FEN2="r3k2r/p1ppqpb1/bn2pnp1/3PN3/Qp2P3/2N2P1p/PPPB2PP/R3K2R b KQkq - 0 1"
echo "Position: White Queen on a4 can capture pawns"
echo "FEN: $FEN2"
echo

echo -e "position fen $FEN2\ngo depth 6\nquit" | ./bin/seajay 2>&1 | grep -E "moveeff|info depth" | tail -6

echo
echo "====================================="
echo "Key Observation:"
echo "- QxP and RxP should be ordered LAST"
echo "- Efficiency should remain high (85%+)"
echo "====================================="