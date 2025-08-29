#!/bin/bash

echo "Testing Mate Score with Debug Output"
echo "===================================="
echo ""

# Simple mate in 1 for Black
echo "Black to move, mate in 1:"
echo "FEN: 3r2k1/5ppp/8/8/8/8/5PPP/6K1 b - - 0 1"
echo ""
echo "Running search..."
(echo "position fen 3r2k1/5ppp/8/8/8/8/5PPP/6K1 b - - 0 1"; echo "go depth 3"; sleep 1; echo "quit") | ./build/seajay 2>&1 | grep -E "info|bestmove|Root:|score"
echo ""

# Simple mate in 1 for White  
echo "White to move, mate in 1:"
echo "FEN: 6k1/5ppp/8/8/8/8/5PPP/3R2K1 w - - 0 1"
echo ""
echo "Running search..."
(echo "position fen 6k1/5ppp/8/8/8/8/5PPP/3R2K1 w - - 0 1"; echo "go depth 3"; sleep 1; echo "quit") | ./build/seajay 2>&1 | grep -E "info|bestmove|Root:|score"