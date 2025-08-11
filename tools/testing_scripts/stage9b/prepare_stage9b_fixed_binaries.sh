#!/bin/bash

# Stage 9b Fixed Binary Preparation Script
# Prepares binaries for testing the vector operations fix

echo "=== SeaJay Stage 9b Fixed Binary Preparation ==="
echo "Preparing binaries to test vector operations fix"
echo

# Step 1: Build Stage 9 binary (base without draw detection)
echo "Step 1: Building Stage 9 binary (base)..."

# Check if Stage 9 binary already exists
if [ -f "/workspace/bin/seajay_stage9_base" ]; then
    echo "✓ Stage 9 binary already exists"
else
    echo "Building Stage 9 binary..."
    
    # Save current changes
    git stash save "Preparing SPRT test binaries" 2>/dev/null || echo "No changes to stash"
    
    # Checkout Stage 9 completion
    git checkout fe33035  # Stage 9 completion commit
    
    # Clean build
    rm -rf /workspace/build
    cmake -B /workspace/build -DCMAKE_BUILD_TYPE=Release >/dev/null 2>&1
    
    if cmake --build /workspace/build -j4 >/dev/null 2>&1; then
        echo "✓ Stage 9 binary built successfully"
        cp /workspace/build/seajay /workspace/bin/seajay_stage9_base
        echo "✓ Saved as: /workspace/bin/seajay_stage9_base"
    else
        echo "✗ Failed to build Stage 9 binary"
        exit 1
    fi
    
    # Return to main branch
    git checkout main
    git stash pop 2>/dev/null || echo "No stash to pop"
fi

# Step 2: Build current Stage 9b fixed binary
echo
echo "Step 2: Building Stage 9b fixed binary (with vector ops fix)..."

# Clean build with current code
rm -rf /workspace/build
cmake -B /workspace/build -DCMAKE_BUILD_TYPE=Release >/dev/null 2>&1

if cmake --build /workspace/build -j4 >/dev/null 2>&1; then
    echo "✓ Stage 9b fixed binary built successfully"
    cp /workspace/build/seajay /workspace/bin/seajay_stage9b_fixed
    echo "✓ Saved as: /workspace/bin/seajay_stage9b_fixed"
else
    echo "✗ Failed to build Stage 9b fixed binary"
    exit 1
fi

# Step 3: Verify both binaries work
echo
echo "Step 3: Verifying both binaries..."

echo "Testing Stage 9 binary..."
if echo -e "position startpos\ngo depth 1\nquit" | /workspace/bin/seajay_stage9_base >/dev/null 2>&1; then
    echo "✓ Stage 9 binary responds to UCI commands"
else
    echo "✗ Stage 9 binary failed UCI test"
    exit 1
fi

echo "Testing Stage 9b fixed binary..."
if echo -e "position startpos\ngo depth 1\nquit" | /workspace/bin/seajay_stage9b_fixed >/dev/null 2>&1; then
    echo "✓ Stage 9b fixed binary responds to UCI commands"
else
    echo "✗ Stage 9b fixed binary failed UCI test"
    exit 1
fi

# Step 4: Quick performance comparison
echo
echo "Step 4: Quick performance comparison..."

echo "Stage 9 performance (perft 4):"
timeout 10s /workspace/bin/seajay_stage9_base perft 4 2>&1 | grep -E "nodes|time" | head -2

echo
echo "Stage 9b fixed performance (perft 4):"
timeout 10s /workspace/bin/seajay_stage9b_fixed perft 4 2>&1 | grep -E "nodes|time" | head -2

# Step 5: Verify draw detection still works
echo
echo "Step 5: Verifying draw detection..."

# Test insufficient material position (K vs K)
DRAW_TEST=$(echo -e "position fen 8/8/8/4k3/8/3K4/8/8 w - - 0 1\ngo depth 1\nquit" | /workspace/bin/seajay_stage9b_fixed 2>&1)

if echo "$DRAW_TEST" | grep -qi "draw\|insufficient"; then
    echo "✓ Draw detection is working in fixed binary"
else
    echo "⚠️ WARNING: Draw detection may not be working"
    echo "  This could affect test results"
fi

echo
echo "=== Binary Preparation Complete ==="
echo
echo "Binaries ready for SPRT testing:"
echo "  Base:  /workspace/bin/seajay_stage9_base"
echo "  Fixed: /workspace/bin/seajay_stage9b_fixed"
echo "  Current build: /workspace/build/seajay"
echo
echo "Next steps:"
echo "  1. Run SPRT test: ./run_stage9b_fixed_sprt.sh"
echo "  2. Monitor progress in the output directory"
echo "  3. Expected result: Recovery of ~70 Elo"
echo
echo "The fix removes vector operations from the search hot path while"
echo "maintaining full draw detection functionality."