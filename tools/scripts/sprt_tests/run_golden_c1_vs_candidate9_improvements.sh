#!/bin/bash

# SPRT Test: Golden C1 vs Candidate 9 (Quiescence Improvements)
# Testing chess-engine-expert's suggested optimizations

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
WORKSPACE_ROOT="/workspace"
TIMESTAMP=$(date +%Y%m%d_%H%M%S)
TEST_NAME="SPRT-GoldenC1-vs-C9-IMPROVEMENTS-${TIMESTAMP}"
RESULT_DIR="${WORKSPACE_ROOT}/sprt_results/${TEST_NAME}"

# Create result directory
mkdir -p "${RESULT_DIR}"

# Engine binaries
ENGINE_GOLDEN="${WORKSPACE_ROOT}/binaries/seajay-stage14-sprt-candidate1-GOLDEN"
ENGINE_C9="${WORKSPACE_ROOT}/binaries/seajay-stage14-sprt-candidate9"

# Test configuration
TIME_CONTROL="10+0.1"
CONCURRENCY="1"  # Serial testing for consistency

# SPRT parameters
# Expecting improvement, so testing for positive ELO gain
ALPHA="0.05"
BETA="0.05"
ELO0="0"     # H0: No improvement
ELO1="30"    # H1: At least 30 ELO improvement (conservative estimate)

# Verify engines exist
if [ ! -f "${ENGINE_GOLDEN}" ]; then
    echo "Error: Golden C1 engine not found at ${ENGINE_GOLDEN}"
    exit 1
fi

if [ ! -f "${ENGINE_C9}" ]; then
    echo "Error: Candidate 9 engine not found at ${ENGINE_C9}"
    echo "Please build Candidate 9 first!"
    exit 1
fi

# Check for fast-chess
FASTCHESS="${WORKSPACE_ROOT}/external/testers/fast-chess/fastchess"
if [ ! -f "${FASTCHESS}" ]; then
    echo "fast-chess not found. Downloading..."
    mkdir -p "${WORKSPACE_ROOT}/external/testers/fast-chess"
    cd "${WORKSPACE_ROOT}/external/testers/fast-chess"
    wget https://github.com/Disservin/fastchess/releases/download/v9.1.0/fast-chess-linux-x64
    mv fast-chess-linux-x64 fastchess
    chmod +x fastchess
    cd "${SCRIPT_DIR}"
fi

# Opening book
OPENING_BOOK="${WORKSPACE_ROOT}/external/books/4moves_test.pgn"
if [ ! -f "${OPENING_BOOK}" ]; then
    echo "Error: Opening book not found at ${OPENING_BOOK}"
    exit 1
fi

# Get versions and checksums
echo "Verifying engines..."
GOLDEN_VERSION=$(echo -e "uci\nquit" | timeout 2 "${ENGINE_GOLDEN}" 2>&1 | grep "id name" | head -1)
C9_VERSION=$(echo -e "uci\nquit" | timeout 2 "${ENGINE_C9}" 2>&1 | grep "id name" | head -1)
GOLDEN_MD5=$(md5sum "${ENGINE_GOLDEN}" | cut -d' ' -f1)
C9_MD5=$(md5sum "${ENGINE_C9}" | cut -d' ' -f1)
GOLDEN_SIZE=$(ls -l "${ENGINE_GOLDEN}" | awk '{print $5}')
C9_SIZE=$(ls -l "${ENGINE_C9}" | awk '{print $5}')

# Create test info file
cat > "${RESULT_DIR}/test_info.txt" << EOF
SPRT Test: Golden C1 vs Candidate 9 (Quiescence Improvements)
=============================================================
Date: $(date)
Purpose: Test chess-engine-expert's suggested quiescence optimizations

IMPROVEMENTS IMPLEMENTED:
1. Delta Margin Reduction
   - DELTA_MARGIN: 900 → 200 centipawns
   - DELTA_MARGIN_ENDGAME: 600 → 150 centipawns
   - Expected: 20-30 ELO gain

2. Check Ply Limit Reduction
   - MAX_CHECK_PLY: 8 → 3 plies
   - Prevents search explosion
   - Better time management

3. Time Pressure Panic Mode
   - Activates when time < 100ms
   - Delta margin → 100cp
   - Max captures → 8 per node
   - Should reduce time losses

NOT YET IMPLEMENTED (deferred to C10):
- Quiet checks at depth 0 (worth 15-25 ELO)

