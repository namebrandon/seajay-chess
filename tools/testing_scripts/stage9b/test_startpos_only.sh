#!/bin/bash

# Quick test without opening book - just from startpos
# This eliminates opening book variance

FASTCHESS="/workspace/external/testers/fast-chess/fastchess"
TEST_BIN="/workspace/build/seajay"
BASE_BIN="/workspace/bin/seajay_stage9_base"

echo "Testing Stage 9b Fixed vs Stage 9 Base"
echo "WITHOUT opening book (startpos only)"
echo

$FASTCHESS \
    -engine name="Stage9b-Fixed" cmd="$TEST_BIN" \
    -engine name="Stage9-Base" cmd="$BASE_BIN" \
    -each proto=uci tc="10+0.1" \
    -rounds 20 \
    -repeat \
    -concurrency 1 \
    -pgnout file="startpos_games.pgn" \
    2>&1 | tee startpos_test.log

echo
echo "Test complete. Check if results differ from opening book tests."
echo "Game moves saved to: startpos_games.pgn"