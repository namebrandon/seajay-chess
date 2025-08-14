#!/bin/bash
# SPRT Test: Stage 11 (MVV-LVA) vs Stage 10 (Magic Bitboards)
# DRAWKILLER + 60 SECOND TIME CONTROL TEST
# Testing hypothesis: MVV-LVA benefits in tactical positions at deeper search depths
# Combining sharp positions with deep search should maximize MVV-LVA advantages

# Get the directory this script is in
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$SCRIPT_DIR"

# Binary paths - using the binaries in the binaries directory
# Updated to use SPRT candidate 2 with performance fixes
STAGE11_BINARY="$PROJECT_ROOT/binaries/seajay-stage11-candidate2-x86-64"
STAGE10_BINARY="$PROJECT_ROOT/binaries/seajay-stage10-x86-64"

# Fast-chess location
FASTCHESS="$PROJECT_ROOT/external/testers/fast-chess/fastchess"

# Output directory with timestamp - clearly marked as Drawkiller 60s test with v2
TIMESTAMP=$(date +"%Y%m%d_%H%M%S")
OUTPUT_DIR="$PROJECT_ROOT/sprt_results/SPRT-Stage11v2-vs-Stage10-Drawkiller-60s-$TIMESTAMP"
mkdir -p "$OUTPUT_DIR"

# Test configuration - LONGER TIME CONTROL
TIME_CONTROL="60+0.6"  # 60 seconds + 0.6 second increment (10x longer than previous)
THREADS="1"

# SPRT parameters for Phase 2
# More optimistic for tactical positions at depth
ALPHA="0.05"      # Type I error rate (5% false positive)
BETA="0.05"       # Type II error rate (5% false negative)
ELO0="0"          # Null hypothesis: no improvement
ELO1="40"         # Alternative hypothesis: +40 Elo (higher for tactical + deep)

# Opening book - Drawkiller for sharp tactical positions
# This book has 15,962 positions and ~63% draw rate
OPENING_BOOK="$PROJECT_ROOT/external/books/Drawkiller_balanced_big.epd"

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
    echo "Error: Drawkiller opening book not found at $OPENING_BOOK"
    echo "Please ensure external books are set up"
    exit 1
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
echo "           DRAWKILLER + 60 SECOND TIME CONTROL"
echo "==================================================================="
echo "Testing: FIXED MVV-LVA in tactical positions + deep search"
echo ""
echo "Previous results with candidate 1:"
echo "  10+0.1 with 4moves: -2.9 Elo"
echo "  10+0.1 with Drawkiller: -8.7 Elo"
echo "  60+0.6: -10 Elo (allocation overhead dominated)"
echo ""
echo "Candidate 2 fixes:"
echo "  - In-place sorting eliminates heap allocation"
echo "  - Only sorts captures (preserves quiet move order)"
echo "  - No thread-local overhead"
echo ""
echo "Hypothesis for 60+0.6 with Drawkiller:"
echo "  - Deep tactical searches (depth 10-14+)"
echo "  - Many captures to order in sharp positions"
echo "  - MVV-LVA should excel in these conditions"
echo "  - Overhead amortized over millions of nodes"
echo ""
echo "Engines:"
echo "  Test:  Stage 11 - Magic Bitboards + MVV-LVA"
echo "  Base:  Stage 10 - Magic Bitboards only"
echo ""
echo "Parameters:"
echo "  Time Control: $TIME_CONTROL (10x longer than previous tests)"
echo "  SPRT: elo0=$ELO0, elo1=$ELO1, alpha=$ALPHA, beta=$BETA"
echo "  Opening Book: Drawkiller_balanced_big.epd"
echo "  Book Positions: 15,962 sharp tactical positions"
echo "  Expected Draw Rate: ~63% (lower than standard books)"
echo "  Output: $OUTPUT_DIR"
echo ""
echo "Why this combination should work:"
echo "  1. Tactical positions = more captures to order"
echo "  2. Deep search = overhead amortized"
echo "  3. Sharp play = good move ordering more critical"
echo "  4. Opposite-side castling = attacking races favor MVV-LVA"
echo "==================================================================="
echo ""
echo "Starting SPRT test..."
echo "  - Max rounds: 2000 (4000 games)"
echo "  - Concurrency: 1 (sequential games)"
echo "  - SPRT will stop when reaching statistical decision"
echo "  - Estimated time: 24-48 hours at 60+0.6 time control"
echo "  - With 15,962 positions, no game repetition expected"
echo ""
echo "Press Ctrl+C to stop early if needed"
echo ""

