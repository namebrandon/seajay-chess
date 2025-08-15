#!/bin/bash

# SPRT Test: Golden C1 (Original 411KB) vs Rebuilt C1 (from ce52720 commit, 384KB)
# This test will determine if the uncommitted changes in the golden C1 binary
# were responsible for the exceptional performance

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
WORKSPACE_ROOT="/workspace"
TIMESTAMP=$(date +%Y%m%d_%H%M%S)
TEST_NAME="SPRT-GoldenC1-vs-RebuiltC1-${TIMESTAMP}"
RESULT_DIR="${WORKSPACE_ROOT}/sprt_results/${TEST_NAME}"

# Create result directory
mkdir -p "${RESULT_DIR}"

# Engine binaries
ENGINE_GOLDEN="${WORKSPACE_ROOT}/binaries/seajay-stage14-sprt-candidate1-GOLDEN"
ENGINE_REBUILT="${WORKSPACE_ROOT}/binaries/seajay-stage14-rebuilt-from-ce52720"

# Test configuration
TIME_CONTROL="10+0.1"
CONCURRENCY="1"  # Serial testing for consistency

# SPRT parameters
# If the uncommitted changes were significant, we should see a clear difference
ALPHA="0.05"
BETA="0.05"
ELO0="0"
ELO1="50"  # Testing if golden is at least 50 ELO better

# Verify engines exist
if [ ! -f "${ENGINE_GOLDEN}" ]; then
    echo "Error: Golden C1 engine not found at ${ENGINE_GOLDEN}"
    echo "This is the preserved original binary that showed +300 ELO"
    exit 1
fi

if [ ! -f "${ENGINE_REBUILT}" ]; then
    echo "Error: Rebuilt C1 engine not found at ${ENGINE_REBUILT}"
    echo "Please ensure you've rebuilt from commit ce52720"
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

# Test engines respond to UCI and get their versions
echo "Testing engine responsiveness..."
echo "Testing Golden C1 (411KB)..."
GOLDEN_VERSION=$(echo -e "uci\nquit" | timeout 2 "${ENGINE_GOLDEN}" 2>&1 | grep "id name" | head -1)
if [ $? -ne 0 ]; then
    echo "Error: Golden C1 binary not responding to UCI"
    exit 1
fi

echo "Testing Rebuilt C1 (384KB)..."
REBUILT_VERSION=$(echo -e "uci\nquit" | timeout 2 "${ENGINE_REBUILT}" 2>&1 | grep "id name" | head -1)
if [ $? -ne 0 ]; then
    echo "Error: Rebuilt C1 binary not responding to UCI"
    exit 1
fi

# Display versions and checksums
echo "Golden C1: ${GOLDEN_VERSION}"
GOLDEN_MD5=$(md5sum "${ENGINE_GOLDEN}" | cut -d' ' -f1)
echo "  MD5: ${GOLDEN_MD5}"
echo "  Size: $(ls -l "${ENGINE_GOLDEN}" | awk '{print $5}') bytes"

echo "Rebuilt C1: ${REBUILT_VERSION}"
REBUILT_MD5=$(md5sum "${ENGINE_REBUILT}" | cut -d' ' -f1)
echo "  MD5: ${REBUILT_MD5}"
echo "  Size: $(ls -l "${ENGINE_REBUILT}" | awk '{print $5}') bytes"

# Create test info file
cat > "${RESULT_DIR}/test_info.txt" << EOF
SPRT Test: Golden C1 vs Rebuilt C1
=====================================
Date: $(date)
Purpose: Determine if uncommitted changes in golden C1 were responsible for +300 ELO

Golden C1 Binary: ${ENGINE_GOLDEN}
  - The original binary that showed +300 ELO gain
  - Size: 411,336 bytes
  - MD5: 0b0ea4c7f8f0aa60079a2ccc2997bf88
  - Version: ${GOLDEN_VERSION}
  - Contains uncommitted changes not in git

Rebuilt C1 Binary: ${ENGINE_REBUILT}
  - Rebuilt from commit ce52720
  - Size: 384,312 bytes  
  - MD5: ${REBUILT_MD5}
  - Version: ${REBUILT_VERSION}
  - Built from exact commit code

Time Control: ${TIME_CONTROL}
SPRT Parameters:
  - H0 (null): ELO difference <= ${ELO0}
  - H1 (alternative): ELO difference >= ${ELO1}
  - Alpha (Type I error): ${ALPHA}
  - Beta (Type II error): ${BETA}
Opening Book: 4moves_test.pgn

Expected Results:
  - If uncommitted changes were critical: Golden C1 will dominate
  - If uncommitted changes were minor: Similar performance
  - This test isolates the impact of the missing 27KB of code

Key Questions:
1. Were the uncommitted changes functional improvements?
2. Or were they debug/logging code that happened to help?
3. Can we reverse-engineer what made golden C1 special?
EOF

echo "======================================"
echo "SPRT Test: Golden C1 vs Rebuilt C1"
echo "======================================"
echo "Purpose: Test impact of uncommitted changes"
echo ""
echo "Golden C1:  411KB, MD5: ${GOLDEN_MD5}"
echo "            The binary that achieved +300 ELO"
echo ""
echo "Rebuilt C1: 384KB, MD5: ${REBUILT_MD5}"
echo "            From commit ce52720 (same code)"
echo ""
echo "Size difference: 27KB (uncommitted code)"
echo ""
echo "Opening Book: 4moves_test.pgn"
echo "Time Control: ${TIME_CONTROL}"
echo "SPRT Bounds: [${ELO0}, ${ELO1}] ELO"
echo ""
echo "This test will reveal if the uncommitted"
echo "changes were responsible for C1's success."
echo ""
echo "Output: ${RESULT_DIR}/games.pgn"
echo "======================================"
echo ""
echo "Starting SPRT test..."

# Run the test with correct fastchess syntax
"${FASTCHESS}" \
    -engine name="Golden-C1" cmd="${ENGINE_GOLDEN}" \
    -engine name="Rebuilt-C1" cmd="${ENGINE_REBUILT}" \
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
        echo "✅ CONFIRMED: Uncommitted changes were CRITICAL!"
        echo "    The 27KB of missing code contained the secret to +300 ELO"
        echo "    We need to reverse-engineer the golden binary"
    elif grep -q "H0 accepted" "${RESULT_DIR}/console_output.txt"; then
        echo ""
        echo "❌ SURPRISING: No significant difference!"
        echo "    The uncommitted changes were NOT responsible"
        echo "    Something else caused the performance difference"
    else
        echo ""
        echo "⚠️  Test ended without conclusion (may have hit game limit)"
    fi
fi

echo "======================================"
echo "View games: less ${RESULT_DIR}/games.pgn"
echo "View log: less ${RESULT_DIR}/fastchess.log"
echo "======================================"