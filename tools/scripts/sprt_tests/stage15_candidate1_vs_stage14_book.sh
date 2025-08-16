#!/bin/bash
# SPRT Test: Stage 15 Candidate 1 (NO TUNING) vs Stage 14 Final - With Opening Book
# Purpose: Baseline comparison without tuning influence

echo "============================================================"
echo "SPRT Test: Stage 15 Candidate 1 (NO TUNING) vs Stage 14 Final"
echo "Using 4moves_test.pgn opening book"
echo "============================================================"
echo ""

# Binary paths
STAGE15_BINARY="/workspace/binaries/seajay-stage15-original"  # Stage 15 Candidate 1 (no tuning)
STAGE14_BINARY="/workspace/binaries/seajay-stage14-final"

# Expected checksums for validation
STAGE15_EXPECTED_MD5="fcf0edacd7cec502cbfd982e69d201be"  # Stage 15 Candidate 1
STAGE14_EXPECTED_MD5="1c65de0c2e95cb371e0d637368f4d60d"  # Stage 14 Final

# Test configuration
OUTPUT_DIR="/workspace/sprt_results/stage15_candidate1_vs_stage14_book_$(date +%Y%m%d_%H%M%S)"
OPENING_BOOK="/workspace/external/books/4moves_test.pgn"
TC="10+0.1"  # 10 seconds + 0.1 increment
CONCURRENCY=1

# SPRT parameters
ELO0=0      # H0: No improvement
ELO1=30     # H1: +30 Elo improvement (SEE benefit without tuning)
ALPHA=0.05  # Type I error rate
BETA=0.05   # Type II error rate

# Create output directory
mkdir -p "$OUTPUT_DIR"

echo "Test Configuration:"
echo "==================="
echo "Stage 15 Binary: $STAGE15_BINARY (Candidate 1 - NO TUNING)"
echo "Stage 14 Binary: $STAGE14_BINARY"
echo "Opening Book: $OPENING_BOOK"
echo "Time Control: $TC"
echo "SPRT Bounds: [$ELO0, $ELO1]"
echo "Output Directory: $OUTPUT_DIR"
echo ""

# Verify binaries exist
if [ ! -f "$STAGE15_BINARY" ]; then
    echo "ERROR: Stage 15 Candidate 1 binary not found: $STAGE15_BINARY"
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

echo "Stage 15 Candidate 1 MD5: $STAGE15_ACTUAL_MD5"
echo "Stage 14 Final MD5: $STAGE14_ACTUAL_MD5"

if [ "$STAGE15_ACTUAL_MD5" != "$STAGE15_EXPECTED_MD5" ]; then
    echo "WARNING: Stage 15 Candidate 1 checksum mismatch!"
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

# Check for opening book
if [ ! -f "$OPENING_BOOK" ]; then
    echo "ERROR: Opening book not found: $OPENING_BOOK"
    echo "Please ensure the opening book exists."
    exit 1
fi

# Check for fast-chess
FASTCHESS="/workspace/external/testers/fast-chess/fastchess"
if [ ! -f "$FASTCHESS" ]; then
    echo "fast-chess not found, setting up..."
    /workspace/tools/scripts/setup-external-tools.sh
fi

echo "Stage 15 Candidate 1 Features:"
echo "=============================="
echo "✓ SEE (Static Exchange Evaluation) for move ordering"
echo "✓ SEE-based pruning in quiescence search"
echo "✗ NO TUNING - Using default piece values"
echo "✗ NO PST ADJUSTMENTS - Original Stage 15 implementation"
echo ""
echo "This test establishes baseline SEE performance without tuning"
echo ""

echo "Starting SPRT test with opening book..."
echo "Press Ctrl+C to stop early (progress will be saved)"
echo ""

# Run SPRT test with opening book
"$FASTCHESS" \
    -engine cmd="$STAGE15_BINARY" name=Stage15-Candidate1 \
    -engine cmd="$STAGE14_BINARY" name=Stage14-Final \
    -each tc="$TC" \
    -games 2 -rounds 10000 -repeat -concurrency "$CONCURRENCY" \
    -sprt elo0="$ELO0" elo1="$ELO1" alpha="$ALPHA" beta="$BETA" \
    -openings file="$OPENING_BOOK" format=pgn order=random plies=8 \
    -pgnout file="$OUTPUT_DIR/games.pgn" \
    -log file="$OUTPUT_DIR/fastchess.log" level=info \
    -recover \
    -event "Stage 15 Candidate 1 vs Stage 14 Final (Book)" \
    -site "SPRT with 4moves Opening Book"

echo ""
echo "============================================================"
echo "Test complete!"
echo "Results saved to: $OUTPUT_DIR"
echo ""
echo "To view results:"
echo "  tail -20 $OUTPUT_DIR/fastchess.log"
echo "  grep 'Elo\|LLR\|Score' $OUTPUT_DIR/fastchess.log | tail -5"
echo ""
echo "Expected outcome: Stage 15 Candidate 1 should show baseline"
echo "                  SEE improvement without tuning influence"
echo "============================================================"