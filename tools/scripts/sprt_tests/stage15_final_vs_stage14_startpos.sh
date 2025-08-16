#!/bin/bash
# SPRT Test: Stage 15 Final (Tuned) vs Stage 14 Final - From Starting Position
# Expected: +30-40 Elo improvement from SEE implementation

echo "============================================================"
echo "SPRT Test: Stage 15 Final vs Stage 14 Final"
echo "From STARTING POSITION (no opening book)"
echo "============================================================"
echo ""

# Binary paths
STAGE15_BINARY="/workspace/binaries/seajay-stage15-properly-fixed"
STAGE14_BINARY="/workspace/binaries/seajay-stage14-final"

# Expected checksums for validation
STAGE15_EXPECTED_MD5="1364e4c6b35e19d71b07882e9fd08424"
STAGE14_EXPECTED_MD5="1c65de0c2e95cb371e0d637368f4d60d"

# Test configuration
OUTPUT_DIR="/workspace/sprt_results/stage15_final_startpos_$(date +%Y%m%d_%H%M%S)"
TC="10+0.1"  # 10 seconds + 0.1 increment
CONCURRENCY=1

# SPRT parameters
ELO0=0      # H0: No improvement
ELO1=30     # H1: +30 Elo improvement (SEE benefit)
ALPHA=0.05  # Type I error rate
BETA=0.05   # Type II error rate

# Create output directory
mkdir -p "$OUTPUT_DIR"

echo "Test Configuration:"
echo "==================="
echo "Stage 15 Binary: $STAGE15_BINARY"
echo "Stage 14 Binary: $STAGE14_BINARY"
echo "Opening: Starting position only"
echo "Time Control: $TC"
echo "SPRT Bounds: [$ELO0, $ELO1]"
echo "Output Directory: $OUTPUT_DIR"
echo ""

# Verify binaries exist
if [ ! -f "$STAGE15_BINARY" ]; then
    echo "ERROR: Stage 15 binary not found: $STAGE15_BINARY"
    exit 1
fi

if [ ! -f "$STAGE14_BINARY" ]; then
    echo "ERROR: Stage 14 binary not found: $STAGE14_BINARY"
    exit 1
fi

# Validate checksums
echo "Validating binary checksums..."
STAGE15_ACTUAL_MD5=$(md5sum "$STAGE15_BINARY" | cut -d' ' -f1)
STAGE14_ACTUAL_MD5=$(md5sum "$STAGE14_BINARY" | cut -d' ' -f1)

echo "Stage 15 MD5: $STAGE15_ACTUAL_MD5"
echo "Stage 14 MD5: $STAGE14_ACTUAL_MD5"

if [ "$STAGE15_ACTUAL_MD5" != "$STAGE15_EXPECTED_MD5" ]; then
    echo "WARNING: Stage 15 checksum mismatch!"
    echo "  Expected: $STAGE15_EXPECTED_MD5"
    echo "  Actual:   $STAGE15_ACTUAL_MD5"
    read -p "Continue anyway? (y/n): " -n 1 -r
    echo
    if [[ ! $REPLY =~ ^[Yy]$ ]]; then
        exit 1
    fi
fi

if [ "$STAGE14_ACTUAL_MD5" != "$STAGE14_EXPECTED_MD5" ]; then
    echo "WARNING: Stage 14 checksum mismatch!"
    echo "  Expected: $STAGE14_EXPECTED_MD5"
    echo "  Actual:   $STAGE14_ACTUAL_MD5"
    read -p "Continue anyway? (y/n): " -n 1 -r
    echo
    if [[ ! $REPLY =~ ^[Yy]$ ]]; then
        exit 1
    fi
fi

echo ""
echo "Checksums validated ✓"
echo ""

# Check for fast-chess
FASTCHESS="/workspace/external/testers/fast-chess/fastchess"
if [ ! -f "$FASTCHESS" ]; then
    echo "fast-chess not found, setting up..."
    /workspace/tools/scripts/setup-external-tools.sh
fi

echo "⚠️  WARNING: Testing from starting position only!"
echo "================================================"
echo "Both engines have the White bias bug (85-95% White wins)"
echo "This affects game diversity but both engines equally"
echo "The relative strength difference should still be ~+30 Elo"
echo ""

echo "Stage 15 Changes:"
echo "================="
echo "✓ SEE (Static Exchange Evaluation) for move ordering"
echo "✓ SEE-based pruning in quiescence search"
echo "✓ Tuned piece values: N=320, B=330, R=500, Q=950"
echo "✓ Tuned SEE margins: Conservative=-100, Aggressive=-75, Endgame=-25"
echo ""

echo "Starting SPRT test from starting position..."
echo "Press Ctrl+C to stop early (progress will be saved)"
echo ""

# Run SPRT test (no opening book)
"$FASTCHESS" \
    -engine cmd="$STAGE15_BINARY" name=Stage15-Final \
    -engine cmd="$STAGE14_BINARY" name=Stage14-Final \
    -each tc="$TC" \
    -games 2 -rounds 10000 -repeat -concurrency "$CONCURRENCY" \
    -sprt elo0="$ELO0" elo1="$ELO1" alpha="$ALPHA" beta="$BETA" \
    -pgnout file="$OUTPUT_DIR/games.pgn" \
    -log file="$OUTPUT_DIR/fastchess.log" level=info \
    -recover \
    -event "Stage 15 Final vs Stage 14 Final (Startpos)" \
    -site "SPRT from Starting Position"

echo ""
echo "============================================================"
echo "Test complete!"
echo "Results saved to: $OUTPUT_DIR"
echo ""
echo "To view results:"
echo "  tail -20 $OUTPUT_DIR/fastchess.log"
echo "  grep 'Elo\\|LLR\\|Score' $OUTPUT_DIR/fastchess.log | tail -5"
echo ""
echo "Expected outcome: Stage 15 should show +30-40 Elo improvement"
echo "Note: Lower game diversity due to startpos-only testing"
echo "      May take longer to converge than book-based test"
echo "============================================================"