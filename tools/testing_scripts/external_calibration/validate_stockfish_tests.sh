#!/bin/bash

echo "Validating Stockfish SPRT Test Setup"
echo "====================================="
echo ""

cd /workspace

# Check SeaJay
echo "1. Testing SeaJay..."
if echo "uci" | timeout 2 ./build/seajay 2>&1 | grep -q "uciok"; then
    VERSION=$(echo "uci" | ./build/seajay 2>&1 | grep "id name" | head -1)
    echo "   ✓ SeaJay UCI: OK"
    echo "   ✓ Version: $VERSION"
else
    echo "   ✗ SeaJay UCI: FAILED"
    exit 1
fi

# Check Stockfish
echo ""
echo "2. Testing Stockfish..."
if echo "uci" | timeout 2 ./external/engines/stockfish/stockfish 2>&1 | grep -q "uciok"; then
    VERSION=$(echo "uci" | ./external/engines/stockfish/stockfish 2>&1 | grep "id name" | head -1)
    echo "   ✓ Stockfish UCI: OK"
    echo "   ✓ Version: $VERSION"
else
    echo "   ✗ Stockfish UCI: FAILED"
    exit 1
fi

# Check Stockfish options
echo ""
echo "3. Checking Stockfish strength options..."
echo "uci" | ./external/engines/stockfish/stockfish 2>&1 | grep -E "(Skill Level|UCI_LimitStrength|UCI_Elo)" | while read line; do
    echo "   - $line"
done

# Test with Skill Level 0 (800 ELO)
echo ""
echo "4. Testing Stockfish at Skill Level 0 (~800 ELO)..."
/workspace/external/testers/fast-chess/fastchess \
    -engine name="SeaJay" cmd="./build/seajay" \
    -engine name="Stockfish-800" cmd="./external/engines/stockfish/stockfish" option."Skill Level"=0 option.Threads=1 option.Hash=16 \
    -each proto=uci tc="1+0.01" \
    -rounds 1 \
    -openings file="./external/books/4moves_test.pgn" format=pgn order=random \
    -pgnout "/tmp/test_sf800.pgn" 2>&1 | grep -E "(Fatal|Score)" | head -3

if [ $? -eq 0 ]; then
    echo "   ✓ Stockfish 800 ELO test: OK"
else
    echo "   ✗ Stockfish 800 ELO test: FAILED"
fi

# Test with Skill Level 5 (1200 ELO)
echo ""
echo "5. Testing Stockfish at Skill Level 5 (~1200 ELO)..."
/workspace/external/testers/fast-chess/fastchess \
    -engine name="SeaJay" cmd="./build/seajay" \
    -engine name="Stockfish-1200" cmd="./external/engines/stockfish/stockfish" option."Skill Level"=5 option.Threads=1 option.Hash=16 \
    -each proto=uci tc="1+0.01" \
    -rounds 1 \
    -openings file="./external/books/4moves_test.pgn" format=pgn order=random \
    -pgnout "/tmp/test_sf1200.pgn" 2>&1 | grep -E "(Fatal|Score)" | head -3

if [ $? -eq 0 ]; then
    echo "   ✓ Stockfish 1200 ELO test: OK"
else
    echo "   ✗ Stockfish 1200 ELO test: FAILED"
fi

echo ""
echo "====================================="
echo "Validation complete!"
echo ""
echo "Available SPRT tests:"
echo "1. SeaJay vs Stockfish 800 ELO:"
echo "   ./run_seajay_vs_stockfish_800_sprt.sh"
echo ""
echo "2. SeaJay vs Stockfish 1200 ELO:"
echo "   ./run_seajay_vs_stockfish_1200_sprt.sh"
echo ""
echo "Expected outcomes:"
echo "- vs 800 ELO: SeaJay (~650) should score 20-30%"
echo "- vs 1200 ELO: SeaJay (~650) should score 2-8%"
echo "====================================="