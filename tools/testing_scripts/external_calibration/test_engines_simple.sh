#!/bin/bash

echo "Testing engines for UCI compliance..."
echo ""

# Change to workspace directory
cd /workspace

# Test SeaJay
echo "1. Testing SeaJay..."
if echo "uci" | timeout 2 ./build/seajay 2>&1 | grep -q "uciok"; then
    echo "   ✓ SeaJay responds to UCI"
else
    echo "   ✗ SeaJay failed UCI test"
fi

# Test Shallow Blue  
echo "2. Testing Shallow Blue..."
if echo "uci" | timeout 2 ./external/engines/shallow-blue-2.0.0/shallowblue 2>&1 | grep -q "uciok"; then
    echo "   ✓ Shallow Blue responds to UCI"
else
    echo "   ✗ Shallow Blue failed UCI test"
fi

echo ""
echo "3. Running minimal fast-chess test..."
/workspace/external/testers/fast-chess/fastchess \
    -engine name="SeaJay" cmd="./build/seajay" \
    -engine name="ShallowBlue" cmd="./external/engines/shallow-blue-2.0.0/shallowblue" \
    -each proto=uci tc="1+0.01" \
    -rounds 1 \
    -openings file="./external/books/4moves_test.pgn" format=pgn order=random \
    -pgnout "/tmp/simple_test.pgn" 2>&1 | grep -E "(Started|Finished|Fatal)"

echo ""
echo "Test complete."