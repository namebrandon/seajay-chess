#!/bin/bash

# SPRT Test: SeaJay vs Shallow Blue
# Purpose: Test SeaJay's strength against Shallow Blue 2.0.0
# Date: $(date +"%Y-%m-%d")
# Test Type: External engine comparison

# Change to workspace directory to ensure proper paths
cd /workspace

# Test identification
TEST_ID="SPRT-2025-EXT-003-SHALLOWBLUE"
TEST_NAME="SeaJay-2.9.1"
OPPONENT_NAME="ShallowBlue-2.0.0"

# Shallow Blue is rated 1617 ELO
# SeaJay is estimated at ~650 ELO
# Expected difference: ~967 ELO

# SPRT Parameters
# Testing if SeaJay can achieve meaningful performance against Shallow Blue
# With ~967 ELO difference, SeaJay would score approximately 0.2-2%
ELO0=-1200   # Lower bound - expecting significant deficit
ELO1=-800    # Upper bound - would be good if within 800 ELO
ALPHA=0.05   # Type I error probability
BETA=0.05    # Type II error probability

# Test configuration
TC="10+0.1"     # Time control (10 seconds + 0.1 increment)
ROUNDS=200      # Maximum rounds (SPRT will likely stop very quickly)
# CONCURRENCY removed - SeaJay doesn't handle concurrent instances

# Paths (using relative paths from /workspace)
SEAJAY_BIN="./build/seajay"
SHALLOWBLUE_BIN="./external/engines/shallow-blue-2.0.0/shallowblue"
BOOK="./external/books/4moves_test.pgn"
OUTPUT_DIR="./sprt_results/${TEST_ID}"

# Create output directory with timestamp
mkdir -p "${OUTPUT_DIR}"
TIMESTAMP=$(date +"%Y-%m-%d_%H-%M-%S")

# Create test documentation
cat > "${OUTPUT_DIR}/test_info.md" << EOF
# SPRT Test: SeaJay vs Shallow Blue

## Test Information
- **Test ID:** ${TEST_ID}
- **Date:** $(date)
- **Purpose:** Evaluate SeaJay's performance against Shallow Blue

## Engines
- **SeaJay:** Version 2.9.1-draw-detection (Phase 2 complete, ~650 ELO estimated)
- **Shallow Blue:** Version 2.0.0 (Rated 1617 ELO, with magic bitboards)

## Features Comparison
### SeaJay (Phase 2)
- 4-ply search with alpha-beta pruning
- Material evaluation + Piece-Square Tables
- Basic draw detection
- ~795K NPS

### Shallow Blue
- Magic bitboards for move generation
- Principal Variation Search (PVS)
- Transposition tables
- Quiescence search
- Advanced evaluation (mobility, king safety, pawn structure)
- Move ordering (MVV/LVA, killer heuristic, history heuristic)

## SPRT Configuration
- **Hypothesis:** Testing if deficit is less than 1500 ELO
- **Elo bounds:** [${ELO0}, ${ELO1}]
- **Significance:** α = ${ALPHA}, β = ${BETA}
- **Time control:** ${TC}
- **Opening book:** 4moves_test.pgn
- **Max rounds:** ${ROUNDS}
- **Concurrency:** Sequential (no concurrent games)

## Expected Outcomes
Given the ~967 ELO difference:
- SeaJay expected to score 0.2-2% (approximately 0-4 points out of 200 games)
- Test will likely terminate quickly confirming the large skill gap
- This establishes a baseline for measuring future improvements

## Notes
With Shallow Blue at 1617 ELO, it represents a strong amateur level engine.
This test helps establish how far SeaJay needs to progress to reach
competitive amateur level. The ~967 ELO gap shows SeaJay needs to gain
approximately 1000 ELO through Phases 3-4 development.
EOF

echo "=================================================="
echo "SPRT Test: ${TEST_ID}"
echo "=================================================="
echo "Testing: ${TEST_NAME} vs ${OPPONENT_NAME}"
echo ""
echo "SeaJay (~650 ELO) vs Shallow Blue (1617 ELO)"
echo "Expected deficit: ~967 ELO"
echo ""
echo "SPRT bounds: [${ELO0}, ${ELO1}]"
echo "Time control: ${TC}"
echo "Concurrency: Sequential (no concurrency)"
echo ""
echo "Note: With this skill gap, SeaJay is expected to score <2%"
echo "The test will likely stop quickly after confirming the deficit"
echo ""
echo "Starting at $(date)"
echo "Test documentation saved to: ${OUTPUT_DIR}/test_info.md"
echo "=================================================="
echo ""

# Run fast-chess with SPRT
# Note: Explicitly disable Shallow Blue's opening book to ensure fair testing
/workspace/external/testers/fast-chess/fastchess \
    -engine name="${TEST_NAME}" cmd="${SEAJAY_BIN}" \
    -engine name="${OPPONENT_NAME}" cmd="${SHALLOWBLUE_BIN}" option.OwnBook=false \
    -each proto=uci tc="${TC}" \
    -sprt elo0="${ELO0}" elo1="${ELO1}" alpha="${ALPHA}" beta="${BETA}" \
    -rounds "${ROUNDS}" \
    -repeat \
    -recover \
    -openings file="${BOOK}" format=pgn order=random \
    -pgnout "${OUTPUT_DIR}/games_${TIMESTAMP}.pgn" fi \
    -event "SeaJay_vs_ShallowBlue_SPRT" \
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
SeaJay (Phase 2, ~650 ELO) vs Shallow Blue (1617 ELO)
Time Control: ${TC}

Results:
$(tail -20 "${OUTPUT_DIR}/console_output_${TIMESTAMP}.txt" | grep -E "(Score|Elo|SPRT|Games)")

Analysis:
Given the significant strength difference (~967 ELO), these results were expected.
SeaJay needs substantial improvements (Phase 3-4) to compete at this level.
Key areas for improvement:
- Magic bitboards for faster move generation
- Transposition tables
- Quiescence search
- Better evaluation terms
- Deeper search capability
EOF

echo ""
echo "Summary saved to: ${OUTPUT_DIR}/summary_${TIMESTAMP}.txt"
echo ""
echo "To analyze individual games:"
echo "  pgn-extract -s ${OUTPUT_DIR}/games_${TIMESTAMP}.pgn"