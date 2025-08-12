#!/bin/bash

# SPRT Test for Stage 10: Magic Bitboards vs Stage 9b
# Test ID: SPRT-2025-002
# Date: 2025-08-12
# Feature: Magic Bitboards (55.98x speedup in attack generation)

TEST_ID="SPRT-2025-002"
TEST_NAME="Stage10-Magic"
BASE_NAME="Stage9b-NoMagic"

# SPRT Parameters
# Expected improvement: 30-50 Elo from dramatically faster move generation
ELO0=0      # Null hypothesis: no improvement
ELO1=30     # Alternative hypothesis: 30+ Elo gain
ALPHA=0.05  # Type I error probability
BETA=0.05   # Type II error probability

# Test configuration
TC="10+0.1"  # 10 seconds + 0.1 increment
ROUNDS=2000  # Maximum rounds (SPRT will likely stop earlier)

# Paths
TEST_BIN="/workspace/bin/seajay_stage10_magic"
BASE_BIN="/workspace/bin/seajay_stage9b_no_magic"
BOOK="/workspace/external/books/4moves_test.pgn"
OUTPUT_DIR="/workspace/sprt_results/${TEST_ID}"

# Create output directory
mkdir -p "${OUTPUT_DIR}"

echo "=========================================="
echo "SPRT Test: ${TEST_ID}"
echo "Testing: ${TEST_NAME} vs ${BASE_NAME}"
echo "Feature: Magic Bitboards Implementation"
echo "Expected: 55.98x faster attack generation"
echo "SPRT bounds: [${ELO0}, ${ELO1}]"
echo "Time control: ${TC}"
echo "Max rounds: ${ROUNDS}"
echo "Starting at $(date)"
echo "=========================================="
echo ""

# Verify binaries exist
if [ ! -f "${TEST_BIN}" ]; then
    echo "ERROR: Test binary not found: ${TEST_BIN}"
    exit 1
fi

if [ ! -f "${BASE_BIN}" ]; then
    echo "ERROR: Base binary not found: ${BASE_BIN}"
    exit 1
fi

# Verify fast-chess exists
if [ ! -f "/workspace/external/testers/fast-chess/fastchess" ]; then
    echo "ERROR: fast-chess not found at /workspace/external/testers/fast-chess/fastchess"
    echo "Please run: ./tools/scripts/setup-external-tools.sh"
    exit 1
fi

# Verify opening book exists
if [ ! -f "${BOOK}" ]; then
    echo "ERROR: Opening book not found: ${BOOK}"
    echo "Please run: ./tools/scripts/setup-external-tools.sh"
    exit 1
fi

echo "All prerequisites verified ✓"
echo ""

# Run fast-chess with SPRT
/workspace/external/testers/fast-chess/fastchess \
    -engine name="${TEST_NAME}" cmd="${TEST_BIN}" \
    -engine name="${BASE_NAME}" cmd="${BASE_BIN}" \
    -each proto=uci tc="${TC}" \
    -sprt elo0="${ELO0}" elo1="${ELO1}" alpha="${ALPHA}" beta="${BETA}" \
    -rounds "${ROUNDS}" \
    -repeat \
    -recover \
    -openings file="${BOOK}" format=pgn \
    -pgnout "${OUTPUT_DIR}/games.pgn" \
    -log file="${OUTPUT_DIR}/fastchess.log" level=info \
    2>&1 | tee "${OUTPUT_DIR}/console_output.txt"

# Calculate exit status
EXIT_STATUS=$?

echo ""
echo "=========================================="
echo "Test completed at $(date)"
echo "Exit status: ${EXIT_STATUS}"
echo "Results saved to: ${OUTPUT_DIR}"
echo ""
echo "To view results:"
echo "  cat ${OUTPUT_DIR}/console_output.txt | tail -20"
echo "=========================================="

exit ${EXIT_STATUS}