# Build fast-chess command
# Note: Fewer max rounds due to much longer games
# EPD format requires format=epd instead of format=pgn
"$FASTCHESS" \
    -engine name="Stage11-MVV-LVA-v2-DK60s" cmd="$STAGE11_BINARY" \
    -engine name="Stage10-Magic-DK60s" cmd="$STAGE10_BINARY" \
    -each tc="$TIME_CONTROL" proto=uci \
    -rounds 2000 \
    -concurrency 1 \
    -recover \
    -sprt elo0="$ELO0" elo1="$ELO1" alpha="$ALPHA" beta="$BETA" \
    -ratinginterval 2 \
    -openings file="$OPENING_BOOK" format=epd order=random \
    -pgnout "$OUTPUT_DIR/games.pgn" fi \
    -log file="$OUTPUT_DIR/fastchess.log" level=info \
    | tee "$OUTPUT_DIR/sprt_output.txt"

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
echo "PASS: MVV-LVA excels in deep tactical positions (≥$ELO1 Elo)"
echo "      Validates that implementation works, just needs depth"
echo "FAIL: MVV-LVA doesn't help even in ideal conditions"
echo "      Suggests fundamental implementation or design issue"
echo ""
echo "This is the ultimate test for MVV-LVA:"
echo "  - If it fails here, the implementation needs rework"
echo "  - If it passes here but not in quiet positions, it's specialized"
echo "==================================================================="

# Save test summary with Drawkiller-specific information
cat > "$OUTPUT_DIR/test_summary.txt" << EOF
SPRT Test: Stage 11 vs Stage 10 (Drawkiller + 60s)
===================================================
Date: $(date)
Test Engine: Stage 11 (MVV-LVA)
Base Engine: Stage 10 (Magic Bitboards)

Test Configuration:
- Time Control: 60+0.6 (10x longer than initial tests)
- Opening Book: Drawkiller_balanced_big.epd
- Book Size: 15,962 sharp tactical positions
- Expected Search Depth: 10-14+ ply
- Draw Rate: ~63% (vs 85% in quiet positions)

Hypothesis:
Combining tactical positions with deep search should maximize MVV-LVA benefits.
Previous tests showed negative results at shallow depths (6-8 ply).
At depth 10+, in tactical positions, MVV-LVA should excel.

SPRT Parameters: elo0=$ELO0, elo1=$ELO1, alpha=$ALPHA, beta=$BETA

Why This Should Work:
1. Tactical positions have many captures to order
2. Deep search amortizes sorting overhead
3. Sharp positions make move ordering critical
4. Opposite-side castling creates attacking races

Previous Results Summary:
- 10+0.1 with 4moves: -2.9 Elo
- 10+0.1 with Drawkiller: -8.7 Elo
- Both at depth 6-8 where overhead > benefit

Stage 11 Features:
- MVV-LVA move ordering for captures
- Expected 15-30% node reduction at depth 10+
- Should excel in tactical positions

Stage 10 Features:
- Magic bitboards for move generation
- Natural move ordering from generator

Results:
See sprt_output.txt for detailed results
EOF

echo ""
echo "Test summary saved to: $OUTPUT_DIR/test_summary.txt"
echo ""
echo "Note: This test will take much longer (~24-48 hours)"
echo "      Each game takes ~2-3 minutes at 60+0.6"
echo ""
echo "Comparison:"
echo "Run both 60s tests (4moves and Drawkiller) to compare:"
echo "  - Quiet vs tactical positions at depth"
echo "  - Whether MVV-LVA is universally helpful or position-dependent"