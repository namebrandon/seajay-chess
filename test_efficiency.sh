#!/bin/bash

echo "====================================="
echo "Move Ordering Efficiency Test"
echo "Testing bad capture penalty approach"
echo "====================================="
echo

# Test various positions to see move ordering efficiency

# Position 1: Tactical position (Kiwipete)
FEN1="r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1"
echo "Test 1: Kiwipete (very tactical)"
echo "FEN: $FEN1"
echo
echo -e "position fen $FEN1\ngo depth 7\nquit" | ./bin/seajay 2>&1 | grep -E "moveeff|tthits|info depth" | tail -10
echo

# Position 2: Middlegame with many captures available
FEN2="r1bqkb1r/pppp1ppp/2n2n2/4p3/2B1P3/5N2/PPPP1PPP/RNBQK2R w KQkq - 4 4"
echo "Test 2: Italian Game position"
echo "FEN: $FEN2"
echo
echo -e "position fen $FEN2\ngo depth 8\nquit" | ./bin/seajay 2>&1 | grep -E "moveeff|tthits|info depth" | tail -8
echo

# Position 3: Position with bad captures (queen can take defended pawns)
FEN3="r1bqkbnr/pppp1ppp/2n5/4p3/2B1P3/5Q2/PPPP1PPP/RNB1K1NR b KQkq - 0 4"
echo "Test 3: Scholar's mate threat (Queen on f3)"
echo "FEN: $FEN3"
echo
echo -e "position fen $FEN3\ngo depth 8\nquit" | ./bin/seajay 2>&1 | grep -E "moveeff|tthits|info depth" | tail -8
echo

# Position 4: Endgame position
FEN4="8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1"
echo "Test 4: Rook endgame"
echo "FEN: $FEN4"
echo
echo -e "position fen $FEN4\ngo depth 10\nquit" | ./bin/seajay 2>&1 | grep -E "moveeff|tthits|info depth" | tail -10
echo

echo "====================================="
echo "Summary of Move Ordering Efficiency:"
echo "Target: 85%+ (was 76.8% originally)"
echo "====================================="

# Extract best efficiency from each test
echo
echo "Peak efficiency achieved in each position:"
echo -e "position fen $FEN1\ngo depth 7\nquit" | ./bin/seajay 2>&1 | grep "moveeff" | sed 's/.*moveeff //' | sed 's/%.*/%/' | sort -t% -k1 -rn | head -1 | sed 's/^/Kiwipete: /'
echo -e "position fen $FEN2\ngo depth 8\nquit" | ./bin/seajay 2>&1 | grep "moveeff" | sed 's/.*moveeff //' | sed 's/%.*/%/' | sort -t% -k1 -rn | head -1 | sed 's/^/Italian: /'
echo -e "position fen $FEN3\ngo depth 8\nquit" | ./bin/seajay 2>&1 | grep "moveeff" | sed 's/.*moveeff //' | sed 's/%.*/%/' | sort -t% -k1 -rn | head -1 | sed 's/^/Scholar: /'
echo -e "position fen $FEN4\ngo depth 10\nquit" | ./bin/seajay 2>&1 | grep "moveeff" | sed 's/.*moveeff //' | sed 's/%.*/%/' | sort -t% -k1 -rn | head -1 | sed 's/^/Endgame: /'