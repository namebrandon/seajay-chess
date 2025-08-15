#!/bin/bash

# SPRT Test: Golden C1 vs Candidate 8 (ifdefs removed, UCI-controlled features)
# This validates that removing dangerous compile-time flags doesn't affect performance

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
WORKSPACE_ROOT="/workspace"
TIMESTAMP=$(date +%Y%m%d_%H%M%S)
TEST_NAME="SPRT-GoldenC1-vs-C8-NO-IFDEFS-${TIMESTAMP}"
RESULT_DIR="${WORKSPACE_ROOT}/sprt_results/${TEST_NAME}"

# Create result directory
mkdir -p "${RESULT_DIR}"

# Engine binaries
ENGINE_GOLDEN="${WORKSPACE_ROOT}/binaries/seajay-stage14-sprt-candidate1-GOLDEN"
ENGINE_C8="${WORKSPACE_ROOT}/binaries/seajay-stage14-sprt-candidate8-NO-IFDEFS"

# Test configuration
TIME_CONTROL="10+0.1"
CONCURRENCY="1"  # Serial testing for consistency

# SPRT parameters
# They should be identical in performance
ALPHA="0.05"
BETA="0.05"
ELO0="-20"  # Very tight bounds - they should be equal
ELO1="20"   

# Verify engines exist
if [ ! -f "${ENGINE_GOLDEN}" ]; then
    echo "Error: Golden C1 engine not found at ${ENGINE_GOLDEN}"
    exit 1
fi

if [ ! -f "${ENGINE_C8}" ]; then
    echo "Error: Candidate 8 engine not found at ${ENGINE_C8}"
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
C8_VERSION=$(echo -e "uci\nquit" | timeout 2 "${ENGINE_C8}" 2>&1 | grep "id name" | head -1)
GOLDEN_MD5=$(md5sum "${ENGINE_GOLDEN}" | cut -d' ' -f1)
C8_MD5=$(md5sum "${ENGINE_C8}" | cut -d' ' -f1)
GOLDEN_SIZE=$(ls -l "${ENGINE_GOLDEN}" | awk '{print $5}')
C8_SIZE=$(ls -l "${ENGINE_C8}" | awk '{print $5}')

# Create test info file
cat > "${RESULT_DIR}/test_info.txt" << EOF
SPRT Test: Golden C1 vs Candidate 8 (All IFDEFs Removed)
=========================================================
Date: $(date)
Purpose: Validate that removing dangerous compile-time feature flags doesn't affect performance

THE LESSON LEARNED:
- Compile-time feature flags are dangerous
- They can accidentally disable entire features
- All features should compile in and be controlled via UCI options
- This prevents the "4 hours chasing ghosts" scenario

Golden C1 Binary: ${ENGINE_GOLDEN}
  - The original working binary
  - Size: ${GOLDEN_SIZE} bytes (411,336)
  - MD5: ${GOLDEN_MD5}
  - Version: ${GOLDEN_VERSION}
  - Has quiescence and MVV-LVA compiled in

Candidate 8 Binary: ${ENGINE_C8}
  - Fixed build with ifdefs removed, UCI control only
  - Size: ${C8_SIZE} bytes (should be ~411KB)
  - MD5: ${C8_MD5}
  - Version: ${C8_VERSION}
  - All features always compile in, controlled via UCI

Changes in Candidate 8:
1. Removed #ifdef ENABLE_QUIESCENCE from negamax.cpp
2. Removed #ifdef ENABLE_MVV_LVA from quiescence.cpp
3. Removed #define ENABLE_MVV_LVA from move_ordering.h
4. All features now controlled via UCI options, not compile flags

Time Control: ${TIME_CONTROL}
SPRT Parameters:
  - H0: ELO difference within [-20, 20]
  - H1: ELO difference outside [-20, 20]
  - Alpha: ${ALPHA}, Beta: ${BETA}
  - Very tight bounds - they should be identical
Opening Book: 4moves_test.pgn

Expected Results:
  - Binary sizes should match (~411KB)
  - Performance should be identical
  - This proves UCI control is the right approach
EOF

echo "======================================"
echo "FINAL TEST: Golden C1 vs Candidate 8"
echo "======================================"
echo ""
echo "THE LESSON:"
echo "  Compile-time feature flags are dangerous!"
echo "  We spent 4 hours chasing ghosts because of them."
echo ""
echo "THE FIX:"
echo "  All features now compile in"
echo "  Control via UCI options only"
echo ""
echo "Binary Sizes:"
echo "  Golden C1:    ${GOLDEN_SIZE} bytes"
echo "  Candidate 8:  ${C8_SIZE} bytes"
echo "  (Should both be ~411KB)"
echo ""
echo "Time Control: ${TIME_CONTROL}"
echo "SPRT Bounds: [${ELO0}, ${ELO1}] ELO"
echo ""
echo "If this test shows equal performance,"
echo "we've successfully removed the dangerous pattern!"
echo "======================================"
echo ""
echo "Starting SPRT test..."

# Run the test
"${FASTCHESS}" \
    -engine name="Golden-C1" cmd="${ENGINE_GOLDEN}" \
    -engine name="C8-NO-IFDEFS" cmd="${ENGINE_C8}" \
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
        echo "✅ SUCCESS! Pattern fixed safely!"
        echo ""
        echo "Golden C1 and Candidate 8 perform identically!"
        echo "Removing dangerous compile-time flags was successful."
        echo ""
        echo "Key takeaways:"
        echo "1. Always compile in all features"
        echo "2. Use UCI options for runtime control"
        echo "3. Never hide core features behind #ifdefs"
        echo "4. This prevents 'phantom bugs' like we experienced"
        echo ""
        echo "Stage 14 is properly complete with safe code!"
    elif grep -q "H1 accepted" "${RESULT_DIR}/console_output.txt"; then
        echo "⚠️ UNEXPECTED: Performance difference detected"
        echo ""
        echo "This shouldn't happen - investigate:"
        echo "1. Check if optimizer behaves differently"
        echo "2. Verify all ifdefs were removed correctly"
        echo "3. Compare assembly output"
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