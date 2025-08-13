#!/bin/bash
# SPRT Test: Stage 11 WITH MVV-LVA vs Stage 11 WITHOUT MVV-LVA (Ablation Test)
# Testing hypothesis: MVV-LVA provides +50 Elo improvement
# This is an ablation test to isolate the MVV-LVA contribution

# Get the directory this script is in
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../../.." && pwd)"

# Binary paths
WITH_MVV_BINARY="$PROJECT_ROOT/bin/seajay_stage11_with_mvv"
WITHOUT_MVV_BINARY="$PROJECT_ROOT/bin/seajay_stage11_without_mvv"

# Fast-chess location
FASTCHESS="$PROJECT_ROOT/external/testers/fast-chess/fastchess"

# Output directory with timestamp
TIMESTAMP=$(date +"%Y%m%d_%H%M%S")
OUTPUT_DIR="$PROJECT_ROOT/sprt_results/SPRT-Stage11-MVV-Ablation-$TIMESTAMP"
mkdir -p "$OUTPUT_DIR"

# Test configuration
TIME_CONTROL="10+0.1"  # 10 seconds + 0.1 second increment

# SPRT parameters
# Testing for MVV-LVA specific gain: +50 Elo
ALPHA="0.05"      # Type I error rate
BETA="0.05"       # Type II error rate  
ELO0="0"          # Null hypothesis: no improvement
ELO1="50"         # Alternative hypothesis: +50 Elo improvement from MVV-LVA

# Opening book - use 4moves_test.pgn as requested
OPENING_BOOK="$PROJECT_ROOT/external/books/4moves_test.pgn"

# Build the binaries first
echo "Building Stage 11 WITH MVV-LVA..."
cd "$PROJECT_ROOT/build"
rm -rf *
cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS="-DENABLE_MVV_LVA" > /dev/null 2>&1
make -j$(nproc) seajay > /dev/null 2>&1
if [ $? -ne 0 ]; then
    echo "Error: Failed to build Stage 11 with MVV-LVA"
    exit 1
fi
cp seajay "$WITH_MVV_BINARY"

echo "Building Stage 11 WITHOUT MVV-LVA..."
rm -rf *
cmake .. -DCMAKE_BUILD_TYPE=Release > /dev/null 2>&1
make -j$(nproc) seajay > /dev/null 2>&1
if [ $? -ne 0 ]; then
    echo "Error: Failed to build Stage 11 without MVV-LVA"
    exit 1
fi
cp seajay "$WITHOUT_MVV_BINARY"

cd "$PROJECT_ROOT"

# Verify binaries exist
if [ ! -f "$WITH_MVV_BINARY" ]; then
    echo "Error: Stage 11 WITH MVV-LVA binary not found at $WITH_MVV_BINARY"
    exit 1
fi

if [ ! -f "$WITHOUT_MVV_BINARY" ]; then
    echo "Error: Stage 11 WITHOUT MVV-LVA binary not found at $WITHOUT_MVV_BINARY"
    exit 1
fi

if [ ! -f "$FASTCHESS" ]; then
    echo "Error: fast-chess not found at $FASTCHESS"
    echo "Please run: $PROJECT_ROOT/tools/scripts/setup-external-tools.sh"
    exit 1
fi

if [ ! -f "$OPENING_BOOK" ]; then
    echo "Error: Opening book not found at $OPENING_BOOK"
    echo "Please ensure external books are set up"
    exit 1
fi

# Test that both engines respond to UCI
echo "Testing engine responsiveness..."
echo -e "uci\nquit" | timeout 2 "$WITH_MVV_BINARY" > /dev/null 2>&1
if [ $? -ne 0 ]; then
    echo "Error: Stage 11 WITH MVV-LVA binary not responding to UCI"
    exit 1
fi

echo -e "uci\nquit" | timeout 2 "$WITHOUT_MVV_BINARY" > /dev/null 2>&1
if [ $? -ne 0 ]; then
    echo "Error: Stage 11 WITHOUT MVV-LVA binary not responding to UCI"
    exit 1
fi

echo "==================================================================="
echo "SPRT Ablation Test: Stage 11 WITH MVV-LVA vs WITHOUT MVV-LVA"
echo "==================================================================="
echo "Testing for +50 Elo gain from MVV-LVA feature"
echo "Time Control: $TIME_CONTROL"
echo "SPRT: elo0=$ELO0, elo1=$ELO1, alpha=$ALPHA, beta=$BETA"
echo "Opening Book: 4moves_test.pgn"
echo "Output: $OUTPUT_DIR"
echo "==================================================================="

# Run SPRT test
"$FASTCHESS" \
    -engine name="Stage11-WITH-MVV-LVA" cmd="$WITH_MVV_BINARY" \
    -engine name="Stage11-WITHOUT-MVV-LVA" cmd="$WITHOUT_MVV_BINARY" \
    -each tc="$TIME_CONTROL" proto=uci \
    -openings file="$OPENING_BOOK" format=pgn order=random \
    -games 2 \
    -repeat \
    -recover \
    -sprt elo0="$ELO0" elo1="$ELO1" alpha="$ALPHA" beta="$BETA" \
    -ratinginterval 10 \
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
    tail -20 "$OUTPUT_DIR/sprt_output.txt" | grep -E "Elo|LLR|Games|Score|Result"
fi

echo "==================================================================="