#!/bin/bash

# IMPROVED SPRT Test Script for SeaJay Stage 8 vs Stage 7
# Uses better opening book to reduce repetition draws

TEST_ID="SPRT-2025-006"
TEST_NAME="Stage8-AlphaBeta"
BASE_NAME="Stage7-NoAlphaBeta"

# SPRT Parameters
ELO0=0       # Lower bound
ELO1=100     # Upper bound for time-based test
ALPHA=0.05   # Type I error probability
BETA=0.05    # Type II error probability

# Test configuration
TC="10+0.1"  # Time control
ROUNDS=1000  # Maximum rounds

# Paths
TEST_BIN="/workspace/bin/seajay_stage8_alphabeta"
BASE_BIN="/workspace/bin/seajay_stage7_no_alphabeta"
BOOK="/workspace/external/books/varied_4moves.pgn"  # Better opening book
OUTPUT_DIR="/workspace/sprt_results/${TEST_ID}"

# Create output directory
mkdir -p "${OUTPUT_DIR}"

echo "============================================="
echo "IMPROVED SPRT Test: ${TEST_ID}"
echo "Testing: Stage 8 (with alpha-beta) vs Stage 7 (without)"
echo "Using varied opening book to reduce repetitions"
echo "SPRT bounds: [${ELO0}, ${ELO1}]"
echo "Time Control: ${TC}"
echo "Starting at $(date)"
echo "============================================="
echo ""

# Verify binaries exist
if [ ! -f "${TEST_BIN}" ]; then
    echo "ERROR: Stage 8 binary not found at ${TEST_BIN}"
    exit 1
fi

if [ ! -f "${BASE_BIN}" ]; then
    echo "ERROR: Stage 7 binary not found at ${BASE_BIN}"
    exit 1
fi

# Check if better opening book exists
if [ ! -f "${BOOK}" ]; then
    echo "ERROR: Varied opening book not found at ${BOOK}"
    echo "Please ensure varied_4moves.pgn exists"
    exit 1
fi

echo "Opening book: Using 30 varied positions (4 moves deep)"
echo "This should significantly reduce repetition draws"
echo ""

# Check if fast-chess exists
FAST_CHESS="/workspace/external/testers/fast-chess/fastchess"
if [ ! -f "${FAST_CHESS}" ]; then
    echo "WARNING: fast-chess not found at ${FAST_CHESS}"
    echo "Attempting to download and build fast-chess..."
    
    mkdir -p /workspace/external/testers
    cd /workspace/external/testers
    
    if [ ! -d "fast-chess" ]; then
        git clone https://github.com/Disservin/fastchess.git fast-chess
    fi
    
    cd fast-chess
    make -j4
    
    if [ ! -f "fastchess" ]; then
        echo "ERROR: Failed to build fast-chess"
        exit 1
    fi
    
    cd /workspace
fi

echo "Running IMPROVED SPRT test..."
echo ""

# Run fast-chess with better settings to reduce draws
"${FAST_CHESS}" \
    -engine name="${TEST_NAME}" cmd="${TEST_BIN}" \
    -engine name="${BASE_NAME}" cmd="${BASE_BIN}" \
    -each proto=uci tc="${TC}" \
    -games 2 \
    -sprt elo0="${ELO0}" elo1="${ELO1}" alpha="${ALPHA}" beta="${BETA}" \
    -rounds "${ROUNDS}" \
    -repeat \
    -recover \
    -openings file="${BOOK}" format=pgn order=random plies=8 \
    -draw movenumber=40 movecount=8 score=10 \
    -resign movecount=3 score=400 \
    -pgnout "${OUTPUT_DIR}/games.pgn" \
    -log file="${OUTPUT_DIR}/fastchess.log" level=info \
    2>&1 | tee "${OUTPUT_DIR}/console_output.txt"

RESULT=$?

echo ""
echo "============================================="
echo "Test completed at $(date)"
echo "Results saved to: ${OUTPUT_DIR}"
echo ""

# Analyze draw rate
if [ -f "${OUTPUT_DIR}/console_output.txt" ]; then
    echo "Test Results Summary:"
    echo "---------------------"
    grep -E "Score|Elo|SPRT|LLR" "${OUTPUT_DIR}/console_output.txt" | tail -10
    echo ""
    
    # Check draw statistics
    TOTAL_GAMES=$(grep -c "Result" "${OUTPUT_DIR}/games.pgn" 2>/dev/null || echo "0")
    DRAWS=$(grep -c "1/2-1/2" "${OUTPUT_DIR}/games.pgn" 2>/dev/null || echo "0")
    REPETITIONS=$(grep -c "repetition" "${OUTPUT_DIR}/games.pgn" 2>/dev/null || echo "0")
    
    echo "Game Statistics:"
    echo "----------------"
    echo "Total games: ${TOTAL_GAMES}"
    echo "Total draws: ${DRAWS}"
    echo "Repetition draws: ${REPETITIONS}"
    
    if [ "$TOTAL_GAMES" -gt 0 ]; then
        DRAW_RATE=$((DRAWS * 100 / TOTAL_GAMES))
        echo "Draw rate: ${DRAW_RATE}%"
        
        if [ "$DRAWS" -gt 0 ]; then
            REP_RATE=$((REPETITIONS * 100 / DRAWS))
            echo "Repetitions as % of draws: ${REP_RATE}%"
        fi
    fi
    
    echo ""
    
    # Result interpretation
    if grep -q "SPRT: H1 accepted" "${OUTPUT_DIR}/console_output.txt"; then
        echo "✓ TEST PASSED: Stage 8 is significantly stronger!"
    elif grep -q "SPRT: H0 accepted" "${OUTPUT_DIR}/console_output.txt"; then
        echo "✗ Unexpected: No improvement detected"
    else
        echo "⋯ TEST INCONCLUSIVE"
    fi
fi

echo ""
echo "Notes on Repetition Draws:"
echo "--------------------------"
echo "High repetition rate is expected because:"
echo "1. Neither engine has repetition detection implemented"
echo "2. Both engines play deterministically"
echo "3. This will be fixed when repetition detection is added"
echo ""
echo "The varied opening book should help reduce this issue."
echo ""

exit $RESULT