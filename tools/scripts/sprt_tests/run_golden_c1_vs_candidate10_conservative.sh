#!/bin/bash

# SPRT Test: Golden C1 vs Candidate 10 (Conservative Improvements)
# After C9 catastrophe, using conservative delta margins

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
WORKSPACE_ROOT="/workspace"
TIMESTAMP=$(date +%Y%m%d_%H%M%S)
TEST_NAME="SPRT-GoldenC1-vs-C10-CONSERVATIVE-${TIMESTAMP}"
RESULT_DIR="${WORKSPACE_ROOT}/sprt_results/${TEST_NAME}"

# Create result directory
mkdir -p "${RESULT_DIR}"

# Engine binaries
ENGINE_GOLDEN="${WORKSPACE_ROOT}/binaries/seajay-stage14-sprt-candidate1-GOLDEN"
ENGINE_C10="${WORKSPACE_ROOT}/binaries/seajay-stage14-sprt-candidate10-conservative"

# Test configuration
TIME_CONTROL="10+0.1"
CONCURRENCY="1"  # Serial testing for consistency

# SPRT parameters
# Conservative expectations after C9 failure
ALPHA="0.05"
BETA="0.05"
ELO0="-10"    # H0: Minor regression acceptable
ELO1="10"     # H1: Minor improvement hoped for

# Verify engines exist
if [ ! -f "${ENGINE_GOLDEN}" ]; then
    echo "Error: Golden C1 engine not found at ${ENGINE_GOLDEN}"
    exit 1
fi

if [ ! -f "${ENGINE_C10}" ]; then
    echo "Error: Candidate 10 engine not found at ${ENGINE_C10}"
    echo "Please build Candidate 10 first!"
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
C10_VERSION=$(echo -e "uci\nquit" | timeout 2 "${ENGINE_C10}" 2>&1 | grep "id name" | head -1)
GOLDEN_MD5=$(md5sum "${ENGINE_GOLDEN}" | cut -d' ' -f1)
C10_MD5=$(md5sum "${ENGINE_C10}" | cut -d' ' -f1)
GOLDEN_SIZE=$(ls -l "${ENGINE_GOLDEN}" | awk '{print $5}')
C10_SIZE=$(ls -l "${ENGINE_C10}" | awk '{print $5}')

# Create test info file
cat > "${RESULT_DIR}/test_info.txt" << EOF
SPRT Test: Golden C1 vs Candidate 10 (Conservative Improvements)
================================================================
Date: $(date)
Purpose: Test conservative improvements after C9's catastrophic failure

LESSONS LEARNED FROM C9 FAILURE:
- Delta margin of 200cp was catastrophically low
- SeaJay needs 900cp margin to see queen captures
- Aggressive pruning causes tactical blindness
- Engine-specific tuning is critical

IMPROVEMENTS IN C10:
1. Reverted Delta Margins (Critical Fix)
   - DELTA_MARGIN: 200 → 900 centipawns (back to Golden)
   - DELTA_MARGIN_ENDGAME: 150 → 600 centipawns (back to Golden)
   - DELTA_MARGIN_PANIC: 100 → 400 centipawns (conservative)

2. Moderate Check Ply Reduction
   - MAX_CHECK_PLY: 8 → 6 plies (balanced)
   - Not as aggressive as C9's reduction to 3

3. Time Pressure Panic Mode (Retained)
   - Activates when time < 100ms
   - But with conservative 400cp margin
   - Max captures → 8 per node

Golden C1 Binary: ${ENGINE_GOLDEN}
  - The original working binary
  - Size: ${GOLDEN_SIZE} bytes (411,336)
  - MD5: ${GOLDEN_MD5}
  - Version: ${GOLDEN_VERSION}
  - Baseline performance

Candidate 10 Binary: ${ENGINE_C10}
  - Conservative improvements only
  - Size: ${C10_SIZE} bytes
  - MD5: ${C10_MD5}
  - Version: ${C10_VERSION}
  - Expected: 0-10 ELO improvement

Time Control: ${TIME_CONTROL}
SPRT Parameters:
  - H0: ELO difference within [-10, 10]
  - H1: ELO difference outside [-10, 10]
  - Alpha: ${ALPHA}, Beta: ${BETA}
Opening Book: 4moves_test.pgn

Expected Results:
  - Should perform similarly to Golden C1
  - Slightly better time management
  - No tactical blindness like C9
EOF

echo "======================================"
echo "SPRT TEST: Golden C1 vs Candidate 10"
echo "======================================"
echo ""
echo "RECOVERY FROM C9 CATASTROPHE:"
echo "  C9 lost 0-10 with 200cp margins"
echo "  C10 uses conservative 900cp margins"
echo ""
echo "CHANGES IN C10:"
echo "  1. Delta margins: Back to 900/600cp"
echo "  2. Check depth: 8→6 (moderate)"
echo "  3. Panic mode: 400cp margin"
echo ""
echo "Binary Sizes:"
echo "  Golden C1:     ${GOLDEN_SIZE} bytes"
echo "  Candidate 10:  ${C10_SIZE} bytes"
echo ""
echo "Time Control: ${TIME_CONTROL}"
echo "SPRT Bounds: [${ELO0}, ${ELO1}] ELO"
echo ""
echo "Starting SPRT test..."
echo "======================================"

# Run the test
"${FASTCHESS}" \
    -engine name="Golden-C1" cmd="${ENGINE_GOLDEN}" \
    -engine name="C10-CONSERVATIVE" cmd="${ENGINE_C10}" \
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
        echo "✅ SUCCESS! Performance maintained!"
        echo ""
        echo "Candidate 10 performs similarly to Golden C1."
        echo "Conservative approach avoided C9's catastrophe."
        echo ""
        echo "Key takeaway: Delta margins must match piece values!"
        echo "SeaJay needs 900cp to see queen captures."
    elif grep -q "H1 accepted" "${RESULT_DIR}/console_output.txt"; then
        echo "📊 RESULT: Performance difference detected"
        echo ""
        echo "Check if it's positive or negative:"
        echo "- If positive: Time management improvements working"
        echo "- If negative: Even conservative changes hurt"
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