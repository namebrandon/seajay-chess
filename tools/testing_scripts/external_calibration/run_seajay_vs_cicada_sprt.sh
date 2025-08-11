#!/bin/bash

# SPRT Test: SeaJay vs Cicada
# Purpose: Test if SeaJay is within ±100 ELO of Cicada
# Date: $(date +"%Y-%m-%d")
# Test Type: External engine comparison

# Test identification
TEST_ID="SPRT-2025-EXT-001-CICADA"
TEST_NAME="SeaJay-2.9.1"
OPPONENT_NAME="Cicada-v0.1"

# SPRT Parameters
# H0: SeaJay is more than 100 ELO weaker than Cicada (diff < -100)
# H1: SeaJay is within 100 ELO of Cicada (diff >= -100)
# Using asymmetric bounds to test if we're "close enough"
ELO0=-100    # Lower bound - we're testing if we're at least this close
ELO1=0       # Upper bound - we don't need to prove we're better
ALPHA=0.05   # Type I error probability
BETA=0.05    # Type II error probability

# Test configuration
TC="10+0.1"     # Time control (10 seconds + 0.1 increment)
ROUNDS=2000     # Maximum rounds (SPRT may stop earlier)
CONCURRENCY=4   # Number of concurrent games

# Paths
SEAJAY_BIN="/workspace/build/seajay"
CICADA_BIN="/workspace/external/engines/cicada-linux-v0.1/cicada"
BOOK="/workspace/external/books/4moves_test.pgn"
OUTPUT_DIR="/workspace/sprt_results/${TEST_ID}"

# Create output directory with timestamp
mkdir -p "${OUTPUT_DIR}"
TIMESTAMP=$(date +"%Y-%m-%d_%H-%M-%S")

# Create test documentation
cat > "${OUTPUT_DIR}/test_info.md" << EOF
# SPRT Test: SeaJay vs Cicada

## Test Information
- **Test ID:** ${TEST_ID}
- **Date:** $(date)
- **Purpose:** Evaluate if SeaJay is within 100 ELO of Cicada

## Engines
- **SeaJay:** Version 2.9.1-draw-detection (Phase 2 complete)
- **Cicada:** Version 0.1 (external reference engine)

## SPRT Configuration
- **Hypothesis:** SeaJay is within 100 ELO of Cicada
- **Elo bounds:** [${ELO0}, ${ELO1}]
- **Significance:** α = ${ALPHA}, β = ${BETA}
- **Time control:** ${TC}
- **Opening book:** 4moves_test.pgn
- **Max rounds:** ${ROUNDS}
- **Concurrency:** ${CONCURRENCY} games

## Expected Outcomes
- **H1 accepted:** SeaJay is within 100 ELO of Cicada (competitive performance)
- **H0 accepted:** SeaJay is more than 100 ELO weaker (needs improvement)

## Notes
This is an external engine comparison test, not an internal development test.
The goal is to establish SeaJay's approximate strength relative to a known engine.
EOF

echo "=================================================="
echo "SPRT Test: ${TEST_ID}"
echo "=================================================="
echo "Testing: ${TEST_NAME} vs ${OPPONENT_NAME}"
echo "Hypothesis: SeaJay is within 100 ELO of Cicada"
echo "SPRT bounds: [${ELO0}, ${ELO1}]"
echo "Time control: ${TC}"
echo "Concurrency: ${CONCURRENCY} games"
echo "Starting at $(date)"
echo ""
echo "Test documentation saved to: ${OUTPUT_DIR}/test_info.md"
echo "=================================================="
echo ""

# Run fast-chess with SPRT
# Note: Using special PGN naming to isolate these external comparison games
/workspace/external/testers/fast-chess/fastchess \
    -engine name="${TEST_NAME}" cmd="${SEAJAY_BIN}" \
    -engine name="${OPPONENT_NAME}" cmd="${CICADA_BIN}" \
    -each proto=uci tc="${TC}" \
    -sprt elo0="${ELO0}" elo1="${ELO1}" alpha="${ALPHA}" beta="${BETA}" \
    -rounds "${ROUNDS}" \
    -repeat \
    -recover \
    -concurrency "${CONCURRENCY}" \
    -openings file="${BOOK}" format=pgn order=random \
    -pgnout "${OUTPUT_DIR}/games_${TIMESTAMP}.pgn" fi \
    -event "SeaJay_vs_Cicada_SPRT" \
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
SeaJay vs Cicada
Time Control: ${TC}

Results:
$(tail -20 "${OUTPUT_DIR}/console_output_${TIMESTAMP}.txt" | grep -E "(Score|Elo|SPRT|Games)")
EOF

echo ""
echo "Summary saved to: ${OUTPUT_DIR}/summary_${TIMESTAMP}.txt"
echo ""
echo "To analyze the games:"
echo "  pgn-extract -s ${OUTPUT_DIR}/games_${TIMESTAMP}.pgn"
echo ""
echo "To replay specific games:"
echo "  cutechess-cli -pgnin ${OUTPUT_DIR}/games_${TIMESTAMP}.pgn -pgnout analyze.pgn -analyze"