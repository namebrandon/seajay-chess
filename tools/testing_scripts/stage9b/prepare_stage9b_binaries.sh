#!/bin/bash

# Stage 9b SPRT Binary Preparation Script
# Following SeaJay SPRT Testing Process

echo "=== SeaJay Stage 9b SPRT Binary Preparation ==="
echo

# Step 1: Save current work (already committed)
echo "✓ Current work already committed (136d4aa)"

# Step 2: Build Stage 9 binary (base without draw detection)
echo "Step 1: Building Stage 9 binary (base)..."
git stash save "Preparing SPRT test binaries" 2>/dev/null || echo "No changes to stash"

echo "Checking out Stage 9 completion commit..."
git checkout fe33035  # Stage 9 completion commit

echo "Building Stage 9 binary..."
cd /workspace/build
make clean >/dev/null 2>&1
cmake .. >/dev/null 2>&1
if make -j >/dev/null 2>&1; then
    echo "✓ Stage 9 binary built successfully"
    cp seajay ../bin/seajay_stage9_base
    echo "✓ Saved as: /workspace/bin/seajay_stage9_base"
else
    echo "✗ Failed to build Stage 9 binary"
    exit 1
fi

# Step 3: Build Stage 9b binary (test with draw detection)  
echo
echo "Step 2: Building Stage 9b binary (test)..."
cd /workspace
git checkout main  # Return to Stage 9b code
git stash pop 2>/dev/null || echo "No stash to pop"

echo "Building Stage 9b binary..."
cd /workspace/build
make clean >/dev/null 2>&1
cmake .. >/dev/null 2>&1
if make -j >/dev/null 2>&1; then
    echo "✓ Stage 9b binary built successfully"
    cp seajay ../bin/seajay_stage9b_draws
    echo "✓ Saved as: /workspace/bin/seajay_stage9b_draws"
else
    echo "✗ Failed to build Stage 9b binary" 
    exit 1
fi

# Step 4: Verify both binaries work
echo
echo "Step 3: Verifying both binaries..."

echo "Testing Stage 9 binary..."
if echo -e "position startpos\ngo depth 1\nquit" | /workspace/bin/seajay_stage9_base >/dev/null 2>&1; then
    echo "✓ Stage 9 binary responds to UCI commands"
else
    echo "✗ Stage 9 binary failed UCI test"
    exit 1
fi

echo "Testing Stage 9b binary..."
if echo -e "position startpos\ngo depth 1\nquit" | /workspace/bin/seajay_stage9b_draws >/dev/null 2>&1; then
    echo "✓ Stage 9b binary responds to UCI commands"
else
    echo "✗ Stage 9b binary failed UCI test"
    exit 1
fi

echo
echo "=== Binary Preparation Complete ==="
echo
echo "Binaries ready for SPRT testing:"
echo "  Base:  /workspace/bin/seajay_stage9_base"
echo "  Test:  /workspace/bin/seajay_stage9b_draws"
echo
echo "Next step: Run the SPRT test script"