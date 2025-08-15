#!/bin/bash

# SPRT Test: Stage 14 Candidate 1 (Original) vs Candidate 5 (Clean Rebuild)
# Direct head-to-head comparison to validate performance difference
# This test will definitively prove if C1 truly performed better than C5
# or if environmental factors affected the original test

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
WORKSPACE_ROOT="/workspace"
TIMESTAMP=$(date +%Y%m%d_%H%M%S)
TEST_NAME="SPRT-C1vsC5-direct-${TIMESTAMP}"
RESULT_DIR="${WORKSPACE_ROOT}/sprt_results/${TEST_NAME}"

# Create result directory
mkdir -p "${RESULT_DIR}"

# Engine binaries - BOTH Stage 14 candidates
ENGINE_C1="${WORKSPACE_ROOT}/binaries/seajay-stage14-sprt-candidate1"
ENGINE_C5="${WORKSPACE_ROOT}/binaries/seajay-stage14-sprt-candidate5"

# Test configuration
TIME_CONTROL="10+0.1"
CONCURRENCY="1"  # Serial testing for consistency

# SPRT parameters
# If C1 really was +300 ELO and C5 is +87 ELO, difference should be ~200+ ELO
# We'll test for a more modest difference to be conservative
ALPHA="0.05"
BETA="0.05"
ELO0="0"
ELO1="50"  # Testing if C1 is at least 50 ELO better than C5

# Verify engines exist
if [ ! -f "${ENGINE_C1}" ]; then
    echo "Error: Candidate 1 engine not found at ${ENGINE_C1}"
    exit 1
fi

if [ ! -f "${ENGINE_C5}" ]; then
    echo "Error: Candidate 5 engine not found at ${ENGINE_C5}"
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
    echo "Please ensure 4moves_test.pgn is available"
    exit 1
fi

# Test engines respond to UCI
echo "Testing engine responsiveness..."
echo "Testing Candidate 1..."
C1_VERSION=$(echo -e "uci\nquit" | timeout 2 "${ENGINE_C1}" 2>&1 | grep "id name" | head -1)
if [ $? -ne 0 ]; then
    echo "Error: Candidate 1 binary not responding to UCI"
    exit 1
fi

echo "Testing Candidate 5..."
C5_VERSION=$(echo -e "uci\nquit" | timeout 2 "${ENGINE_C5}" 2>&1 | grep "id name" | head -1)
if [ $? -ne 0 ]; then
    echo "Error: Candidate 5 binary not responding to UCI"
    exit 1
fi

# Display versions
echo "Candidate 1: ${C1_VERSION}"
echo "Candidate 5: ${C5_VERSION}"

# Create test info file
cat > "${RESULT_DIR}/test_info.txt" << EOF
SPRT Test: Candidate 1 vs Candidate 5 (Direct Comparison)
=========================================================
Date: $(date)
Purpose: Validate if C1 truly performs better than C5 or if environmental factors affected original test

Candidate 1 Binary: ${ENGINE_C1}
  - Original implementation showing +300 ELO gain
  - Size: 411,336 bytes
  - MD5: 0b0ea4c7f8f0aa60079a2ccc2997bf88
  - Version: ${C1_VERSION}

Candidate 5 Binary: ${ENGINE_C5}  
  - Clean rebuild with reverted code
  - Size: 384,312 bytes
  - MD5: 78781e3c12eec07c1db03d4df1d4393a
  - Version: ${C5_VERSION}

Time Control: ${TIME_CONTROL}
SPRT Parameters:
  - H0 (null): ELO difference <= ${ELO0}
  - H1 (alternative): ELO difference >= ${ELO1}
  - Alpha (Type I error): ${ALPHA}
  - Beta (Type II error): ${BETA}
Opening Book: 4moves_test.pgn

Expected Results:
  - If C1 truly +300 ELO and C5 ~+87 ELO: H1 should be accepted quickly
  - If no real difference: Test will hover around LLR=0
  - If environmental issue in original test: C1 and C5 should perform similarly

Key Question: Is the binary size difference (411KB vs 384KB) causing the performance gap?
EOF

echo "======================================"
echo "SPRT Test: Candidate 1 vs Candidate 5"
echo "======================================"
echo "Purpose: Direct head-to-head comparison"
echo "Engine 1: Stage 14 Candidate 1 (Original, 411KB)"
echo "Engine 2: Stage 14 Candidate 5 (Clean rebuild, 384KB)"
echo "Opening Book: 4moves_test.pgn"
echo "Time Control: ${TIME_CONTROL}"
echo "SPRT Bounds: [${ELO0}, ${ELO1}] ELO"
echo ""
echo "This test will definitively answer:"
echo "- Is C1 truly stronger than C5?"
echo "- Or were there environmental factors in the original test?"
echo ""
echo "Output: ${RESULT_DIR}/games.pgn"
echo "======================================"
echo ""
echo "Starting SPRT test..."

# Run the test with correct fastchess syntax
"${FASTCHESS}" \
    -engine name="C1-Original" cmd="${ENGINE_C1}" \
    -engine name="C5-Rebuild" cmd="${ENGINE_C5}" \
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
    
    # Check if H0 or H1 was accepted
    if grep -q "H1 accepted" "${RESULT_DIR}/console_output.txt"; then
        echo ""
        echo "✅ CONFIRMED: Candidate 1 is significantly stronger than Candidate 5"
        echo "    There IS a real performance difference to investigate"
    elif grep -q "H0 accepted" "${RESULT_DIR}/console_output.txt"; then
        echo ""
        echo "❌ SURPRISING: No significant difference between C1 and C5"
        echo "    Original C1 test may have had environmental factors"
    else
        echo ""
        echo "⚠️  Test ended without conclusion (may have hit game limit)"
    fi
fi

echo "======================================"
echo "View games: less ${RESULT_DIR}/games.pgn"
echo "View log: less ${RESULT_DIR}/fastchess.log"
echo "======================================"