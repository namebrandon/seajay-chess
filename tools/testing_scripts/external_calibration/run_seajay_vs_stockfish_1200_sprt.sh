#!/bin/bash

# SPRT Test: SeaJay vs Stockfish (limited to ~1200 ELO)
# Purpose: Test SeaJay against Stockfish with limited strength
# Date: $(date +"%Y-%m-%d")
# Test Type: External engine comparison with strength-limited opponent

# Change to workspace directory to ensure proper paths
cd /workspace

# Test identification
TEST_ID="SPRT-2025-EXT-006-SF1200"
TEST_NAME="SeaJay-2.12.tt"
OPPONENT_NAME="Stockfish-1200ELO"

# SeaJay Stage 12 is estimated at ~1300 ELO
# Stockfish will be limited to approximately 1200 ELO
# Expected difference: ~100 ELO (SeaJay slightly stronger)

# SPRT Parameters
# Testing if SeaJay Stage 12 is stronger than 1200 ELO opponent
ELO0=50      # Lower bound - testing if at least 50 ELO stronger
ELO1=150     # Upper bound - expecting around 100 ELO advantage
ALPHA=0.05   # Type I error probability
BETA=0.05    # Type II error probability

# Test configuration
TC="10+0.1"     # Time control (10 seconds + 0.1 increment)
ROUNDS=300      # Maximum rounds (SPRT will likely stop earlier)
# CONCURRENCY removed - SeaJay doesn't handle concurrent instances

# Paths (using relative paths from /workspace)
SEAJAY_BIN="./binaries/seajay-stage12-tt-candidate1-x86-64"
STOCKFISH_BIN="./external/engines/stockfish/stockfish"
BOOK="./external/books/4moves_test.pgn"
OUTPUT_DIR="./sprt_results/${TEST_ID}"

# Create output directory with timestamp
mkdir -p "${OUTPUT_DIR}"
TIMESTAMP=$(date +"%Y-%m-%d_%H-%M-%S")

# Create test documentation
cat > "${OUTPUT_DIR}/test_info.md" << EOF
# SPRT Test: SeaJay Stage 12 vs Stockfish (1200 ELO)

## Test Information
- **Test ID:** ${TEST_ID}
- **Date:** $(date)
- **Purpose:** Evaluate SeaJay against Stockfish limited to ~1200 ELO

## Engines
- **SeaJay:** Stage 12 TT Final - Estimated ~1300 ELO
- **Stockfish:** Limited to ~1200 ELO using Skill Level settings

## Stockfish Settings
Using Skill Level for consistent benchmark (UCI_Elo minimum is 1320):
- **Skill Level:** 5 (consistent benchmark level)
- **Threads:** 1 (single-threaded for consistency)
- **Hash:** 16 MB (minimal)

Note: Skill Level strength is approximate and varies by hardware/TC:
- Level 0: Weakest (makes obvious blunders)
- Level 5: Our benchmark level (estimated 1000-1400 ELO range)
- Level 10: Intermediate 
- Level 15: Strong club level
- Level 20: Full strength

The exact Elo is less important than having a consistent benchmark to measure progress.

## SPRT Configuration
- **Hypothesis:** Testing if SeaJay Stage 12 is 50-150 ELO stronger than 1200-rated opponent
- **Elo bounds:** [${ELO0}, ${ELO1}]
- **Significance:** α = ${ALPHA}, β = ${BETA}
- **Time control:** ${TC}
- **Opening book:** 4moves_test.pgn
- **Max rounds:** ${ROUNDS}
- **Concurrency:** Sequential (no concurrent games)

## Expected Outcomes
Progressive benchmarks by stage:
- If SeaJay scores >50%: We're competitive with this benchmark
- If SeaJay scores 60-65%: Significant improvement confirmed
- If SeaJay scores <40%: More work needed to reach this level

Key is tracking improvement across stages against this consistent benchmark.

## Notes
Stockfish Skill Level 5 serves as our external benchmark opponent.
Actual Elo may vary, but consistency allows us to measure progress.
Each stage should show improvement against this fixed reference point.
Phase 3-4 improvements should significantly close this gap.
EOF

echo "=================================================="
echo "SPRT Test: ${TEST_ID}"
echo "=================================================="
echo "Testing: ${TEST_NAME} vs ${OPPONENT_NAME}"
echo ""
echo "SeaJay Stage 12 (~1300 ELO) vs Stockfish (limited to ~1200 ELO)"
echo "Expected advantage: ~100 ELO"
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
SeaJay Stage 12 (~1300 ELO) vs Stockfish (Skill Level 5, ~1200 ELO)
Time Control: ${TC}

Results:
$(tail -20 "${OUTPUT_DIR}/console_output_${TIMESTAMP}.txt" | grep -E "(Score|Elo|SPRT|Games)")

Analysis:
This test evaluates SeaJay Stage 12 against our Stockfish Skill Level 5 benchmark.
This provides a consistent external reference point to measure improvement.
Progress tracking: Compare this result with previous stages to confirm improvement.
Goal: Eventually achieve >50% score to show competitive strength at this level.
EOF

echo ""
echo "Summary saved to: ${OUTPUT_DIR}/summary_${TIMESTAMP}.txt"
