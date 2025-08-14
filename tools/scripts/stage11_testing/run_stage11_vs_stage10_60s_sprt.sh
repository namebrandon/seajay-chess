#!/bin/bash
# SPRT Test: Stage 11 (MVV-LVA) vs Stage 10 (Magic Bitboards)
# LONGER TIME CONTROL TEST: 60+0.6 to test deeper search hypothesis
# Testing hypothesis: Stage 11 MVV-LVA benefits emerge at deeper search depths
# MVV-LVA overhead should be amortized at depths 10-14+

# Get the directory this script is in
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$SCRIPT_DIR"

# Binary paths - using the binaries in the binaries directory
# Updated to use SPRT candidate 2 with performance fixes
STAGE11_BINARY="$PROJECT_ROOT/binaries/seajay-stage11-candidate2-x86-64"
STAGE10_BINARY="$PROJECT_ROOT/binaries/seajay-stage10-x86-64"

# Fast-chess location
FASTCHESS="$PROJECT_ROOT/external/testers/fast-chess/fastchess"

# Output directory with timestamp - clearly marked as 60s test with v2
TIMESTAMP=$(date +"%Y%m%d_%H%M%S")
OUTPUT_DIR="$PROJECT_ROOT/sprt_results/SPRT-Stage11v2-vs-Stage10-60s-$TIMESTAMP"
mkdir -p "$OUTPUT_DIR"

# Test configuration - LONGER TIME CONTROL
TIME_CONTROL="60+0.6"  # 60 seconds + 0.6 second increment (10x longer than previous)
THREADS="1"

# SPRT parameters for Phase 2
# Same parameters but expecting better performance at depth
ALPHA="0.05"      # Type I error rate (5% false positive)
BETA="0.05"       # Type II error rate (5% false negative)
ELO0="0"          # Null hypothesis: no improvement
ELO1="30"         # Alternative hypothesis: +30 Elo improvement (more conservative for deeper search)

# Opening book - use 4moves_test.pgn as in original test
OPENING_BOOK="$PROJECT_ROOT/external/books/4moves_test.pgn"

# Verify binaries exist
if [ ! -f "$STAGE11_BINARY" ]; then
    echo "Error: Stage 11 binary not found at $STAGE11_BINARY"
    exit 1
fi

if [ ! -f "$STAGE10_BINARY" ]; then
    echo "Error: Stage 10 binary not found at $STAGE10_BINARY"
    exit 1
fi

if [ ! -f "$FASTCHESS" ]; then
    echo "Error: fast-chess not found at $FASTCHESS"
    echo "Please run: $PROJECT_ROOT/tools/scripts/setup-external-tools.sh"
    exit 1
fi

if [ ! -f "$OPENING_BOOK" ]; then
    echo "Warning: Opening book not found at $OPENING_BOOK"
    echo "Will run without opening book (from startpos)"
    OPENING_BOOK=""
fi

# Test that both engines respond to UCI
echo "Testing engine responsiveness..."
echo -e "uci\nquit" | timeout 2 "$STAGE11_BINARY" > /dev/null 2>&1
if [ $? -ne 0 ]; then
    echo "Error: Stage 11 binary not responding to UCI"
    exit 1
fi

echo -e "uci\nquit" | timeout 2 "$STAGE10_BINARY" > /dev/null 2>&1
if [ $? -ne 0 ]; then
    echo "Error: Stage 10 binary not responding to UCI"
    exit 1
fi

echo "==================================================================="
echo "SPRT Test: Stage 11 v2 (MVV-LVA Fixed) vs Stage 10 (Magic Bitboards)"
echo "           60 SECOND TIME CONTROL - DEEP SEARCH TEST"
echo "==================================================================="
echo "Testing FIXED MVV-LVA implementation at deeper search depths"
echo ""
echo "Previous results with candidate 1:"
echo "  - 10+0.1: -2.5 Elo regression"
echo "  - 60+0.6: -10 Elo regression (worse at depth!)"
echo ""
echo "Candidate 2 fixes applied:"
echo "  - In-place sorting (no heap allocation)"
echo "  - Only sorts captures, preserves quiet move order"
echo "  - Removed thread-local overhead"
echo ""
echo "Expected at 60+0.6:"
echo "  - Search depths 10-14+ ply"
echo "  - MVV-LVA overhead amortized over deeper search"
echo "  - Better move ordering should reduce node count significantly"
echo ""
echo "Engines:"
echo "  Test:  Stage 11 - Magic Bitboards + MVV-LVA"
echo "  Base:  Stage 10 - Magic Bitboards only"
echo ""
echo "Parameters:"
echo "  Time Control: $TIME_CONTROL (10x longer than previous tests)"
echo "  SPRT: elo0=$ELO0, elo1=$ELO1, alpha=$ALPHA, beta=$BETA"
if [ -n "$OPENING_BOOK" ]; then
    echo "  Opening Book: 4moves_test.pgn"
