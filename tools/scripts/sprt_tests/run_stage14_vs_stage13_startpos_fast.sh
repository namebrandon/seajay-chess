#!/bin/bash

# SPRT Test: Stage 14 Candidate 2 vs Stage 13 SPRT-Fixed
# Opening: Starting position (no opening book)
# Time control: Fast (10+0.1)
# Expected: Stage 14 should show improvement but possibly less than with varied openings

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
WORKSPACE_ROOT="/workspace"
TIMESTAMP=$(date +%Y%m%d_%H%M%S)
TEST_NAME="SPRT-Stage14v13-startpos-fast-${TIMESTAMP}"
RESULT_DIR="${WORKSPACE_ROOT}/sprt_results/${TEST_NAME}"

# Create result directory
mkdir -p "${RESULT_DIR}"

# Engine binaries
ENGINE_NEW="${WORKSPACE_ROOT}/binaries/seajay-stage14-sprt-candidate2"
ENGINE_BASE="${WORKSPACE_ROOT}/binaries/seajay-stage13-sprt-fixed"

# Test configuration
TIME_CONTROL="10+0.1"
CONCURRENCY="1"  # Serial testing (historically used)

# SPRT parameters
ALPHA="0.05"
BETA="0.05"
ELO0="0"
ELO1="30"

# Verify engines exist
if [ ! -f "${ENGINE_NEW}" ]; then
    echo "Error: Stage 14 engine not found at ${ENGINE_NEW}"
    exit 1
fi

if [ ! -f "${ENGINE_BASE}" ]; then
    echo "Error: Stage 13 engine not found at ${ENGINE_BASE}"
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

# Test engines respond to UCI
echo "Testing engine responsiveness..."
echo -e "uci\nquit" | timeout 2 "${ENGINE_NEW}" > /dev/null 2>&1
if [ $? -ne 0 ]; then
    echo "Error: Stage 14 binary not responding to UCI"
    exit 1
fi

echo -e "uci\nquit" | timeout 2 "${ENGINE_BASE}" > /dev/null 2>&1
if [ $? -ne 0 ]; then
    echo "Error: Stage 13 binary not responding to UCI"
    exit 1
fi

# Create test info file
cat > "${RESULT_DIR}/test_info.txt" << EOF
SPRT Test: Stage 14 vs Stage 13 (Starting Position)
====================================================
Date: $(date)
Stage 14 Binary: ${ENGINE_NEW} (Candidate 2 - Time Control Fixes)
Stage 13 Binary: ${ENGINE_BASE}
Time Control: ${TIME_CONTROL}
SPRT Parameters:
  - H0 (null): ELO difference <= ${ELO0}
  - H1 (alternative): ELO difference >= ${ELO1}
  - Alpha (Type I error): ${ALPHA}
  - Beta (Type II error): ${BETA}
Opening: Starting position only
Expected Result: H1 accepted (+30+ Elo improvement)
Note: May show more draws than with opening variety
Features Tested:
  - Quiescence search effectiveness from startpos
  - Opening play improvements
  - Time control fixes (frequent checking, emergency cutoff, increased buffer)
EOF

echo "======================================"
echo "SPRT Test: Stage 14 vs Stage 13 (StartPos)"
echo "======================================"
echo "New Engine: Stage 14 (Quiescence Search)"
echo "Base Engine: Stage 13 (Iterative Deepening)"
echo "Opening: Starting Position (no book)"
echo "Time Control: ${TIME_CONTROL}"
echo "SPRT Bounds: [${ELO0}, ${ELO1}] ELO"
echo "Hypothesis: Stage 14 should gain 30+ ELO even from startpos"
echo "Output: ${RESULT_DIR}/games.pgn"
echo "======================================"
echo ""
echo "Note: Testing from startpos may result in more draws"
echo "and smaller ELO differences than with varied openings."
echo ""
echo "Starting SPRT test..."

# Run the test with correct fastchess syntax (no opening book)
"${FASTCHESS}" \
    -engine name="Stage14-QS" cmd="${ENGINE_NEW}" \
    -engine name="Stage13-ID" cmd="${ENGINE_BASE}" \
    -each proto=uci tc="${TIME_CONTROL}" \
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
    
    # Check if H0 or H1 was accepted
    if grep -q "H1 accepted" "${RESULT_DIR}/console_output.txt"; then
        echo ""
        echo "✅ SUCCESS: Stage 14 shows significant improvement over Stage 13!"
    elif grep -q "H0 accepted" "${RESULT_DIR}/console_output.txt"; then
        echo ""
        echo "❌ FAILURE: Stage 14 does not show expected improvement"
    else
        echo ""
        echo "⚠️  Test ended without conclusion (may have hit game limit)"
    fi
fi

echo "======================================"
echo "View games: less ${RESULT_DIR}/games.pgn"
echo "View log: less ${RESULT_DIR}/fastchess.log"
echo "======================================"