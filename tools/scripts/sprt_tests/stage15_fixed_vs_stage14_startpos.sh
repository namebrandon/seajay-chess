#!/bin/bash
# Stage 15 Fixed vs Stage 14 SPRT Test - From Starting Position
# Tests Stage 15 with bug fix against Stage 14 baseline from startpos
# Expected: +30-50 ELO improvement from SEE implementation

echo "================================================"
echo "Stage 15 Fixed vs Stage 14 SPRT Test (STARTPOS)"
echo "================================================"
echo ""
echo "Testing: Stage 15 Fixed (PST bug resolved) vs Stage 14 Final"
echo "Expected gain: +30-50 ELO (SEE improvement)"
echo "Time control: 10+0.1 (fast)"
echo "Opening: Starting position only"
echo ""

# Configuration
ENGINE_TEST="/workspace/binaries/seajay-stage15-fixed"
ENGINE_BASE="/workspace/binaries/seajay-stage14-final"
OUTPUT_DIR="/workspace/sprt_results/stage15_fixed_vs_stage14_startpos"

# SPRT parameters
ELO0=0      # No regression allowed
ELO1=30     # Looking for +30 ELO minimum
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

# Check for fast-chess
FASTCHESS="/workspace/external/testers/fast-chess/fastchess"
if [ ! -f "$FASTCHESS" ]; then
    echo "fast-chess not found, downloading..."
    /workspace/tools/scripts/setup-external-tools.sh
fi

# Expected checksums (update these if binaries change)
EXPECTED_STAGE15_MD5="e2e2ea6b41eab6c353cc116539f216ed"
EXPECTED_STAGE14_MD5="1c65de0c2e95cb371e0d637368f4d60d"

# Display and verify MD5 checksums
echo "Binary checksums:"
ACTUAL_STAGE15_MD5=$(md5sum $ENGINE_TEST | awk '{print $1}')
ACTUAL_STAGE14_MD5=$(md5sum $ENGINE_BASE | awk '{print $1}')

echo "  Stage 15 Fixed: $ACTUAL_STAGE15_MD5"
echo "  Stage 14 Final: $ACTUAL_STAGE14_MD5"
echo ""

# Verify checksums
CHECKSUM_VALID=true

if [ "$ACTUAL_STAGE15_MD5" != "$EXPECTED_STAGE15_MD5" ]; then
    echo "WARNING: Stage 15 Fixed checksum mismatch!"
    echo "  Expected: $EXPECTED_STAGE15_MD5"
    echo "  Actual:   $ACTUAL_STAGE15_MD5"
    CHECKSUM_VALID=false
fi

if [ -n "$EXPECTED_STAGE14_MD5" ] && [ "$ACTUAL_STAGE14_MD5" != "$EXPECTED_STAGE14_MD5" ]; then
    echo "WARNING: Stage 14 Final checksum mismatch!"
    echo "  Expected: $EXPECTED_STAGE14_MD5"
    echo "  Actual:   $ACTUAL_STAGE14_MD5"
    CHECKSUM_VALID=false
fi

if [ "$CHECKSUM_VALID" = false ]; then
    echo ""
    echo "Binary checksums do not match expected values."
    read -p "Continue anyway? (y/n): " -n 1 -r
    echo
    if [[ ! $REPLY =~ ^[Yy]$ ]]; then
        echo "Aborting test."
        exit 1
    fi
else
    echo "Binary validation passed ✓"
fi
echo ""

# Display Stage 15 changes for reference
echo "Stage 15 Fixed Changes:"
echo "  ✓ PST double-negation bug fixed (290 cp error eliminated)"
echo "  ✓ SEE implementation for better move ordering"
echo "  ✓ Tuned parameters: P=100, N=320, B=330, R=500, Q=950"
echo "  ✓ SEE Margins: Conservative -100, Aggressive -75, Endgame -25"
echo ""

# Run SPRT test from starting position
echo "Starting SPRT test from starting position..."
echo "This tests the engine's ability from a single consistent position"
echo "Press Ctrl+C to stop early"
echo ""

# Use fast-chess for SPRT without opening book
"$FASTCHESS" \
    -engine cmd="$ENGINE_TEST" name=Stage15-Fixed \
        option.SEEMode=production option.SEEPruning=aggressive \
    -engine cmd="$ENGINE_BASE" name=Stage14-Final \
    -each tc="$TC" \
    -games 2 -rounds 10000 -repeat -concurrency "$CONCURRENCY" \
    -sprt elo0="$ELO0" elo1="$ELO1" alpha="$ALPHA" beta="$BETA" \
    -pgnout file="$OUTPUT_DIR/games.pgn" \
    -log file="$OUTPUT_DIR/fastchess.log" level=info \
    -recover \
    -event "Stage 15 Fixed vs Stage 14 STARTPOS" \
    -site "SPRT from Starting Position"

# Save results
echo ""
echo "Test complete!"
echo "Results saved to: $OUTPUT_DIR"
echo ""
echo "To view results:"
echo "  cat $OUTPUT_DIR/fastchess.log | tail -20"
echo "  grep 'LLR\\|Elo:' $OUTPUT_DIR/fastchess.log | tail -5"
echo ""
echo "Note: Testing from startpos only provides limited position diversity"
echo "      For more comprehensive testing, use the 4moves opening book"