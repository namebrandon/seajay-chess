#!/bin/bash

cd /workspace

echo "Test 1: Basic test without options"
/workspace/external/testers/fast-chess/fastchess \
    -engine name=SeaJay cmd=./build/seajay \
    -engine name=Stockfish cmd=./external/engines/stockfish/stockfish \
    -each proto=uci tc=1+0.01 \
    -rounds 1 \
    -pgnout /tmp/test1.pgn 2>&1 | grep -E "(Fatal|Started|Finished)" | head -5

echo ""
echo "Test 2: With Skill Level option"
/workspace/external/testers/fast-chess/fastchess \
    -engine name=SeaJay cmd=./build/seajay \
    -engine name=Stockfish cmd=./external/engines/stockfish/stockfish option."Skill Level"=0 \
    -each proto=uci tc=1+0.01 \
    -rounds 1 \
    -pgnout /tmp/test2.pgn 2>&1 | grep -E "(Fatal|Started|Finished)" | head -5

echo ""
echo "Test 3: With all options"
/workspace/external/testers/fast-chess/fastchess \
    -engine name=SeaJay cmd=./build/seajay \
    -engine name=Stockfish cmd=./external/engines/stockfish/stockfish option."Skill Level"=0 option.Threads=1 option.Hash=16 \
    -each proto=uci tc=1+0.01 \
    -rounds 1 \
    -pgnout /tmp/test3.pgn 2>&1 | grep -E "(Fatal|Started|Finished)" | head -5

echo ""
echo "Test 4: With opening book"
/workspace/external/testers/fast-chess/fastchess \
    -engine name=SeaJay cmd=./build/seajay \
    -engine name=Stockfish cmd=./external/engines/stockfish/stockfish option."Skill Level"=0 \
    -each proto=uci tc=1+0.01 \
    -rounds 1 \
    -openings file=./external/books/4moves_test.pgn format=pgn \
    -pgnout /tmp/test4.pgn 2>&1 | grep -E "(Fatal|Started|Finished)" | head -5