#!/bin/bash

# SPRT Test Script for SeaJay Stage 8 - Depth Comparison
# Tests the strength improvement from searching deeper (enabled by alpha-beta pruning)
# Compares depth 3 vs depth 4 search

TEST_ID="SPRT-2025-003"
TEST_NAME="Stage8-Depth4"
BASE_NAME="Stage8-Depth3"

# SPRT Parameters - testing strength improvement from deeper search
ELO0=0       # Lower bound
ELO1=50      # Upper bound - expect significant improvement from extra ply
ALPHA=0.05   # Type I error probability
BETA=0.05    # Type II error probability

# Test configuration
ROUNDS=1000  # Maximum rounds

# Paths
ENGINE_BIN="/workspace/bin/seajay_stage8_alphabeta"
BOOK="/workspace/external/books/4moves_test.pgn"
OUTPUT_DIR="/workspace/sprt_results/${TEST_ID}"

# Create output directory
mkdir -p "${OUTPUT_DIR}"

echo "============================================="
echo "SPRT Test: ${TEST_ID}"
echo "Testing: Depth 4 vs Depth 3 (both with alpha-beta)"
echo "Purpose: Demonstrate strength gain from deeper search"
echo "SPRT bounds: [${ELO0}, ${ELO1}]"
echo "Significance: α=${ALPHA}, β=${BETA}"
echo "Starting at $(date)"
echo "============================================="
echo ""

# Verify binary exists
if [ ! -f "${ENGINE_BIN}" ]; then
    echo "ERROR: Engine binary not found at ${ENGINE_BIN}"
    echo "Please build Stage 8 binary first"
    exit 1
fi

# Check if fast-chess exists
FAST_CHESS="/workspace/external/testers/fast-chess/fastchess"
if [ ! -f "${FAST_CHESS}" ]; then
    echo "WARNING: fast-chess not found at ${FAST_CHESS}"
    echo "Please run ./run_stage8_sprt.sh first to set up fast-chess"
    exit 1
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

[Event "Opening"]
[Result "*"]
1. e4 e5 *

[Event "Opening"]
[Result "*"]
1. d4 d5 *

[Event "Opening"]
[Result "*"]
1. e4 c5 *

[Event "Opening"]
[Result "*"]
1. d4 Nf6 *
EOF
    echo "Created opening book with 8 openings"
fi

echo "Running SPRT test: Depth 4 vs Depth 3..."
echo "This demonstrates the strength gain from searching deeper"
echo ""

# Run fast-chess with SPRT
# Depth 4 should be significantly stronger than depth 3
"${FAST_CHESS}" \
    -engine name="${TEST_NAME}" cmd="${ENGINE_BIN}" \
    -engine name="${BASE_NAME}" cmd="${ENGINE_BIN}" \
    -each proto=uci \
    -engine tc=inf depth=4 \
    -engine tc=inf depth=3 \
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
    if grep -q "SPRT: H1 accepted" "${OUTPUT_DIR}/console_output.txt"; then
        echo "✓ TEST PASSED: Depth 4 is stronger than Depth 3"
        echo "Alpha-beta pruning enables effective deeper search"
    elif grep -q "SPRT: H0 accepted" "${OUTPUT_DIR}/console_output.txt"; then
        echo "✗ TEST FAILED: No improvement detected"
        echo "This is unexpected - deeper search should be stronger"
        echo "Please check the implementation"
    else
        echo "⋯ TEST INCONCLUSIVE: Maximum games reached"
        echo "Partial results may still show improvement"
    fi
fi

echo "============================================="
echo ""
echo "To view detailed results:"
echo "  cat ${OUTPUT_DIR}/console_output.txt"
echo "  less ${OUTPUT_DIR}/games.pgn"
echo ""

exit $RESULT