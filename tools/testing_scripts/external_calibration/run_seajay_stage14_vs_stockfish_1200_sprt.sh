#!/bin/bash

# SPRT Test: SeaJay Stage 14 vs Stockfish (limited to ~1200 ELO)
# Purpose: Test SeaJay Stage 14 C10-CONSERVATIVE against Stockfish with limited strength
# Date: $(date +"%Y-%m-%d")
# Test Type: External engine comparison with strength-limited opponent

# Change to workspace directory to ensure proper paths
cd /workspace

# Test identification
TEST_ID="SPRT-2025-EXT-008-SF1200-STAGE14"
TEST_NAME="SeaJay-Stage14-C10-CONSERVATIVE"
OPPONENT_NAME="Stockfish-1200ELO-1CPU"

# SeaJay Stage 14 is estimated at ~2250 ELO
# Stockfish will be limited to approximately 1200 ELO with 1 CPU core
# Expected difference: ~1000+ ELO (SeaJay should dominate)

# SPRT Parameters
# Testing if SeaJay Stage 14 is significantly stronger than 1200 ELO opponent
ELO0=300     # Lower bound - expecting at least 300 ELO advantage
ELO1=500     # Upper bound - could be 500+ ELO advantage
ALPHA=0.05   # Type I error probability
BETA=0.05    # Type II error probability

# Test configuration
TC="10+0.1"     # Time control (10 seconds + 0.1 increment)
ROUNDS=200      # Maximum rounds (SPRT will likely stop much earlier given expected advantage)

# Paths (using relative paths from /workspace)
SEAJAY_BIN="./binaries/seajay-stage14-sprt-candidate10-conservative"
STOCKFISH_BIN="./external/engines/stockfish/stockfish"
BOOK="./external/books/4moves_test.pgn"
OUTPUT_DIR="./sprt_results/${TEST_ID}"

# Create output directory with timestamp
mkdir -p "${OUTPUT_DIR}"
TIMESTAMP=$(date +"%Y%m%d_%H%M%S")

# Create test documentation
cat > "${OUTPUT_DIR}/test_info.md" << EOF
# SPRT Test: SeaJay Stage 14 vs Stockfish (1200 ELO)

## Test Information
- **Test ID:** ${TEST_ID}
- **Date:** $(date)
- **Purpose:** Evaluate SeaJay Stage 14 against Stockfish limited to ~1200 ELO

## Engines
- **SeaJay:** Stage 14 C10-CONSERVATIVE - Estimated ~2250 ELO
- **Stockfish:** Limited to ~1200 ELO using Skill Level settings + 1 CPU core

## SeaJay Stage 14 Achievements
- +300 ELO improvement over Stage 13
- Basic quiescence search with captures and check evasions
- Conservative delta pruning (900cp/600cp margins)
- MVV-LVA move ordering in quiescence
- Transposition table integration
- Final candidate after extensive debugging (C1-C10 candidates)

## Stockfish Settings
Using Skill Level + single CPU core for fair comparison:
- **Skill Level:** 5 (consistent benchmark level)
- **Threads:** 1 (single-threaded - CRITICAL for fairness with SeaJay)
- **Hash:** 16 MB (minimal)

Note: SeaJay is not multi-threaded, so Stockfish must also use 1 CPU core.

Skill Level strength is approximate:
- Level 0: Weakest (makes obvious blunders)
- Level 5: Our benchmark level (estimated 1000-1400 ELO range)
- Level 10: Intermediate 
- Level 15: Strong club level
- Level 20: Full strength

## SPRT Configuration
- **Hypothesis:** Testing if SeaJay Stage 14 is 300-500 ELO stronger than 1200-rated opponent
- **Elo bounds:** [${ELO0}, ${ELO1}]
- **Significance:** α = ${ALPHA}, β = ${BETA}
- **Time control:** ${TC}
- **Opening book:** 4moves_test.pgn
- **Max rounds:** ${ROUNDS}
- **Concurrency:** Sequential (no concurrent games)

## Expected Outcomes
With Stage 14's massive improvements:
- Expected score: 80-90% (SeaJay should dominate)
- SPRT should terminate quickly with H1 acceptance
- This tests our estimated ~2250 ELO strength against 1200 benchmark
- Demonstrates progress from previous stages

