#!/bin/bash

# SPRT Test: SeaJay vs Stockfish (limited to ~1200 ELO)
# Purpose: Test SeaJay against Stockfish with limited strength
# Date: $(date +"%Y-%m-%d")
# Test Type: External engine comparison with strength-limited opponent

# Change to workspace directory to ensure proper paths
cd /workspace

# Test identification
TEST_ID="SPRT-2025-EXT-005-SF1200"
TEST_NAME="SeaJay-2.9.1"
OPPONENT_NAME="Stockfish-1200ELO"

# SeaJay is estimated at ~650 ELO
# Stockfish will be limited to approximately 1200 ELO
# Expected difference: ~550 ELO

# SPRT Parameters
# Testing if SeaJay can achieve any meaningful score against 1200 ELO opponent
ELO0=-700    # Lower bound - expecting significant deficit
ELO1=-400    # Upper bound - would be respectable if within 400 ELO
ALPHA=0.05   # Type I error probability
BETA=0.05    # Type II error probability

# Test configuration
TC="10+0.1"     # Time control (10 seconds + 0.1 increment)
ROUNDS=300      # Maximum rounds (SPRT will likely stop earlier)
# CONCURRENCY removed - SeaJay doesn't handle concurrent instances

# Paths (using relative paths from /workspace)
SEAJAY_BIN="./build/seajay"
STOCKFISH_BIN="./external/engines/stockfish/stockfish"
BOOK="./external/books/4moves_test.pgn"
OUTPUT_DIR="./sprt_results/${TEST_ID}"

# Create output directory with timestamp
mkdir -p "${OUTPUT_DIR}"
TIMESTAMP=$(date +"%Y-%m-%d_%H-%M-%S")

# Create test documentation
cat > "${OUTPUT_DIR}/test_info.md" << EOF
# SPRT Test: SeaJay vs Stockfish (1200 ELO)

## Test Information
- **Test ID:** ${TEST_ID}
- **Date:** $(date)
- **Purpose:** Evaluate SeaJay against Stockfish limited to ~1200 ELO

## Engines
- **SeaJay:** Version 2.9.1-draw-detection (Phase 2 complete, ~650 ELO estimated)
- **Stockfish:** Limited to ~1200 ELO using Skill Level settings

## Stockfish Settings
Since UCI_Elo minimum is 1320, we'll use Skill Level to approximate 1200 ELO:
- **Skill Level:** 5 (approximately 1200 ELO)
- **Threads:** 1 (single-threaded for consistency)
- **Hash:** 16 MB (minimal)

Note: Skill Level mapping (approximate):
- Level 0: ~800 ELO
- Level 5: ~1200 ELO
- Level 10: ~1800 ELO
- Level 15: ~2300 ELO
- Level 20: Full strength

## SPRT Configuration
- **Hypothesis:** Testing if SeaJay can stay within 400-700 ELO of 1200-rated opponent
- **Elo bounds:** [${ELO0}, ${ELO1}]
- **Significance:** α = ${ALPHA}, β = ${BETA}
- **Time control:** ${TC}
- **Opening book:** 4moves_test.pgn
- **Max rounds:** ${ROUNDS}
- **Concurrency:** Sequential (no concurrent games)

## Expected Outcomes
Given the ~550 ELO expected difference:
- SeaJay expected to score 2-8% (6-24 points out of 300 games)
- This represents a significant challenge for current SeaJay
- Test establishes how much improvement is needed for club-level play

## Notes
Stockfish at Skill Level 5 approximates intermediate amateur play (~1200 ELO).
This test shows the gap between SeaJay and typical online players.
Phase 3-4 improvements should significantly close this gap.
EOF

echo "=================================================="
echo "SPRT Test: ${TEST_ID}"
echo "=================================================="
echo "Testing: ${TEST_NAME} vs ${OPPONENT_NAME}"
echo ""
echo "SeaJay (~650 ELO) vs Stockfish (limited to ~1200 ELO)"
echo "Expected deficit: ~550 ELO"
echo ""
echo "SPRT bounds: [${ELO0}, ${ELO1}]"
echo "Time control: ${TC}"
echo "Concurrency: Sequential (no concurrency)"
echo ""
echo "Note: Using Stockfish Skill Level 5 for ~1200 ELO strength"
echo ""
echo "Starting at $(date)"
echo "Test documentation saved to: ${OUTPUT_DIR}/test_info.md"
echo "=================================================="
echo ""

# Run fast-chess with SPRT
# Note: Setting Stockfish options for intermediate play
/workspace/external/testers/fast-chess/fastchess \
    -engine name="${TEST_NAME}" cmd="${SEAJAY_BIN}" \
    -engine name="${OPPONENT_NAME}" cmd="${STOCKFISH_BIN}" option."Skill Level"=5 option.Threads=1 option.Hash=16 \
    -each proto=uci tc="${TC}" \
    -sprt elo0="${ELO0}" elo1="${ELO1}" alpha="${ALPHA}" beta="${BETA}" \
    -rounds "${ROUNDS}" \
    -repeat \
    -recover \
    -openings file="${BOOK}" format=pgn order=random \
    -pgnout "${OUTPUT_DIR}/games_${TIMESTAMP}.pgn" fi \
    -event "SeaJay_vs_Stockfish_1200" \
    -site "Strength_Limited_Test" \
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
SeaJay (Phase 2, ~650 ELO) vs Stockfish (Skill Level 5, ~1200 ELO)
Time Control: ${TC}

Results:
$(tail -20 "${OUTPUT_DIR}/console_output_${TIMESTAMP}.txt" | grep -E "(Score|Elo|SPRT|Games)")

Analysis:
This test evaluates SeaJay against intermediate amateur level play.
Stockfish at Skill Level 5 approximates 1200 ELO.
Results show the development gap to reach club player strength.
EOF

echo ""
echo "Summary saved to: ${OUTPUT_DIR}/summary_${TIMESTAMP}.txt"