#!/bin/bash

echo "Reconstructing the game position from PGN..."
echo ""
echo "Starting position: rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"
echo ""
echo "Moves from game 2:"
echo "1.Nc3 e5 2.e4 Nc6 3.Nf3 Be7 4.Bb5 d6 5.O-O Nf6 6.d3 O-O 7.Bc4 Bg4 8.h3 Bxf3"
echo "9.Qxf3 Nd4 10.Qg3 Nxc2"
echo ""
echo "Testing position after 10...Nxc2 (Black knight forks Queen on g3 and Rook on a1):"
echo ""

# Use SeaJay to set up the position move by move
echo -e "position startpos moves b1c3 e7e5 e2e4 b8c6 g1f3 f8e7 f1b5 d7d6 e1g1 g8f6 d2d3 e8g8 b5c4 c8g4 h2h3 g4f3 d1f3 c6d4 f3g3 d4c2\nd\nquit" | ./bin/seajay

