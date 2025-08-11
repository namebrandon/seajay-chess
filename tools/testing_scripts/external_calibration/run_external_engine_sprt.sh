#!/bin/bash

# Generic SPRT Test Script for External Engine Comparison
# This script can be configured for different external engines

# Configuration - EDIT THESE VALUES
TEST_ID="SPRT-2025-EXT-XXX"
OPPONENT_NAME="ExternalEngine"
OPPONENT_BIN="/path/to/engine"
OPPONENT_ELO=1500  # Estimated ELO of opponent

# Calculate appropriate SPRT bounds based on opponent strength
# SeaJay is estimated at ~650 ELO
SEAJAY_ELO=650
EXPECTED_DIFF=$((OPPONENT_ELO - SEAJAY_ELO))

# Set bounds to test if we're within reasonable range
if [ $EXPECTED_DIFF -gt 500 ]; then
    # Very strong opponent
    ELO0=-600
    ELO1=-400
elif [ $EXPECTED_DIFF -gt 300 ]; then
    # Strong opponent  
    ELO0=-400
    ELO1=-200
elif [ $EXPECTED_DIFF -gt 100 ]; then
    # Moderate opponent
    ELO0=-200
    ELO1=-50
else
    # Similar strength
    ELO0=-100
    ELO1=100
fi

echo "=================================================="
echo "External Engine SPRT Test Configuration"
echo "=================================================="
echo "SeaJay (~${SEAJAY_ELO} ELO) vs ${OPPONENT_NAME} (~${OPPONENT_ELO} ELO)"
echo "Expected difference: ${EXPECTED_DIFF} ELO"
echo "SPRT bounds selected: [${ELO0}, ${ELO1}]"
echo ""
echo "To run this test:"
echo "1. Edit this script to set:"
echo "   - TEST_ID (unique identifier)"
echo "   - OPPONENT_NAME (engine name)"
echo "   - OPPONENT_BIN (path to engine binary)"
echo "   - OPPONENT_ELO (estimated strength)"
echo ""
echo "2. Verify the opponent engine works:"
echo "   echo -e 'uci\\nquit' | \${OPPONENT_BIN}"
echo ""
echo "3. Run the test:"
echo "   ./run_external_engine_sprt.sh"
echo "=================================================="

# Don't run if not configured
if [ "$OPPONENT_BIN" = "/path/to/engine" ]; then
    echo ""
    echo "ERROR: Please configure the script first!"
    exit 1
fi

# Test configuration
TEST_NAME="SeaJay-2.9.1"
ALPHA=0.05
BETA=0.05
TC="10+0.1"
ROUNDS=1000
CONCURRENCY=4

# Paths
SEAJAY_BIN="/workspace/build/seajay"
BOOK="/workspace/external/books/4moves_test.pgn"
OUTPUT_DIR="/workspace/sprt_results/${TEST_ID}"

# Create output directory
mkdir -p "${OUTPUT_DIR}"
TIMESTAMP=$(date +"%Y-%m-%d_%H-%M-%S")

# Create test documentation
cat > "${OUTPUT_DIR}/test_info.md" << EOF
# SPRT Test: SeaJay vs ${OPPONENT_NAME}

## Test Information
- **Test ID:** ${TEST_ID}
- **Date:** $(date)
- **Purpose:** Compare SeaJay strength against external engine

## Engines
- **SeaJay:** Version 2.9.1-draw-detection (~${SEAJAY_ELO} ELO)
- **${OPPONENT_NAME}:** Estimated ${OPPONENT_ELO} ELO

## SPRT Configuration
- **Expected difference:** ${EXPECTED_DIFF} ELO
- **Elo bounds:** [${ELO0}, ${ELO1}]
- **Significance:** α = ${ALPHA}, β = ${BETA}
- **Time control:** ${TC}
- **Concurrency:** ${CONCURRENCY} games

## Hypothesis
Testing if SeaJay performs within the expected range against ${OPPONENT_NAME}.
EOF

echo ""
echo "Starting SPRT test at $(date)"
echo ""

# Run the test
/workspace/external/testers/fast-chess/fastchess \
    -engine name="${TEST_NAME}" cmd="${SEAJAY_BIN}" \
    -engine name="${OPPONENT_NAME}" cmd="${OPPONENT_BIN}" \
    -each proto=uci tc="${TC}" \
    -sprt elo0="${ELO0}" elo1="${ELO1}" alpha="${ALPHA}" beta="${BETA}" \
    -rounds "${ROUNDS}" \
    -repeat \
    -recover \
    -concurrency "${CONCURRENCY}" \
    -openings file="${BOOK}" format=pgn order=random \
    -pgnout "${OUTPUT_DIR}/games_${TIMESTAMP}.pgn" fi \
    -event "SeaJay_vs_${OPPONENT_NAME}" \
    -log file="${OUTPUT_DIR}/fastchess_${TIMESTAMP}.log" level=info \
    2>&1 | tee "${OUTPUT_DIR}/console_output_${TIMESTAMP}.txt"

echo ""
echo "Test completed at $(date)"
echo "Results saved to: ${OUTPUT_DIR}"