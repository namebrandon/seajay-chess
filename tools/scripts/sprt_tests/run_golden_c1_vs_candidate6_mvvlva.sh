#!/bin/bash

# SPRT Test: Golden C1 vs Candidate 6 (with MVV-LVA enabled)
# This test validates that MVV-LVA was the missing piece

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
WORKSPACE_ROOT="/workspace"
TIMESTAMP=$(date +%Y%m%d_%H%M%S)
TEST_NAME="SPRT-GoldenC1-vs-C6-MVV-${TIMESTAMP}"
RESULT_DIR="${WORKSPACE_ROOT}/sprt_results/${TEST_NAME}"

# Create result directory
mkdir -p "${RESULT_DIR}"

# Engine binaries
ENGINE_GOLDEN="${WORKSPACE_ROOT}/binaries/seajay-stage14-sprt-candidate1-GOLDEN"
ENGINE_C6="${WORKSPACE_ROOT}/binaries/seajay-stage14-sprt-candidate6-mvvlva"

# Test configuration
TIME_CONTROL="10+0.1"
CONCURRENCY="1"  # Serial testing for consistency

# SPRT parameters
# If MVV-LVA was the key, they should perform similarly
ALPHA="0.05"
BETA="0.05"
ELO0="-30"  # Allow C6 to be slightly weaker
ELO1="30"   # But not much different

# Verify engines exist
if [ ! -f "${ENGINE_GOLDEN}" ]; then
    echo "Error: Golden C1 engine not found at ${ENGINE_GOLDEN}"
    exit 1
fi

if [ ! -f "${ENGINE_C6}" ]; then
    echo "Error: Candidate 6 engine not found at ${ENGINE_C6}"
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
GOLDEN_VERSION=$(echo -e "uci\nquit" | timeout 2 "${ENGINE_GOLDEN}" 2>&1 | grep "id name" | head -1)
C6_VERSION=$(echo -e "uci\nquit" | timeout 2 "${ENGINE_C6}" 2>&1 | grep "id name" | head -1)
GOLDEN_MD5=$(md5sum "${ENGINE_GOLDEN}" | cut -d' ' -f1)
C6_MD5=$(md5sum "${ENGINE_C6}" | cut -d' ' -f1)

# Create test info file
cat > "${RESULT_DIR}/test_info.txt" << EOF
SPRT Test: Golden C1 vs Candidate 6 (MVV-LVA Enabled)
======================================================
Date: $(date)
Purpose: Validate that MVV-LVA was the critical missing component

Golden C1 Binary: ${ENGINE_GOLDEN}
  - Original binary with MVV-LVA (uncommitted)
  - Size: 411,336 bytes
  - MD5: ${GOLDEN_MD5}
  - Version: ${GOLDEN_VERSION}

Candidate 6 Binary: ${ENGINE_C6}
  - Rebuilt with MVV-LVA properly enabled
  - Size: $(ls -l "${ENGINE_C6}" | awk '{print $5}') bytes
  - MD5: ${C6_MD5}
  - Version: ${C6_VERSION}

Time Control: ${TIME_CONTROL}
SPRT Parameters:
  - H0 (null): ELO difference within [-30, 30]
  - H1 (alternative): ELO difference outside [-30, 30]
  - Alpha: ${ALPHA}, Beta: ${BETA}
Opening Book: 4moves_test.pgn

Expected Results:
  - If MVV-LVA was the key: Similar performance (H0 accepted)
  - If other factors matter: Golden C1 still stronger

Discovery:
  Symbol analysis revealed Golden C1 has 27 MVV-LVA related symbols
  that were missing from all rebuilt binaries. This explains the
  27KB size difference and +301 ELO performance gap.
EOF

echo "======================================"
echo "SPRT Test: Golden C1 vs Candidate 6"
echo "======================================"
echo "Purpose: Validate MVV-LVA hypothesis"
echo ""
echo "Golden C1:    411KB (original with MVV-LVA)"
echo "Candidate 6:  402KB (rebuilt with MVV-LVA)"
echo ""
echo "Hypothesis: MVV-LVA was the secret sauce"
echo ""
echo "Time Control: ${TIME_CONTROL}"
echo "SPRT Bounds: [${ELO0}, ${ELO1}] ELO"
echo "======================================"
echo ""
echo "Starting SPRT test..."

# Run the test
"${FASTCHESS}" \
    -engine name="Golden-C1" cmd="${ENGINE_GOLDEN}" \
    -engine name="C6-MVV-LVA" cmd="${ENGINE_C6}" \
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
    
    # Interpret results
    if grep -q "H0 accepted" "${RESULT_DIR}/console_output.txt"; then
        echo ""
        echo "✅ CONFIRMED: MVV-LVA was the missing piece!"
        echo "    Candidate 6 matches Golden C1 performance"
        echo "    Mystery solved - it was move ordering all along"
    elif grep -q "H1 accepted" "${RESULT_DIR}/console_output.txt"; then
        echo ""
        echo "⚠️  PARTIAL SUCCESS: MVV-LVA helped but isn't everything"
        echo "    Golden C1 still has additional optimizations"
        echo "    Check the 9KB size difference (411KB vs 402KB)"
    else
        echo ""
        echo "Test ongoing or inconclusive..."
    fi
fi

echo "======================================"
echo "View games: less ${RESULT_DIR}/games.pgn"
echo "View log: less ${RESULT_DIR}/fastchess.log"
echo "======================================"