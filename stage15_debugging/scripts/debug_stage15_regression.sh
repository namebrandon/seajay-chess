#!/bin/bash
# Debug Stage 15 regression by testing evaluation at different commits

echo "Stage 15 Regression Debugging - Commit Bisection"
echo "================================================"
echo ""

# Test position from the opening where Stage 15 shows wrong evaluation
# Using a position from early in the game where we see the +2.90 vs +0.20 discrepancy
TEST_POSITION="position startpos moves e2e4 c7c5"

# Key commits to test
declare -a COMMITS=(
    "7ecfc35:Stage 14 FINAL (baseline)"
    "b99f10b:Stage 15 Day 1 (SEE start)"
    "4418fb5:Stage 15 SPRT Candidate 1"
    "13198cf:Stage 15 complete pre-bugfix"
    "aa269a9:PST bug fix #1"
    "d75ee06:PST bug fix #2"
    "c570c83:Parameter tuning complete"
)

# Function to test evaluation at a specific commit
test_commit() {
    local commit_hash=$1
    local commit_desc=$2
    
    echo "Testing: $commit_desc"
    echo "Commit: $commit_hash"
    
    # Checkout the commit
    git checkout $commit_hash 2>/dev/null
    
    # Clean build
    cd /workspace/build
    rm -rf *
    cmake -DCMAKE_BUILD_TYPE=Release .. >/dev/null 2>&1
    make -j4 >/dev/null 2>&1
    
    if [ -f ../build/seajay ]; then
        # Test the position
        echo -e "$TEST_POSITION\neval\nquit" | ../build/seajay 2>/dev/null | grep -E "Final evaluation:|eval:|Evaluation:" | head -1
        
        # Also test from Black's perspective
        echo -e "$TEST_POSITION\nflip\neval\nquit" | ../build/seajay 2>/dev/null | grep -E "Final evaluation:|eval:|Evaluation:" | head -1
    else
        echo "Build failed at this commit"
    fi
    
    echo "---"
}

echo "Test Position: After 1.e4 c5 (Sicilian Defense)"
echo ""

# Test each commit
for commit_info in "${COMMITS[@]}"; do
    IFS=':' read -r hash desc <<< "$commit_info"
    test_commit "$hash" "$desc"
    echo ""
done

# Return to main branch
git checkout main 2>/dev/null

echo "================================================"
echo "Analysis Complete"
echo ""
echo "Look for:"
echo "1. When evaluation jumps from ~0.20 to ~2.90"
echo "2. Color asymmetry in evaluation"
echo "3. Which commit introduced the bug"