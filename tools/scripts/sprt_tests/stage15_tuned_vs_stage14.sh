#!/bin/bash
# Stage 15 Tuned vs Stage 14 SPRT Test - With 4moves Opening Book
# Tests Stage 15 with tuned parameters against Stage 14 baseline
# Expected: +30-50 ELO improvement (should maintain or exceed original gain)

echo "================================================"
echo "Stage 15 Tuned vs Stage 14 SPRT Test"
echo "================================================"
echo ""
echo "Testing: Stage 15 Tuned (Day 8) vs Stage 14 Final"
echo "Expected gain: +30-50 ELO (SEE improvement)"
echo "Time control: 10+0.1 (fast)"
echo "Opening book: 4moves_test.pgn"
echo ""

# Configuration
ENGINE_TEST="/workspace/binaries/seajay_stage15_tuned_fixed"
ENGINE_BASE="/workspace/binaries/seajay-stage14-final"
OPENING_BOOK="/workspace/external/books/4moves_test.pgn"
OUTPUT_DIR="/workspace/sprt_results/stage15_tuned_vs_stage14"

# SPRT parameters
ELO0=0      # No regression allowed
ELO1=30     # Looking for +30 ELO minimum (same as original test)
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
echo "  Stage 15 Tuned: $(md5sum $ENGINE_TEST | awk '{print $1}')"
echo "  Stage 14 Final: $(md5sum $ENGINE_BASE | awk '{print $1}')"
echo ""

# Display tuned parameters for reference
echo "Stage 15 Tuned Parameters:"
echo "  SEE Margins: Conservative -100, Aggressive -75, Endgame -25"
echo "  Piece Values: P=100, N=320, B=330, R=500, Q=950, K=20000"
echo ""

# Run SPRT test
echo "Starting SPRT test..."
echo "Press Ctrl+C to stop early"
echo ""

# Use fast-chess for SPRT
"$FASTCHESS" \
    -engine cmd="$ENGINE_TEST" name=Stage15-Tuned \
        option.SEEMode=production option.SEEPruning=aggressive \
    -engine cmd="$ENGINE_BASE" name=Stage14-Final \
    -each tc="$TC" \
    -openings file="$OPENING_BOOK" format=pgn order=random \
    -games 2 -rounds 10000 -repeat -concurrency "$CONCURRENCY" \
    -sprt elo0="$ELO0" elo1="$ELO1" alpha="$ALPHA" beta="$BETA" \
    -pgnout file="$OUTPUT_DIR/games.pgn" \
    -log file="$OUTPUT_DIR/fastchess.log" level=info \
    -recover \
    -event "Stage 15 Tuned vs Stage 14" \
    -site "SPRT Final Validation"

# Save results
echo ""
echo "Test complete!"
echo "Results saved to: $OUTPUT_DIR"
echo ""
echo "To view results:"
echo "  cat $OUTPUT_DIR/fastchess.log | tail -20"
echo "  grep 'LLR\\|Elo:' $OUTPUT_DIR/fastchess.log | tail -5"
echo ""
echo "Compare with original Stage 15 results:"
echo "  Original: +36.18 ± 24.93 Elo, LOS 99.80%"