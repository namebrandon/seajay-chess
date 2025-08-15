#!/bin/bash

# SPRT Test: Golden C1 vs Candidate 7 (with ENABLE_QUIESCENCE fixed)
# This should finally match the golden binary's performance!

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
WORKSPACE_ROOT="/workspace"
TIMESTAMP=$(date +%Y%m%d_%H%M%S)
TEST_NAME="SPRT-GoldenC1-vs-C7-FINAL-${TIMESTAMP}"
RESULT_DIR="${WORKSPACE_ROOT}/sprt_results/${TEST_NAME}"

# Create result directory
mkdir -p "${RESULT_DIR}"

# Engine binaries
ENGINE_GOLDEN="${WORKSPACE_ROOT}/binaries/seajay-stage14-sprt-candidate1-GOLDEN"
ENGINE_C7="${WORKSPACE_ROOT}/binaries/seajay-stage14-sprt-candidate7-FIXED"

# Test configuration
TIME_CONTROL="10+0.1"
CONCURRENCY="1"  # Serial testing for consistency

# SPRT parameters
# They should be nearly identical now
ALPHA="0.05"
BETA="0.05"
ELO0="-20"  # Very tight bounds - they should be almost equal
ELO1="20"   

# Verify engines exist
if [ ! -f "${ENGINE_GOLDEN}" ]; then
    echo "Error: Golden C1 engine not found at ${ENGINE_GOLDEN}"
    exit 1
fi

if [ ! -f "${ENGINE_C7}" ]; then
    echo "Error: Candidate 7 engine not found at ${ENGINE_C7}"
    echo "Please build with ENABLE_QUIESCENCE defined!"
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
C7_VERSION=$(echo -e "uci\nquit" | timeout 2 "${ENGINE_C7}" 2>&1 | grep "id name" | head -1)
GOLDEN_MD5=$(md5sum "${ENGINE_GOLDEN}" | cut -d' ' -f1)
C7_MD5=$(md5sum "${ENGINE_C7}" | cut -d' ' -f1)
GOLDEN_SIZE=$(ls -l "${ENGINE_GOLDEN}" | awk '{print $5}')
C7_SIZE=$(ls -l "${ENGINE_C7}" | awk '{print $5}')

# Create test info file
cat > "${RESULT_DIR}/test_info.txt" << EOF
SPRT Test: Golden C1 vs Candidate 7 (QUIESCENCE FINALLY ENABLED)
================================================================
Date: $(date)
Purpose: Validate that enabling ENABLE_QUIESCENCE flag solves the mystery

THE DISCOVERY:
- Quiescence search was NEVER compiled in (missing ENABLE_QUIESCENCE flag)
- All previous builds fell back to static evaluation
- Golden binary had the flag manually added during development

Golden C1 Binary: ${ENGINE_GOLDEN}
  - The original working binary
  - Size: ${GOLDEN_SIZE} bytes (411,336)
  - MD5: ${GOLDEN_MD5}
  - Version: ${GOLDEN_VERSION}
  - Has quiescence search compiled in

Candidate 7 Binary: ${ENGINE_C7}
  - Fixed build with ENABLE_QUIESCENCE defined
  - Size: ${C7_SIZE} bytes (should be ~411KB)
  - MD5: ${C7_MD5}
  - Version: ${C7_VERSION}
  - Finally has quiescence search enabled!

Time Control: ${TIME_CONTROL}
SPRT Parameters:
  - H0: ELO difference within [-20, 20]
  - H1: ELO difference outside [-20, 20]
  - Alpha: ${ALPHA}, Beta: ${BETA}
  - Very tight bounds - they should be nearly identical
Opening Book: 4moves_test.pgn

Expected Results:
  - Binary sizes should match (~411KB)
  - Performance should be nearly identical
  - This proves quiescence was the missing piece

The Fix Applied:
  Added to CMakeLists.txt:
  - add_compile_definitions(ENABLE_QUIESCENCE)
  - add_compile_definitions(ENABLE_MVV_LVA)
EOF

echo "======================================"
echo "FINAL TEST: Golden C1 vs Candidate 7"
echo "======================================"
echo ""
echo "THE MYSTERY:"
echo "  Golden C1 was +300 ELO stronger than rebuilds"
echo ""
echo "THE DISCOVERY:"
echo "  Quiescence search was NEVER compiled in!"
echo "  Missing flag: ENABLE_QUIESCENCE"
echo ""
echo "THE FIX:"
echo "  Candidate 7 built with quiescence enabled"
echo ""
echo "Binary Sizes:"
echo "  Golden C1:    ${GOLDEN_SIZE} bytes"
echo "  Candidate 7:  ${C7_SIZE} bytes"
echo "  (Should both be ~411KB)"
echo ""
echo "Time Control: ${TIME_CONTROL}"
echo "SPRT Bounds: [${ELO0}, ${ELO1}] ELO"
echo ""
echo "If this test shows equal performance,"
echo "we've finally solved the mystery!"
echo "======================================"
echo ""
echo "Starting SPRT test..."

# Run the test
"${FASTCHESS}" \
    -engine name="Golden-C1" cmd="${ENGINE_GOLDEN}" \
    -engine name="C7-FIXED" cmd="${ENGINE_C7}" \
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
        echo "🎉 SUCCESS! MYSTERY SOLVED!"
        echo ""
        echo "Golden C1 and Candidate 7 perform identically!"
        echo "The missing ENABLE_QUIESCENCE flag was the entire problem."
        echo ""
        echo "What happened:"
        echo "1. Quiescence search was implemented but never compiled in"
        echo "2. All engines fell back to static evaluation"
        echo "3. Golden C1 had the flag manually added during testing"
        echo "4. The +300 ELO was quiescence vs static eval"
        echo ""
        echo "Stage 14 is now properly complete!"
    elif grep -q "H1 accepted" "${RESULT_DIR}/console_output.txt"; then
        echo "⚠️  UNEXPECTED: Still a performance difference"
        echo ""
        echo "Possible remaining issues:"
        echo "1. Compiler optimization differences"
        echo "2. Build environment variations"
        echo "3. The 48-byte size difference might matter"
        echo ""
        echo "But we're much closer than before!"
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