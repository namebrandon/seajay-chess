#!/bin/bash
# External Validation: Stage 14 Final vs Stash v10 - With Opening Book
# Purpose: Establish baseline performance against external engine

echo "============================================================"
echo "External Validation: Stage 14 Final vs Stash v10"
echo "Using 4moves_test.pgn opening book"
echo "============================================================"
echo ""

# Binary paths
STAGE14_BINARY="/workspace/binaries/seajay-stage14-final"
STASH_BINARY="/workspace/external/engines/stash-bot/v10/stash-10.0-linux-x86_64"

# Expected checksums
STAGE14_EXPECTED_MD5="1c65de0c2e95cb371e0d637368f4d60d"

# Test configuration
OUTPUT_DIR="/workspace/sprt_results/external_stage15_validation/stage14_vs_stash_book_$(date +%Y%m%d_%H%M%S)"
OPENING_BOOK="/workspace/external/books/4moves_test.pgn"
TC="10+0.1"  # 10 seconds + 0.1 increment
CONCURRENCY=1
GAMES=2000  # Fixed game count for comparison

# Create output directory
mkdir -p "$OUTPUT_DIR"

echo "Test Configuration:"
echo "==================="
echo "Stage 14 Binary: $STAGE14_BINARY"
echo "Stash Binary: $STASH_BINARY (1620 Elo)"
echo "Opening Book: $OPENING_BOOK"
echo "Time Control: $TC"
echo "Total Games: $GAMES"
echo "Output Directory: $OUTPUT_DIR"
echo ""

# Verify binaries exist
if [ ! -f "$STAGE14_BINARY" ]; then
    echo "ERROR: Stage 14 binary not found: $STAGE14_BINARY"
    exit 1
fi

if [ ! -f "$STASH_BINARY" ]; then
    echo "ERROR: Stash v10 binary not found: $STASH_BINARY"
    exit 1
fi

# Validate Stage 14 checksum
echo "Validating binary checksums..."
STAGE14_ACTUAL_MD5=$(md5sum "$STAGE14_BINARY" | cut -d' ' -f1)

echo "Stage 14 MD5: $STAGE14_ACTUAL_MD5"

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

# Check for opening book
if [ ! -f "$OPENING_BOOK" ]; then
    echo "ERROR: Opening book not found: $OPENING_BOOK"
    exit 1
fi

# Check for fast-chess
FASTCHESS="/workspace/external/testers/fast-chess/fastchess"
if [ ! -f "$FASTCHESS" ]; then
    echo "fast-chess not found, setting up..."
    /workspace/tools/scripts/setup-external-tools.sh
fi

echo "Reference Engine: Stash v10"
echo "============================"
echo "- Estimated Elo: 1620"
echo "- Well-calibrated reference engine"
echo "- Widely used in chess engine community"
echo ""

echo "Stage 14 Features:"
echo "=================="
echo "✓ Quiescence search with captures"
echo "✓ MVV-LVA move ordering"
echo "✓ Transposition table"
echo "✓ Iterative deepening"
echo "✓ Time management"
echo ""

echo "Starting external validation test..."
echo "This will play $GAMES games to establish baseline"
echo "Press Ctrl+C to stop early (progress will be saved)"
echo ""

# Run fixed game count test with opening book
"$FASTCHESS" \
    -engine cmd="$STAGE14_BINARY" name=Stage14-Final \
    -engine cmd="$STASH_BINARY" name=Stash-v10 \
    -each tc="$TC" \
    -games 2 -rounds $((GAMES/2)) -repeat -concurrency "$CONCURRENCY" \
    -openings file="$OPENING_BOOK" format=pgn order=random plies=8 \
    -pgnout file="$OUTPUT_DIR/games.pgn" \
    -log file="$OUTPUT_DIR/fastchess.log" level=info \
    -recover \
    -event "Stage 14 vs Stash v10 External Validation (Book)" \
    -site "External Validation with 4moves Book"

echo ""
echo "============================================================"
echo "Test complete!"
echo "Results saved to: $OUTPUT_DIR"
echo ""
echo "To view results:"
echo "  tail -20 $OUTPUT_DIR/fastchess.log"
echo "  grep 'Elo\|Score' $OUTPUT_DIR/fastchess.log | tail -5"
echo ""
echo "This establishes Stage 14 baseline against Stash v10"
echo "Compare with Stage 15 results to determine SEE value"
echo "============================================================"