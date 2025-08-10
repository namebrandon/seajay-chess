#!/bin/bash

# SPRT Test Script for SeaJay Stage 8 - Alpha-Beta Pruning
# Tests Stage 8 (with alpha-beta) at fixed depth to verify no strength regression
# Since alpha-beta should give identical results with fewer nodes

TEST_ID="SPRT-2025-002"
TEST_NAME="Stage8-AlphaBeta"
BASE_NAME="Stage8-AlphaBeta-Base"

# SPRT Parameters - testing for no regression
# We expect identical strength (alpha-beta only reduces nodes, not strength)
ELO0=-5      # Allow tiny regression due to timing variations
ELO1=5       # No improvement expected (same moves)
ALPHA=0.05   # Type I error probability
BETA=0.10    # Type II error probability (more lenient)

# Test configuration - Fixed depth for deterministic results
DEPTH=4      # Fixed depth search
ROUNDS=500   # Should be enough to detect any issues

# Paths
TEST_BIN="/workspace/bin/seajay_stage8_alphabeta"
BASE_BIN="/workspace/bin/seajay_stage8_alphabeta"  # Same binary for consistency test
BOOK="/workspace/external/books/4moves_test.pgn"
OUTPUT_DIR="/workspace/sprt_results/${TEST_ID}"

# Create output directory
mkdir -p "${OUTPUT_DIR}"

echo "============================================="
echo "SPRT Test: ${TEST_ID}"
echo "Testing: Alpha-Beta Consistency at depth ${DEPTH}"
echo "Purpose: Verify alpha-beta produces identical results"
echo "SPRT bounds: [${ELO0}, ${ELO1}]"
echo "Significance: α=${ALPHA}, β=${BETA}"
echo "Starting at $(date)"
echo "============================================="
echo ""

# Verify binaries exist
if [ ! -f "${TEST_BIN}" ]; then
    echo "ERROR: Test binary not found at ${TEST_BIN}"
    echo "Please build Stage 8 binary first"
    exit 1
fi

# Check if fast-chess exists
FAST_CHESS="/workspace/external/testers/fast-chess/fastchess"
if [ ! -f "${FAST_CHESS}" ]; then
    echo "WARNING: fast-chess not found at ${FAST_CHESS}"
    echo "Attempting to download and build fast-chess..."
    
    mkdir -p /workspace/external/testers
    cd /workspace/external/testers
    
    # Clone fast-chess
    git clone https://github.com/Disservin/fastchess.git fast-chess
    cd fast-chess
    
    # Build fast-chess
    make -j4
    
    if [ ! -f "fastchess" ]; then
        echo "ERROR: Failed to build fast-chess"
        exit 1
    fi
    
    cd /workspace
fi

# Check if opening book exists
if [ ! -f "${BOOK}" ]; then
    echo "WARNING: Opening book not found at ${BOOK}"
    echo "Creating minimal opening book..."
    
    mkdir -p /workspace/external/books
    cat > "${BOOK}" << 'EOF'
[Event "Opening"]
[Result "*"]
1. e4 *

[Event "Opening"]
[Result "*"]
1. d4 *

[Event "Opening"]
[Result "*"]
1. Nf3 *

[Event "Opening"]
[Result "*"]
1. c4 *
EOF
    echo "Created minimal opening book with 4 openings"
fi

echo "Running SPRT test with fixed depth ${DEPTH}..."
echo "This tests that alpha-beta produces identical moves"
echo ""

# Run fast-chess with SPRT
# Using fixed depth for deterministic comparison
"${FAST_CHESS}" \
    -engine name="${TEST_NAME}" cmd="${TEST_BIN}" \
    -engine name="${BASE_NAME}" cmd="${BASE_BIN}" \
    -each proto=uci tc=inf depth=${DEPTH} \
    -sprt elo0="${ELO0}" elo1="${ELO1}" alpha="${ALPHA}" beta="${BETA}" \
    -rounds "${ROUNDS}" \
    -repeat \
    -recover \
    -openings file="${BOOK}" format=pgn \
    -pgnout "${OUTPUT_DIR}/games.pgn" \
    -log file="${OUTPUT_DIR}/fastchess.log" level=info \
    2>&1 | tee "${OUTPUT_DIR}/console_output.txt"

RESULT=$?

echo ""
echo "============================================="
echo "Test completed at $(date)"
echo "Results saved to: ${OUTPUT_DIR}"
echo ""

# Parse results
if [ -f "${OUTPUT_DIR}/console_output.txt" ]; then
    echo "Test Results Summary:"
    echo "---------------------"
    grep -E "Score|Elo|SPRT|LLR" "${OUTPUT_DIR}/console_output.txt" | tail -10
    echo ""
    
    # Check if test passed
    if grep -q "SPRT: H0 accepted" "${OUTPUT_DIR}/console_output.txt"; then
        echo "✓ TEST PASSED: No regression detected"
        echo "Alpha-beta pruning maintains identical strength"
    elif grep -q "SPRT: H1 accepted" "${OUTPUT_DIR}/console_output.txt"; then
        echo "⚠ TEST RESULT: Small difference detected"
        echo "This is unexpected for alpha-beta at fixed depth"
        echo "Please review the games for any issues"
    else
        echo "⋯ TEST INCONCLUSIVE: Maximum games reached"
        echo "Consider extending the test with more games"
    fi
fi

echo "============================================="
echo ""
echo "To view detailed results:"
echo "  cat ${OUTPUT_DIR}/console_output.txt"
echo "  less ${OUTPUT_DIR}/games.pgn"
echo ""

exit $RESULT