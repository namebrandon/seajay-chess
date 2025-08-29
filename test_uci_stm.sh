#!/bin/bash

# Test script for UCI side-to-move perspective scoring
# According to UCI protocol, scores should ALWAYS be from the side-to-move perspective

echo "Testing UCI Side-to-Move Perspective Scoring"
echo "============================================"
echo ""

# Test Position 1: Black to move, Black is winning (should be POSITIVE)
echo "Test 1: Black to move, Black is winning (should show POSITIVE score)"
echo "FEN: rqq2k1r/1ppbpp1p/3b4/8/8/3P4/1PPBBPPP/2R1R1K1 b - - 5 17"
echo -e "position fen rqq2k1r/1ppbpp1p/3b4/8/8/3P4/1PPBBPPP/2R1R1K1 b - - 5 17\ngo depth 10\nquit" | ./seajay 2>/dev/null | grep "score cp" | tail -1
echo ""

# Test Position 2: Black to move, Black is losing (should be NEGATIVE)  
echo "Test 2: Black to move, Black is losing (should show NEGATIVE score)"
echo "FEN: 8/8/4k3/8/4P3/4K3/8/8 b - - 0 1"
echo -e "position fen 8/8/4k3/8/4P3/4K3/8/8 b - - 0 1\ngo depth 10\nquit" | ./seajay 2>/dev/null | grep "score cp" | tail -1
echo ""

# Test Position 3: White to move, White is winning (should be POSITIVE)
echo "Test 3: White to move, White is winning (should show POSITIVE score)"
echo "FEN: 2R1R1K1/1PPBBPPP/3P4/8/8/3b4/1ppbpp1p/rqq2k1r w - - 5 17"
echo -e "position fen 2R1R1K1/1PPBBPPP/3P4/8/8/3b4/1ppbpp1p/rqq2k1r w - - 5 17\ngo depth 10\nquit" | ./seajay 2>/dev/null | grep "score cp" | tail -1
echo ""

# Test Position 4: White to move, White is losing (should be NEGATIVE)
echo "Test 4: White to move, White is losing (should show NEGATIVE score)"
echo "FEN: 8/8/4K3/8/4p3/4k3/8/8 w - - 0 1"
echo -e "position fen 8/8/4K3/8/4p3/4k3/8/8 w - - 0 1\ngo depth 10\nquit" | ./seajay 2>/dev/null | grep "score cp" | tail -1
echo ""

echo "============================================"
echo "Expected results:"
echo "Test 1 (Black winning, Black to move): POSITIVE score"
echo "Test 2 (Black losing, Black to move): NEGATIVE score"
echo "Test 3 (White winning, White to move): POSITIVE score"
echo "Test 4 (White losing, White to move): NEGATIVE score"