#!/bin/bash

# SPRT Test: SeaJay vs Stockfish (limited to ~800 ELO)
# Purpose: Test SeaJay against Stockfish with limited strength
# Date: $(date +"%Y-%m-%d")
# Test Type: External engine comparison with strength-limited opponent

# Change to workspace directory to ensure proper paths
cd /workspace

# Test identification
TEST_ID="SPRT-2025-EXT-004-SF800"
TEST_NAME="SeaJay-2.9.1"
OPPONENT_NAME="Stockfish-800ELO"

# SeaJay is estimated at ~650 ELO
# Stockfish will be limited to approximately 800 ELO using Skill Level
# Expected difference: ~150 ELO

# SPRT Parameters
# Testing if SeaJay can compete reasonably with 800 ELO opponent
ELO0=-250    # Lower bound - expecting some deficit
ELO1=-50     # Upper bound - would be good if within 50 ELO
ALPHA=0.05   # Type I error probability
BETA=0.05    # Type II error probability

# Test configuration
TC="10+0.1"     # Time control (10 seconds + 0.1 increment)
ROUNDS=500      # Maximum rounds (SPRT may stop earlier)
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
# SPRT Test: SeaJay vs Stockfish (800 ELO)

## Test Information
- **Test ID:** ${TEST_ID}
- **Date:** $(date)
- **Purpose:** Evaluate SeaJay against Stockfish limited to ~800 ELO

## Engines
- **SeaJay:** Version 2.9.1-draw-detection (Phase 2 complete, ~650 ELO estimated)
- **Stockfish:** Limited to ~800 ELO using Skill Level 0-1

## Stockfish Settings
- **Skill Level:** 0 (weakest setting, approximately 800 ELO)
- **UCI_LimitStrength:** Not used (minimum is 1320 ELO)
- **Threads:** 1 (single-threaded for consistency)
- **Hash:** 16 MB (minimal)

## SPRT Configuration
- **Hypothesis:** Testing if SeaJay is within 50-250 ELO of 800-rated opponent
- **Elo bounds:** [${ELO0}, ${ELO1}]
- **Significance:** α = ${ALPHA}, β = ${BETA}
- **Time control:** ${TC}
- **Opening book:** 4moves_test.pgn
- **Max rounds:** ${ROUNDS}
- **Concurrency:** Sequential (no concurrent games)

## Expected Outcomes
Given the ~150 ELO expected difference:
- SeaJay expected to score 20-30% (100-150 points out of 500 games)
- This would demonstrate SeaJay is approaching beginner human level
- Test will show if SeaJay can win some games against 800-rated play

## Notes
Stockfish at Skill Level 0 plays at approximately 800 ELO.
This represents a reasonable challenge for SeaJay's current development.
Success here would validate Phase 2 achievements.
EOF

echo "=================================================="
echo "SPRT Test: ${TEST_ID}"
echo "=================================================="
echo "Testing: ${TEST_NAME} vs ${OPPONENT_NAME}"
echo ""
echo "SeaJay (~650 ELO) vs Stockfish (limited to ~800 ELO)"
echo "Expected deficit: ~150 ELO"
echo ""
echo "SPRT bounds: [${ELO0}, ${ELO1}]"
echo "Time control: ${TC}"
echo "Concurrency: Sequential (no concurrency)"
echo ""
echo "Note: Using Stockfish Skill Level 0 for ~800 ELO strength"
echo ""
echo "Starting at $(date)"
echo "Test documentation saved to: ${OUTPUT_DIR}/test_info.md"
echo "=================================================="
echo ""

# Run fast-chess with SPRT
# Note: Setting Stockfish options for weak play
/workspace/external/testers/fast-chess/fastchess \
    -engine name="${TEST_NAME}" cmd="${SEAJAY_BIN}" \
    -engine name="${OPPONENT_NAME}" cmd="${STOCKFISH_BIN}" option."Skill Level"=0 option.Threads=1 option.Hash=16 \
    -each proto=uci tc="${TC}" \
    -sprt elo0="${ELO0}" elo1="${ELO1}" alpha="${ALPHA}" beta="${BETA}" \
    -rounds "${ROUNDS}" \
    -repeat \
    -recover \
    -openings file="${BOOK}" format=pgn order=random \
    -pgnout "${OUTPUT_DIR}/games_${TIMESTAMP}.pgn" fi \
    -event "SeaJay_vs_Stockfish_800" \
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
SeaJay (Phase 2, ~650 ELO) vs Stockfish (Skill Level 0, ~800 ELO)
Time Control: ${TC}

Results:
$(tail -20 "${OUTPUT_DIR}/console_output_${TIMESTAMP}.txt" | grep -E "(Score|Elo|SPRT|Games)")

Analysis:
This test evaluates SeaJay against a beginner-level opponent.
Stockfish at Skill Level 0 approximates 800 ELO play.
Results show how SeaJay performs against weak but sound chess.
EOF

echo ""
echo "Summary saved to: ${OUTPUT_DIR}/summary_${TIMESTAMP}.txt"