#!/bin/bash

echo "Testing UCI piece value controls..."
echo ""

# Test 1: Default values
echo "TEST 1: Check default piece values"
echo -e "uci\nposition fen r1bqkbnr/pppp1ppp/2n5/4p3/4P3/5N2/PPPP1PPP/RNBQKB1R w KQkq - 4 4\neval\nquit" | ./bin/seajay | grep -A 10 "Material"

echo ""
echo "=========================================="
echo ""

# Test 2: Change pawn value to 150
echo "TEST 2: Change Pawn value from 100 to 150"
echo -e "uci\nsetoption name PawnValueMg value 150\nposition fen r1bqkbnr/pppp1ppp/2n5/4p3/4P3/5N2/PPPP1PPP/RNBQKB1R w KQkq - 4 4\neval\nquit" | ./bin/seajay | grep -A 10 "Material"

echo ""
echo "=========================================="
echo ""

# Test 3: Change multiple piece values
echo "TEST 3: Change multiple piece values"
echo "Setting: Pawn=80, Knight=300, Bishop=350, Rook=550, Queen=1000"
echo -e "uci\nsetoption name PawnValueMg value 80\nsetoption name KnightValueMg value 300\nsetoption name BishopValueMg value 350\nsetoption name RookValueMg value 550\nsetoption name QueenValueMg value 1000\nposition fen r1bqkbnr/pppp1ppp/2n5/4p3/4P3/5N2/PPPP1PPP/RNBQKB1R w KQkq - 4 4\neval\nquit" | ./bin/seajay | grep -A 10 "Material"

echo ""
echo "=========================================="
echo ""

# Test 4: Verify bench calculation uses updated values
echo "TEST 4: Verify bench command uses updated values"
echo -e "uci\nsetoption name PawnValueMg value 200\nbench\nquit" | ./bin/seajay | grep "Benchmark complete"