else
    echo "  Opening Book: None (starting position)"
fi
echo "  Output: $OUTPUT_DIR"
echo "==================================================================="
echo ""
echo "Starting SPRT test..."
echo "  - Max rounds: 2000 (4000 games)"
echo "  - Concurrency: 1 (sequential games)"
echo "  - SPRT will stop when reaching statistical decision"
echo "  - Estimated time: 24-48 hours at 60+0.6 time control"
echo "  - Each game takes ~2-3 minutes"
echo ""
echo "Press Ctrl+C to stop early if needed"
echo ""

# Build fast-chess command
# Note: Fewer max rounds due to much longer games
FASTCHESS_CMD="$FASTCHESS \
    -engine name=\"Stage11-MVV-LVA-v2-60s\" cmd=\"$STAGE11_BINARY\" \
    -engine name=\"Stage10-Magic-60s\" cmd=\"$STAGE10_BINARY\" \
    -each tc=\"$TIME_CONTROL\" proto=uci \
    -rounds 2000 \
    -concurrency 1 \
    -recover \
    -sprt elo0=\"$ELO0\" elo1=\"$ELO1\" alpha=\"$ALPHA\" beta=\"$BETA\" \
    -ratinginterval 2 \
    -pgnout \"$OUTPUT_DIR/games.pgn\" fi \
    -log file=\"$OUTPUT_DIR/fastchess.log\" level=info"

# Add opening book if available
if [ -n "$OPENING_BOOK" ]; then
    FASTCHESS_CMD="$FASTCHESS_CMD -openings file=\"$OPENING_BOOK\" format=pgn order=random"
fi

# Run SPRT test and save output
eval "$FASTCHESS_CMD" | tee "$OUTPUT_DIR/sprt_output.txt"

# Parse results
echo ""
echo "==================================================================="
echo "Test Complete!"
echo "Results saved to: $OUTPUT_DIR"
echo ""

# Extract final statistics
if [ -f "$OUTPUT_DIR/sprt_output.txt" ]; then
    echo "Final Statistics:"
    echo "-----------------"
    tail -30 "$OUTPUT_DIR/sprt_output.txt" | grep -E "Elo|LLR|Games|Score|Result|SPRT"
fi

echo ""
echo "==================================================================="
echo "Interpretation:"
echo "---------------"
echo "PASS: MVV-LVA provides improvement at deeper search depths"
echo "      Overhead is amortized, move ordering benefits dominate"
echo "FAIL: MVV-LVA doesn't help even at deeper search"
echo "      May need to optimize implementation or wait for other heuristics"
echo "==================================================================="

# Save test summary
cat > "$OUTPUT_DIR/test_summary.txt" << EOF
SPRT Test: Stage 11 vs Stage 10 (60 Second Time Control)
=========================================================
Date: $(date)
Test Engine: Stage 11 (MVV-LVA)
Base Engine: Stage 10 (Magic Bitboards)

Time Control: 60+0.6 (10x longer than initial tests)
Expected Search Depth: 10-14+ ply

Hypothesis:
MVV-LVA showed -2.5 Elo at 10+0.1 time control (depth 6-8).
Testing whether benefits emerge at deeper search depths where:
- Overhead is amortized over more nodes
- Better move ordering has more impact
- Alpha-beta pruning benefits compound

SPRT Parameters: elo0=$ELO0, elo1=$ELO1, alpha=$ALPHA, beta=$BETA

Stage 11 Features:
- MVV-LVA move ordering for captures
- Expected 15-30% node reduction at depth 10+
- Better alpha-beta pruning efficiency

Stage 10 Features:
- Magic bitboards for move generation
- Natural move ordering from generator

Previous Results (10+0.1):
- Elo: -2.5 to -2.9
- Draw Rate: 85%
- LLR: Trending toward FAIL

Results:
See sprt_output.txt for detailed results
EOF

echo ""
echo "Test summary saved to: $OUTPUT_DIR/test_summary.txt"
echo ""
echo "Note: This test will take much longer (~24-48 hours)"
echo "      Each game takes ~2-3 minutes at 60+0.6"