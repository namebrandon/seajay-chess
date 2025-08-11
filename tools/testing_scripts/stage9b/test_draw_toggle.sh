#!/bin/bash

# Script to test different draw detection configurations

echo "=== Draw Detection Toggle Testing ==="
echo
echo "This script will build and test different configurations:"
echo "1. Current Stage 9b with draw detection ENABLED (optimized)"
echo "2. Current Stage 9b with draw detection DISABLED" 
echo "3. Original Stage 9 (baseline before any changes)"
echo

# Configuration
TEST_DEPTH=6
TEST_POSITIONS=100

# Step 1: Build current Stage 9b with draw detection ENABLED
echo "Building Stage 9b with draw detection ENABLED..."
cd /workspace/build
make clean >/dev/null 2>&1
cmake .. >/dev/null 2>&1
make -j seajay >/dev/null 2>&1
cp seajay ../bin/seajay_stage9b_draw_enabled
echo "✓ Built: /workspace/bin/seajay_stage9b_draw_enabled"

# Step 2: Build Stage 9b with draw detection DISABLED
echo "Building Stage 9b with draw detection DISABLED..."
# Temporarily comment out the draw detection check
sed -i.bak 's/if (shouldCheckDraw && board.isDrawInSearch/if (false \&\& board.isDrawInSearch/' /workspace/src/search/negamax.cpp

make clean >/dev/null 2>&1
cmake .. >/dev/null 2>&1
make -j seajay >/dev/null 2>&1
cp seajay ../bin/seajay_stage9b_draw_disabled
echo "✓ Built: /workspace/bin/seajay_stage9b_draw_disabled"

# Restore the original file
mv /workspace/src/search/negamax.cpp.bak /workspace/src/search/negamax.cpp

# Step 3: Build original Stage 9 from git
echo "Building original Stage 9 (baseline)..."
cd /workspace
git stash >/dev/null 2>&1
git checkout fe33035 >/dev/null 2>&1  # Stage 9 completion commit

cd /workspace/build
make clean >/dev/null 2>&1
cmake .. >/dev/null 2>&1
make -j seajay >/dev/null 2>&1
cp seajay ../bin/seajay_stage9_original
echo "✓ Built: /workspace/bin/seajay_stage9_original"

# Return to main branch
cd /workspace
git checkout main >/dev/null 2>&1
git stash pop >/dev/null 2>&1

echo
echo "=== Running Performance Benchmarks ==="
echo

# Benchmark each version
echo "1. Stage 9 Original (baseline):"
/workspace/bin/seajay_stage9_original bench $TEST_DEPTH 2>&1 | grep -E "(nodes|nps)" | tail -1

echo
echo "2. Stage 9b with draw detection DISABLED:"
/workspace/bin/seajay_stage9b_draw_disabled bench $TEST_DEPTH 2>&1 | grep -E "(nodes|nps)" | tail -1

echo
echo "3. Stage 9b with draw detection ENABLED:"
/workspace/bin/seajay_stage9b_draw_enabled bench $TEST_DEPTH 2>&1 | grep -E "(nodes|nps)" | tail -1

echo
echo "=== Quick Match Test ==="
echo "Running 20 games: Stage 9b (draw disabled) vs Stage 9 original..."
echo

/workspace/external/testers/fast-chess/fastchess \
    -engine name="Stage9b-NoDraws" cmd="/workspace/bin/seajay_stage9b_draw_disabled" \
    -engine name="Stage9-Original" cmd="/workspace/bin/seajay_stage9_original" \
    -each proto=uci tc=5+0.05 \
    -rounds 10 \
    -repeat \
    -concurrency 1 2>&1 | grep -E "(Results|Elo:|Games:)" | tail -3

echo
echo "=== Test Complete ==="
echo
echo "Key questions to answer:"
echo "1. Is NPS similar between Stage9-Original and Stage9b-NoDraws?"
echo "2. Does Stage9b-NoDraws perform similarly to Stage9-Original in games?"
echo "3. How much NPS drop for Stage9b with draws enabled?"
echo
echo "If Stage9b-NoDraws performs poorly vs Stage9-Original, then we have"
echo "unintended changes beyond draw detection affecting performance."