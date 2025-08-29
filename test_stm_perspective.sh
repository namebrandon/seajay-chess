#!/bin/bash

echo "Testing UCI Side-to-Move Perspective"
echo "===================================="
echo ""

# Test simple pawn endgame positions first
echo "Test 1: Simple Pawn Endgame - White to move, White winning"
echo "FEN: 8/8/4K3/8/4P3/4k3/8/8 w - - 0 1"
(echo "position fen 8/8/4K3/8/4P3/4k3/8/8 w - - 0 1"; echo "go depth 8"; sleep 1) | ./build/seajay 2>&1 | grep "info depth" | tail -1
echo ""

echo "Test 2: Simple Pawn Endgame - Black to move, Black losing"
echo "FEN: 8/8/4k3/8/4P3/4K3/8/8 b - - 0 1"
(echo "position fen 8/8/4k3/8/4P3/4K3/8/8 b - - 0 1"; echo "go depth 8"; sleep 1) | ./build/seajay 2>&1 | grep "info depth" | tail -1
echo ""

echo "Test 3: Simple Pawn Endgame - White to move, White losing"
echo "FEN: 8/8/4K3/8/4p3/4k3/8/8 w - - 0 1"
(echo "position fen 8/8/4K3/8/4p3/4k3/8/8 w - - 0 1"; echo "go depth 8"; sleep 1) | ./build/seajay 2>&1 | grep "info depth" | tail -1
echo ""

echo "Test 4: Simple Pawn Endgame - Black to move, Black winning"
echo "FEN: 8/8/4k3/8/4p3/4K3/8/8 b - - 0 1"
(echo "position fen 8/8/4k3/8/4p3/4K3/8/8 b - - 0 1"; echo "go depth 8"; sleep 1) | ./build/seajay 2>&1 | grep "info depth" | tail -1
echo ""

echo "EXPECTED RESULTS:"
echo "Test 1 (White to move, White winning): POSITIVE score"
echo "Test 2 (Black to move, Black losing):  NEGATIVE score"
echo "Test 3 (White to move, White losing):  NEGATIVE score"
echo "Test 4 (Black to move, Black winning): POSITIVE score"