#!/bin/bash
# External Validation: Stage 15 Final (Tuned) vs Stash v10 - From Starting Position
# Purpose: Test if SEE provides real strength against external engine from startpos

echo "============================================================"
echo "External Validation: Stage 15 Final (Tuned) vs Stash v10"
echo "From STARTING POSITION (no opening book)"
echo "============================================================"
echo ""

# Binary paths
STAGE15_BINARY="/workspace/binaries/seajay-stage15-properly-fixed"
STASH_BINARY="/workspace/external/engines/stash-bot/v10/stash-10.0-linux-x86_64"

# Expected checksums
STAGE15_EXPECTED_MD5="1364e4c6b35e19d71b07882e9fd08424"

# Test configuration
OUTPUT_DIR="/workspace/sprt_results/external_stage15_validation/stage15_vs_stash_startpos_$(date +%Y%m%d_%H%M%S)"
TC="10+0.1"  # 10 seconds + 0.1 increment
CONCURRENCY=1
GAMES=2000  # Fixed game count for comparison

# Create output directory
mkdir -p "$OUTPUT_DIR"

echo "Test Configuration:"
echo "==================="
echo "Stage 15 Binary: $STAGE15_BINARY (Tuned)"
echo "Stash Binary: $STASH_BINARY (1620 Elo)"
echo "Opening: Starting position only"
echo "Time Control: $TC"
echo "Total Games: $GAMES"
echo "Output Directory: $OUTPUT_DIR"
echo ""

# Verify binaries exist
if [ ! -f "$STAGE15_BINARY" ]; then
    echo "ERROR: Stage 15 binary not found: $STAGE15_BINARY"
    exit 1
fi

if [ ! -f "$STASH_BINARY" ]; then
    echo "ERROR: Stash v10 binary not found: $STASH_BINARY"
    exit 1
fi

# Validate Stage 15 checksum
echo "Validating binary checksums..."
STAGE15_ACTUAL_MD5=$(md5sum "$STAGE15_BINARY" | cut -d' ' -f1)

echo "Stage 15 MD5: $STAGE15_ACTUAL_MD5"

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

# Check for fast-chess
FASTCHESS="/workspace/external/testers/fast-chess/fastchess"
if [ ! -f "$FASTCHESS" ]; then
    echo "fast-chess not found, setting up..."
    /workspace/tools/scripts/setup-external-tools.sh
fi

echo "⚠️  WARNING: Testing from starting position only!"
echo "================================================"
echo "Stage 15 has the White bias bug (inherited from Stage 10)"
echo "Stage 15 tuning showed +29 Elo from startpos vs Stage 14"
echo "Testing if this improvement holds against external engine"
echo ""

echo "Reference Engine: Stash v10"
echo "============================"
echo "- Estimated Elo: 1620"
echo "- Well-calibrated reference engine"
echo "- No shared bugs with SeaJay"
echo ""

echo "Stage 15 Features (vs Stage 14):"
echo "================================"
echo "✓ SEE (Static Exchange Evaluation) for move ordering"
echo "✓ SEE-based pruning in quiescence search"
echo "✓ Tuned piece values: N=320, B=330, R=500, Q=950"
echo "✓ Tuned SEE margins: Conservative=-100, Aggressive=-75"
echo ""

echo "Critical Question:"
echo "=================="
echo "Stage 15 showed +29 Elo vs Stage 14 from startpos."
echo "Does this translate to improvement vs external engine?"
echo ""

echo "Starting external validation test..."
echo "This will play $GAMES games from startpos"
echo "Press Ctrl+C to stop early (progress will be saved)"
echo ""

# Run fixed game count test from startpos
"$FASTCHESS" \
    -engine cmd="$STAGE15_BINARY" name=Stage15-Final \
    -engine cmd="$STASH_BINARY" name=Stash-v10 \
    -each tc="$TC" \
    -games 2 -rounds $((GAMES/2)) -repeat -concurrency "$CONCURRENCY" \
    -pgnout file="$OUTPUT_DIR/games.pgn" \
    -log file="$OUTPUT_DIR/fastchess.log" level=info \
    -recover \
    -event "Stage 15 vs Stash v10 External Validation (Startpos)" \
    -site "External Validation from Starting Position"

echo ""
echo "============================================================"
echo "Test complete!"
echo "Results saved to: $OUTPUT_DIR"
echo ""
echo "To view results:"
echo "  tail -20 $OUTPUT_DIR/fastchess.log"
echo "  grep 'Elo\|Score' $OUTPUT_DIR/fastchess.log | tail -5"
echo ""
echo "Compare with Stage 14 results to determine if SEE helps"
echo "If Stage 15 ≈ Stage 14 vs Stash, SEE may only affect"
echo "internal dynamics, not real playing strength"
echo "============================================================"