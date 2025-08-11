#!/bin/bash

cd /workspace

echo "Test 1: Single game (no concurrency)"
/workspace/external/testers/fast-chess/fastchess \
    -engine name=SeaJay cmd=./build/seajay \
    -engine name=Stockfish cmd=./external/engines/stockfish/stockfish option."Skill Level"=0 \
    -each proto=uci tc=10+0.1 \
    -rounds 2 \
    -openings file=./external/books/4moves_test.pgn format=pgn \
    -pgnout /tmp/test_no_concurrency.pgn 2>&1 | grep -E "(Fatal|Started|Finished|Score)" | head -10

echo ""
echo "Test 2: With concurrency=2"
/workspace/external/testers/fast-chess/fastchess \
    -engine name=SeaJay cmd=./build/seajay \
    -engine name=Stockfish cmd=./external/engines/stockfish/stockfish option."Skill Level"=0 \
    -each proto=uci tc=10+0.1 \
    -rounds 2 \
    -concurrency 2 \
    -openings file=./external/books/4moves_test.pgn format=pgn \
    -pgnout /tmp/test_concurrency_2.pgn 2>&1 | grep -E "(Fatal|Started|Finished|Score)" | head -10

echo ""
echo "Test 3: With concurrency=4"
/workspace/external/testers/fast-chess/fastchess \
    -engine name=SeaJay cmd=./build/seajay \
    -engine name=Stockfish cmd=./external/engines/stockfish/stockfish option."Skill Level"=0 \
    -each proto=uci tc=10+0.1 \
    -rounds 2 \
    -concurrency 4 \
    -openings file=./external/books/4moves_test.pgn format=pgn \
    -pgnout /tmp/test_concurrency_4.pgn 2>&1 | grep -E "(Fatal|Started|Finished|Score)" | head -10