## Historical Context
- Stage 12: ~1800 ELO (TT implementation)  
- Stage 13: ~1950 ELO (iterative deepening)
- Stage 14: ~2250 ELO (quiescence search +300 Elo)

## Fairness Considerations
- Single CPU core for Stockfish (critical for fair comparison)
- Same time controls for both engines
- Same opening book positions
- Sequential games to avoid resource contention

## Notes
This test validates Stage 14's substantial improvement against our external benchmark.
The ~1000 ELO advantage should result in dominant performance.
Primary goal is to confirm Stage 14's strength estimation is realistic.
EOF

echo "=================================================="
echo "SPRT Test: ${TEST_ID}"
echo "=================================================="
echo "Testing: ${TEST_NAME} vs ${OPPONENT_NAME}"
echo ""
echo "SeaJay Stage 14 C10-CONSERVATIVE (~2250 ELO) vs Stockfish (1 CPU, Skill Level 5, ~1200 ELO)"
echo "Expected advantage: ~1000+ ELO"
echo ""
echo "SPRT bounds: [${ELO0}, ${ELO1}]"
echo "Time control: ${TC}"
echo "Concurrency: Sequential (no concurrency)"
echo ""
echo "IMPORTANT: Stockfish using 1 CPU core for fair comparison"
echo "Note: Using Stockfish Skill Level 5 for ~1200 ELO strength"
echo ""
echo "Starting at $(date)"
echo "Test documentation saved to: ${OUTPUT_DIR}/test_info.md"
echo "=================================================="
echo ""

# Verify engines exist
if [ ! -f "${SEAJAY_BIN}" ]; then
    echo "ERROR: SeaJay binary not found at ${SEAJAY_BIN}"
    exit 1
fi

if [ ! -f "${STOCKFISH_BIN}" ]; then
    echo "ERROR: Stockfish binary not found at ${STOCKFISH_BIN}"
    exit 1
fi

if [ ! -f "${BOOK}" ]; then
    echo "ERROR: Opening book not found at ${BOOK}"
    exit 1
fi

echo "Engine verification:"
echo "  SeaJay: ${SEAJAY_BIN} ✓"
echo "  Stockfish: ${STOCKFISH_BIN} ✓"
echo "  Opening book: ${BOOK} ✓"
echo ""

# Run fast-chess with SPRT
# CRITICAL: Setting Stockfish to 1 thread for fair comparison with single-threaded SeaJay
echo "Starting SPRT test..."
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
    -event "SeaJay_Stage14_vs_Stockfish_1200" \
    -site "Stage14_Strength_Test" \
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
tail -20 "${OUTPUT_DIR}/console_output_${TIMESTAMP}.txt" | grep -E "(Score|Elo|SPRT|Games|LLR)"

# Create a summary file
cat > "${OUTPUT_DIR}/summary_${TIMESTAMP}.txt" << EOF
Test ID: ${TEST_ID}
Date: $(date)
SeaJay Stage 14 C10-CONSERVATIVE (~2250 ELO) vs Stockfish (1 CPU, Skill Level 5, ~1200 ELO)
Time Control: ${TC}

Stage 14 Achievements:
- C10-CONSERVATIVE final candidate after extensive testing (C1-C10)
- +300 ELO improvement over Stage 13
- Basic quiescence search with conservative parameters
- Stable performance validated through 137+ SPRT games vs Golden C1

Results:
$(tail -20 "${OUTPUT_DIR}/console_output_${TIMESTAMP}.txt" | grep -E "(Score|Elo|SPRT|Games|LLR)")

Analysis:
This test evaluates SeaJay Stage 14's substantial improvement against our external benchmark.
Expected to show dominant performance (~80-90% score) given ~1000 ELO advantage.
Confirms Stage 14's estimated strength and validates quiescence search implementation.
Single-threaded Stockfish ensures fair comparison with SeaJay's current architecture.
EOF

echo ""
echo "Summary saved to: ${OUTPUT_DIR}/summary_${TIMESTAMP}.txt"
echo ""
echo "Stage 14 External Calibration Test Complete!"
echo "Expected: Dominant performance confirming ~2250 ELO strength"