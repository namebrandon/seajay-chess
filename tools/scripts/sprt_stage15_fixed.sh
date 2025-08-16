#!/bin/bash

# SPRT test for Stage 15 Fixed vs Stage 14 Final
# Testing if SEE implementation provides expected Elo gain

TIMESTAMP=$(date +%Y%m%d_%H%M%S)
RESULT_DIR="/workspace/sprt_results/stage15_fixed_vs_stage14_${TIMESTAMP}"

echo "================================================="
echo "SPRT Test: Stage 15 Fixed vs Stage 14 Final"
echo "================================================="
echo ""
echo "Testing binaries:"
echo "  New: /workspace/binaries/seajay-stage15-fixed"
echo "  Base: /workspace/binaries/seajay-stage14-final"
echo ""
echo "Binary checksums:"
echo -n "  Stage 15 Fixed: "
md5sum /workspace/binaries/seajay-stage15-fixed | cut -d' ' -f1
echo -n "  Stage 14 Final: "
md5sum /workspace/binaries/seajay-stage14-final | cut -d' ' -f1
echo ""
echo "Expected result: +30-40 Elo from SEE implementation"
echo "SPRT bounds: [0, 30] Elo"
echo ""

# Create result directory
mkdir -p "$RESULT_DIR"

# Create fastchess config
cat > "$RESULT_DIR/config.json" << EOF
{
    "engines": [
        {
            "name": "Stage15-Fixed",
            "cmd": "/workspace/binaries/seajay-stage15-fixed",
            "options": [
                ["Hash", 16],
                ["Threads", 1]
            ]
        },
        {
            "name": "Stage14-Final", 
            "cmd": "/workspace/binaries/seajay-stage14-final",
            "options": [
                ["Hash", 16],
                ["Threads", 1]
            ]
        }
    ],
    "games": 2,
    "rounds": 10000,
    "concurrency": 1,
    "draw": {
        "enabled": true,
        "score": 10,
        "moves": 40,
        "count": 8
    },
    "resign": {
        "enabled": true,
        "score": 700,
        "moves": 3
    },
    "sprt": {
        "enabled": true,
        "elo0": 0.0,
        "elo1": 30.0,
        "alpha": 0.05,
        "beta": 0.05,
        "model": "normalized"
    },
    "opening": {
        "file": "/workspace/external/books/4moves_test.pgn",
        "format": "pgn",
        "order": "random"
    },
    "tc": {
        "time": 10000,
        "increment": 100
    },
    "pgn": {
        "file": "$RESULT_DIR/games.pgn",
        "notation": "lan"
    },
    "log": {
        "file": "$RESULT_DIR/fastchess.log",
        "level": 3
    },
    "output": 1,
    "report_penta": true
}
EOF

echo "Starting SPRT test..."
echo "Results will be saved to: $RESULT_DIR"
echo ""
echo "Command:"
echo "fastchess -config $RESULT_DIR/config.json"
echo ""

# Run the test
cd "$RESULT_DIR"
fastchess -config config.json