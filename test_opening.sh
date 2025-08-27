#!/bin/bash

echo "====================================="
echo "Testing Opening Position Move Ordering"
echo "====================================="
echo

# Test from startpos
echo "Position: Starting position"
echo "Moves after 1.e4 e5 2.Nf3 Nc6 3.Bc4 Nf6 4.Ng5 d5 5.exd5"
echo

# Position after common opening moves where captures become available
FEN="r1bqkb1r/ppp2ppp/2n2n2/3Pp1N1/2B5/8/PPPP1PPP/RNBQK2R b KQkq - 0 5"
echo "FEN: $FEN"
echo "Black to move - can recapture on d5"
echo

echo -e "position fen $FEN\ngo depth 8\nquit" | ./bin/seajay 2>&1 | grep -E "info depth|moveeff"

echo
echo "====================================="
echo "Checking if any QxP/RxP exist early"
echo "====================================="
echo

# Let's check a typical Sicilian position
FEN2="r1bqkbnr/pp1ppppp/2n5/2p5/4P3/5N2/PPPP1PPP/RNBQKB1R w KQkq - 2 3"
echo "Sicilian Defense position"
echo "FEN: $FEN2"
echo

echo -e "position fen $FEN2\ngo depth 8\nquit" | ./bin/seajay 2>&1 | grep -E "info depth|moveeff"

echo
echo "====================================="
echo "Testing pure startpos"
echo "====================================="
echo

echo -e "position startpos\ngo depth 10\nquit" | ./bin/seajay 2>&1 | grep -E "info depth|moveeff"