Golden C1 Binary: ${ENGINE_GOLDEN}
  - The original working binary
  - Size: ${GOLDEN_SIZE} bytes (411,336)
  - MD5: ${GOLDEN_MD5}
  - Version: ${GOLDEN_VERSION}
  - Baseline performance

Candidate 9 Binary: ${ENGINE_C9}
  - Improved quiescence parameters
  - Size: ${C9_SIZE} bytes
  - MD5: ${C9_MD5}
  - Version: ${C9_VERSION}
  - Expected: +20-40 ELO improvement

Time Control: ${TIME_CONTROL}
SPRT Parameters:
  - H0: ELO difference = 0 (no improvement)
  - H1: ELO difference >= 30 (significant improvement)
  - Alpha: ${ALPHA}, Beta: ${BETA}
Opening Book: 4moves_test.pgn

Expected Results:
  - Candidate 9 should show 20-40 ELO improvement
  - Time losses should decrease from 1-2% to <0.5%
  - Tactical strength should improve with better delta pruning
EOF

echo "======================================"
echo "SPRT TEST: Golden C1 vs Candidate 9"
echo "======================================"
echo ""
echo "IMPROVEMENTS TESTED:"
echo "  1. Delta margins: 900→200cp"
echo "  2. Check depth: 8→3 plies"
echo "  3. Panic mode for time pressure"
echo ""
echo "EXPECTED GAIN: +20-40 ELO"
echo ""
echo "Binary Sizes:"
echo "  Golden C1:    ${GOLDEN_SIZE} bytes"
echo "  Candidate 9:  ${C9_SIZE} bytes"
echo ""
echo "Time Control: ${TIME_CONTROL}"
echo "SPRT Bounds: [${ELO0}, ${ELO1}] ELO"
echo ""
echo "Starting SPRT test..."
echo "======================================"

# Run the test
"${FASTCHESS}" \
    -engine name="Golden-C1" cmd="${ENGINE_GOLDEN}" \
    -engine name="C9-IMPROVED" cmd="${ENGINE_C9}" \
    -each proto=uci tc="${TIME_CONTROL}" \
    -openings file="${OPENING_BOOK}" format=pgn order=random \
    -games 2 \
    -rounds 10000 \
    -repeat \
    -recover \
    -concurrency "${CONCURRENCY}" \
    -sprt elo0="${ELO0}" elo1="${ELO1}" alpha="${ALPHA}" beta="${BETA}" \
    -ratinginterval 10 \
    -scoreinterval 10 \
    -pgnout "${RESULT_DIR}/games.pgn" fi \
    -log file="${RESULT_DIR}/fastchess.log" level=info \
    | tee "${RESULT_DIR}/console_output.txt"

# Save summary
echo "" >> "${RESULT_DIR}/console_output.txt"
echo "Test completed at: $(date)" >> "${RESULT_DIR}/console_output.txt"

# Parse results
echo ""
echo "======================================"
echo "Test Complete!"
echo "Results saved to: ${RESULT_DIR}"
echo ""

# Extract final statistics
if [ -f "${RESULT_DIR}/console_output.txt" ]; then
    echo "Final Statistics:"
    echo "-----------------"
    tail -30 "${RESULT_DIR}/console_output.txt" | grep -E "Elo|LLR|Games|Score|Result|accepted|rejected"
    
    echo ""
    echo "======================================"
    echo "INTERPRETATION:"
    echo ""
    
    # Check for results
    if grep -q "H0 accepted" "${RESULT_DIR}/console_output.txt"; then
        echo "❌ NO IMPROVEMENT DETECTED"
        echo ""
        echo "The optimizations did not provide significant improvement."
        echo "Possible reasons:"
        echo "1. Parameters need further tuning"
        echo "2. SeaJay's implementation differs from top engines"
        echo "3. Need to implement quiet checks for full benefit"
    elif grep -q "H1 accepted" "${RESULT_DIR}/console_output.txt"; then
        echo "✅ SUCCESS! IMPROVEMENT CONFIRMED!"
        echo ""
        echo "Candidate 9 shows significant improvement over Golden C1!"
        echo "The quiescence optimizations are working as expected."
        echo ""
        echo "Next steps:"
        echo "1. Consider implementing quiet checks (C10)"
        echo "2. Further parameter tuning possible"
        echo "3. Move to next stage of development"
    else
        echo "Test still running or inconclusive..."
        echo "Check back for final results"
    fi
    
    echo "======================================"
fi

echo ""
echo "View games: less ${RESULT_DIR}/games.pgn"
echo "View log: less ${RESULT_DIR}/fastchess.log"
echo "======================================"