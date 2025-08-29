#!/bin/bash

echo "Testing UCI Mate Score Perspective"
echo "==================================="
echo ""

# Test mate positions
echo "Test 1: White to move, mate in 1 (should be POSITIVE mate)"
echo "FEN: 6k1/5ppp/8/8/8/8/5PPP/3R2K1 w - - 0 1"
(echo "position fen 6k1/5ppp/8/8/8/8/5PPP/3R2K1 w - - 0 1"; echo "go depth 5"; sleep 1) | ./build/seajay 2>&1 | grep "score mate" | head -1
echo ""

echo "Test 2: Black to move, mate in 1 (should be POSITIVE mate)"
echo "FEN: 3r2k1/5ppp/8/8/8/8/5PPP/6K1 b - - 0 1"
(echo "position fen 3r2k1/5ppp/8/8/8/8/5PPP/6K1 b - - 0 1"; echo "go depth 5"; sleep 1) | ./build/seajay 2>&1 | grep "score mate" | head -1
echo ""

echo "Test 3: White to move, getting mated in 1 (should be NEGATIVE mate)"
echo "FEN: 3r2k1/5ppp/8/8/8/8/5PPP/6K1 w - - 0 1"
(echo "position fen 3r2k1/5ppp/8/8/8/8/5PPP/6K1 w - - 0 1"; echo "go depth 5"; sleep 1) | ./build/seajay 2>&1 | grep "score mate" | head -1
echo ""

echo "Test 4: Black to move, getting mated in 1 (should be NEGATIVE mate)"
echo "FEN: 6k1/5ppp/8/8/8/8/5PPP/3R2K1 b - - 0 1"
(echo "position fen 6k1/5ppp/8/8/8/8/5PPP/3R2K1 b - - 0 1"; echo "go depth 5"; sleep 1) | ./build/seajay 2>&1 | grep "score mate" | head -1
echo ""

echo "EXPECTED RESULTS (UCI side-to-move perspective):"
echo "Test 1 (White mates): POSITIVE mate"
echo "Test 2 (Black mates): POSITIVE mate"
echo "Test 3 (White mated): NEGATIVE mate"
echo "Test 4 (Black mated): NEGATIVE mate"