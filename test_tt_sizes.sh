#!/bin/bash

# Test TT performance with different hash sizes
# This will help identify if TT size is causing the low hit rate

POSITION="r1b1k2r/pp4pp/3Bpp2/3p4/6q1/8/PQ3PPP/2R1R1K1 b kq - 3 17"
DEPTH=10

echo "Testing TT Performance with Different Hash Sizes"
echo "================================================"
echo "Position: $POSITION"
echo "Depth: $DEPTH"
echo ""

# Build the engine first
echo "Building SeaJay..."
rm -rf build && ./build.sh Release > /dev/null 2>&1

# Function to test a specific hash size
test_hash_size() {
    local hash_size=$1
    echo "Testing Hash Size: ${hash_size}MB"
    echo "------------------------"
    
    # Run the test with SearchStats enabled to get TT statistics
    ./bin/seajay << EOF 2>&1 | grep -E "SearchStats:|info depth ${DEPTH}|Benchmark complete"
setoption name Hash value $hash_size
setoption name SearchStats value true
setoption name NodeExplosionDiagnostics value true
position fen $POSITION
go depth $DEPTH
quit
EOF
    
    echo ""
}

# Test with different hash sizes
test_hash_size 8     # Default
test_hash_size 32
test_hash_size 64
test_hash_size 128
test_hash_size 256
test_hash_size 512
test_hash_size 1024

echo "================================================"
echo "Analysis Complete"
echo ""
echo "Look for:"
echo "1. TT hit rate (hit%) - should improve with larger hash"
echo "2. TT cutoffs - should increase with better hit rate"
echo "3. Total nodes - should decrease with better TT performance"
echo "4. TT collisions (coll) - should decrease with larger hash"