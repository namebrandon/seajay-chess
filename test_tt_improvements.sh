#!/bin/bash

# Test TT improvements: collision detection and replacement strategy

echo "Testing TT Improvements"
echo "======================="
echo ""

# Test position
POSITION="r1b1k2r/pp4pp/3Bpp2/3p4/6q1/8/PQ3PPP/2R1R1K1 b kq - 3 17"

# Function to run a test
run_test() {
    local description="$1"
    local hash_size="$2"
    local depth="$3"
    
    echo "$description"
    echo "Hash: ${hash_size}MB, Depth: $depth"
    echo "-------------------"
    
    ./bin/seajay << EOF 2>&1 | grep -E "info depth $depth|SearchStats:" | tail -2
setoption name Hash value $hash_size
setoption name SearchStats value true
position fen $POSITION
go depth $depth
quit
EOF
    echo ""
}

# Test 1: Check collision tracking with small hash
echo "Test 1: Collision Detection (1MB hash, should see collisions)"
run_test "Small hash test" 1 8

# Test 2: Medium hash performance
echo "Test 2: Medium Hash Performance"
run_test "Medium hash test" 64 10

# Test 3: Compare hit rates at different depths
echo "Test 3: Hit Rate by Depth"
for depth in 6 8 10 12; do
    echo "Depth $depth:"
    ./bin/seajay << EOF 2>&1 | grep -E "hit%=" | sed 's/.*hit%=\([^ ]*\).*/  Hit rate: \1%/'
setoption name Hash value 128
setoption name SearchStats value true
position fen $POSITION
go depth $depth
quit
EOF
done

echo ""
echo "Analysis:"
echo "1. Check if collisions (coll=) are now being detected with small hash"
echo "2. Compare hit rates - should improve with deeper searches"
echo "3. Look for improved node counts with better replacement strategy"