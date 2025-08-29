#!/bin/bash

echo "Testing UCI Output Scoring"
echo "=========================="
echo ""

# Test 1: Black to move, Black is winning
echo "Test 1: Black to move, Black is winning (should be POSITIVE)"
echo "FEN: rqq2k1r/1ppbpp1p/3b4/8/8/3P4/1PPBBPPP/2R1R1K1 b - - 5 17"
(echo "position fen rqq2k1r/1ppbpp1p/3b4/8/8/3P4/1PPBBPPP/2R1R1K1 b - - 5 17"; echo "go depth 8"; sleep 1) | ./build/seajay 2>/dev/null | grep "info depth 8" | tail -1
echo ""

# Test 2: Black to move, Black is losing 
echo "Test 2: Black to move, Black is losing (should be NEGATIVE)"
echo "FEN: 8/8/4k3/8/4P3/4K3/8/8 b - - 0 1"
(echo "position fen 8/8/4k3/8/4P3/4K3/8/8 b - - 0 1"; echo "go depth 8"; sleep 1) | ./build/seajay 2>/dev/null | grep "info depth 8" | tail -1
echo ""

# Test 3: White to move, White is winning
echo "Test 3: White to move, White is winning (should be POSITIVE)"
echo "FEN: 2R1R1K1/1PPBBPPP/3P4/8/8/3b4/1ppbpp1p/rqq2k1r w - - 5 17" 
(echo "position fen 2R1R1K1/1PPBBPPP/3P4/8/8/3b4/1ppbpp1p/rqq2k1r w - - 5 17"; echo "go depth 8"; sleep 1) | ./build/seajay 2>/dev/null | grep "info depth 8" | tail -1
echo ""

# Test 4: White to move, White is losing
echo "Test 4: White to move, White is losing (should be NEGATIVE)"
echo "FEN: 8/8/4K3/8/4p3/4k3/8/8 w - - 0 1"
(echo "position fen 8/8/4K3/8/4p3/4k3/8/8 w - - 0 1"; echo "go depth 8"; sleep 1) | ./build/seajay 2>/dev/null | grep "info depth 8" | tail -1
echo ""