#!/bin/bash
# Stage 15 Tuned vs Original SPRT Test - With 4moves Opening Book
# Tests Stage 15 tuned parameters against original Stage 15 SPRT candidate
# Expected: Small improvement or equality

echo "================================================"
echo "Stage 15 Tuned vs Original SPRT Test"
echo "================================================"
echo ""
echo "Testing: Stage 15 Tuned (Day 8) vs Stage 15 Original"
echo "Expected gain: 0-10 ELO (parameter optimization)"
echo "Time control: 10+0.1 (fast)"
echo "Opening book: 4moves_test.pgn"
echo ""

# Configuration
ENGINE_TEST="/workspace/binaries/seajay_stage15_tuned_fixed"
ENGINE_BASE="/workspace/binaries/seajay_stage15_sprt_candidate1"
OPENING_BOOK="/workspace/external/books/4moves_test.pgn"
OUTPUT_DIR="/workspace/sprt_results/stage15_tuned_vs_original"

# SPRT parameters
ELO0=-10    # Allow small regression (tuning might trade for stability)
ELO1=10     # Looking for small improvement
ALPHA=0.05  # 5% false positive rate
BETA=0.05   # 5% false negative rate

# Time control
TC="10+0.1"  # 10 seconds + 0.1 increment

# Concurrency - using serial games for consistency
CONCURRENCY=1

# Create output directory
mkdir -p "$OUTPUT_DIR"

# Verify binaries exist
if [ ! -f "$ENGINE_TEST" ]; then
    echo "ERROR: Test engine not found: $ENGINE_TEST"
    exit 1
fi

if [ ! -f "$ENGINE_BASE" ]; then
    echo "ERROR: Base engine not found: $ENGINE_BASE"
    exit 1
fi

if [ ! -f "$OPENING_BOOK" ]; then
    echo "ERROR: Opening book not found: $OPENING_BOOK"
    exit 1
fi

# Check for fast-chess
FASTCHESS="/workspace/external/testers/fast-chess/fastchess"
if [ ! -f "$FASTCHESS" ]; then
    echo "fast-chess not found, downloading..."
    /workspace/tools/scripts/setup-external-tools.sh
fi

# Display MD5 checksums for verification
echo "Binary checksums:"
echo "  Tuned:    $(md5sum $ENGINE_TEST | awk '{print $1}')"
echo "  Original: $(md5sum $ENGINE_BASE | awk '{print $1}')"
echo ""

# Run SPRT test
echo "Starting SPRT test..."
echo "Press Ctrl+C to stop early"
echo ""

# Use fast-chess for SPRT
"$FASTCHESS" \
    -engine cmd="$ENGINE_TEST" name=Stage15-Tuned \
        option.SEEMode=production option.SEEPruning=aggressive \
    -engine cmd="$ENGINE_BASE" name=Stage15-Original \
        option.SEEMode=production option.SEEPruning=aggressive \
    -each tc="$TC" \
    -openings file="$OPENING_BOOK" format=pgn order=random \
    -games 2 -rounds 10000 -repeat -concurrency "$CONCURRENCY" \
    -sprt elo0="$ELO0" elo1="$ELO1" alpha="$ALPHA" beta="$BETA" \
    -pgnout file="$OUTPUT_DIR/games.pgn" \
    -log file="$OUTPUT_DIR/fastchess.log" level=info \
    -recover \
    -event "Stage 15 Tuned vs Original" \
    -site "SPRT Parameter Test"

# Save results
echo ""
echo "Test complete!"
echo "Results saved to: $OUTPUT_DIR"
echo ""
echo "To view results:"
echo "  cat $OUTPUT_DIR/fastchess.log | tail -20"
echo "  grep 'LLR\\|Elo:' $OUTPUT_DIR/fastchess.log | tail -5"
echo "  less $OUTPUT_DIR/games.pgn"