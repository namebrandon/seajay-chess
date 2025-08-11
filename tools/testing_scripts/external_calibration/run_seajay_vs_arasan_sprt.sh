#!/bin/bash

# SPRT Test: SeaJay vs Arasan
# Purpose: Test SeaJay's strength against a known reference engine
# Date: $(date +"%Y-%m-%d")
# Test Type: External engine comparison

# Test identification
TEST_ID="SPRT-2025-EXT-002-ARASAN"
TEST_NAME="SeaJay-2.9.1"
OPPONENT_NAME="Arasan-25.2"

# SPRT Parameters
# Testing if SeaJay can achieve a reasonable performance against Arasan
# Arasan is approximately 3000+ ELO, so we expect a large deficit
# Testing if we can achieve at least 1% win rate (roughly -400 ELO)
ELO0=-500    # Lower bound - expecting significant weakness
ELO1=-300    # Upper bound - would be good for Phase 2 engine
ALPHA=0.05   # Type I error probability
BETA=0.05    # Type II error probability

# Test configuration
TC="10+0.1"     # Time control (10 seconds + 0.1 increment)
ROUNDS=500      # Maximum rounds (SPRT will likely stop earlier)
CONCURRENCY=4   # Number of concurrent games

# Paths
SEAJAY_BIN="/workspace/build/seajay"
ARASAN_BIN="/workspace/external/engines/arasan/arasanx-64"
BOOK="/workspace/external/books/4moves_test.pgn"
OUTPUT_DIR="/workspace/sprt_results/${TEST_ID}"

# Create output directory with timestamp
mkdir -p "${OUTPUT_DIR}"
TIMESTAMP=$(date +"%Y-%m-%d_%H-%M-%S")

# Create test documentation
cat > "${OUTPUT_DIR}/test_info.md" << EOF
# SPRT Test: SeaJay vs Arasan

## Test Information
- **Test ID:** ${TEST_ID}
- **Date:** $(date)
- **Purpose:** Evaluate SeaJay's strength against a strong reference engine

## Engines
- **SeaJay:** Version 2.9.1-draw-detection (Phase 2 complete, ~650 ELO estimated)
- **Arasan:** Version 25.2 (Strong engine, ~3000+ ELO)

## SPRT Configuration
- **Hypothesis:** SeaJay can achieve reasonable performance against Arasan
- **Elo bounds:** [${ELO0}, ${ELO1}]
- **Significance:** α = ${ALPHA}, β = ${BETA}
- **Time control:** ${TC}
- **Opening book:** 4moves_test.pgn
- **Max rounds:** ${ROUNDS}
- **Concurrency:** ${CONCURRENCY} games

## Expected Outcomes
- **H1 accepted:** SeaJay performs better than -300 ELO difference
- **H0 accepted:** SeaJay performs worse than -500 ELO difference

## Notes
This test establishes SeaJay's position relative to a very strong engine.
Given Arasan's strength (~3000 ELO), a large deficit is expected.
The goal is to verify SeaJay plays reasonable chess despite the skill gap.
EOF

echo "=================================================="
echo "SPRT Test: ${TEST_ID}"
echo "=================================================="
echo "Testing: ${TEST_NAME} vs ${OPPONENT_NAME}"
echo "Hypothesis: Testing performance against strong engine"
echo "SPRT bounds: [${ELO0}, ${ELO1}]"
echo "Time control: ${TC}"
echo "Concurrency: ${CONCURRENCY} games"
echo "Starting at $(date)"
echo ""
echo "Note: Arasan is ~3000 ELO, so a large deficit is expected"
echo "Test documentation saved to: ${OUTPUT_DIR}/test_info.md"
echo "=================================================="
echo ""

# Run fast-chess with SPRT
/workspace/external/testers/fast-chess/fastchess \
    -engine name="${TEST_NAME}" cmd="${SEAJAY_BIN}" \
    -engine name="${OPPONENT_NAME}" cmd="${ARASAN_BIN}" \
    -each proto=uci tc="${TC}" \
    -sprt elo0="${ELO0}" elo1="${ELO1}" alpha="${ALPHA}" beta="${BETA}" \
    -rounds "${ROUNDS}" \
    -repeat \
    -recover \
    -concurrency "${CONCURRENCY}" \
    -openings file="${BOOK}" format=pgn order=random \
    -pgnout "${OUTPUT_DIR}/games_${TIMESTAMP}.pgn" fi \
    -event "SeaJay_vs_Arasan_SPRT" \
    -site "External_Comparison_Test" \
    -log file="${OUTPUT_DIR}/fastchess_${TIMESTAMP}.log" level=info \
    2>&1 | tee "${OUTPUT_DIR}/console_output_${TIMESTAMP}.txt"

# Calculate final statistics and create summary
echo ""
echo "=================================================="
echo "Test completed at $(date)"
echo "Results saved to: ${OUTPUT_DIR}"
echo ""
echo "Files created:"
echo "  - games_${TIMESTAMP}.pgn (all games played)"
echo "  - console_output_${TIMESTAMP}.txt (test output)"
echo "  - fastchess_${TIMESTAMP}.log (detailed log)"
echo "  - test_info.md (test documentation)"
echo "=================================================="

# Parse and display key results
echo ""
echo "Extracting final results..."
tail -20 "${OUTPUT_DIR}/console_output_${TIMESTAMP}.txt" | grep -E "(Score|Elo|SPRT|Games)"

# Create a summary file
cat > "${OUTPUT_DIR}/summary_${TIMESTAMP}.txt" << EOF
Test ID: ${TEST_ID}
Date: $(date)
SeaJay (Phase 2, ~650 ELO) vs Arasan (~3000 ELO)
Time Control: ${TC}

Results:
$(tail -20 "${OUTPUT_DIR}/console_output_${TIMESTAMP}.txt" | grep -E "(Score|Elo|SPRT|Games)")

Note: Given the ~2350 ELO difference, SeaJay is expected to score poorly.
The goal is to verify SeaJay plays legal, reasonable chess.
EOF

echo ""
echo "Summary saved to: ${OUTPUT_DIR}/summary_${TIMESTAMP}.txt"