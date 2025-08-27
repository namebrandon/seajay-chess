#!/bin/bash

# Test move ordering improvement - Phase MO1
# This script tests if killer moves are now searched before low-value captures

echo "====================================="
echo "Move Ordering Validation Test"
echo "Testing Phase MO1 improvements"
echo "====================================="
echo

# Test position 1: From actual game - position with captures available
FEN1="r3kb1r/ppp2ppp/2nq1n2/1B1ppb2/6P1/P1N1PN1P/1PPP1P2/R1BQK2R b KQkq - 0 1"

echo "Test 1: Position with multiple capture types"
echo "FEN: $FEN1"
echo
echo "Getting move list with debug info (depth 1):"
echo

# Use go perft 1 to see move ordering
echo -e "position fen $FEN1\ngo perft 1\nquit" | ./bin/seajay | head -20

echo
echo "-----------------------------------"
echo

# Test position 2: Position likely to have QxP situations
FEN2="r1bqkb1r/pppp1ppp/2n2n2/4p3/2B1P3/5N2/PPPP1PPP/RNBQK2R w KQkq - 4 4"

echo "Test 2: Position after 1.e4 e5 2.Nf3 Nc6 3.Bc4 Nf6"
echo "FEN: $FEN2"
echo
echo "Searching to depth 8 to see move ordering effects:"
echo

# Do a short search and look at the info output
echo -e "position fen $FEN2\ngo depth 8\nquit" | ./bin/seajay | grep "info depth"

echo
echo "-----------------------------------"
echo

# Test position 3: Tactical position with bad captures
FEN3="r1bqkbnr/pppp1ppp/2n5/4p3/2B1P3/5Q2/PPPP1PPP/RNB1K1NR b KQkq - 0 4"

echo "Test 3: Position with Queen attacking f7 (Scholar's mate threat)"
echo "FEN: $FEN3"
echo "Black to move - should find defensive moves before bad captures"
echo
echo "Analyzing for 2 seconds:"
echo

echo -e "position fen $FEN3\ngo movetime 2000\nquit" | ./bin/seajay | grep -E "info depth|bestmove"

echo
echo "====================================="
echo "Test complete!"
echo
echo "What to look for:"
echo "1. Killer moves should appear before low-value captures"
echo "2. High-value captures (Q/R captures) should still be first"
echo "3. Search should reach deeper with better ordering"
echo "====================================="