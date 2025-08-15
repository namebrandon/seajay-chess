#!/bin/bash
# Run SPRT test with cutechess-cli

if [ "$#" -ne 1 ]; then
    echo "Usage: $0 <config_file>"
    echo "Example: $0 see_pruning_aggressive.ini"
    exit 1
fi

CONFIG=$1

# Check if cutechess-cli is available
if ! command -v cutechess-cli &> /dev/null; then
    echo "cutechess-cli not found. Please install it first."
    exit 1
fi

# Run the SPRT test
cutechess-cli \
    -engine conf=SeaJay-Test cmd=/workspace/bin/seajay \
    -engine conf=SeaJay-Base cmd=/workspace/bin/seajay \
    -each proto=uci tc=10+0.1 \
    -games 2000 \
    -repeat \
    -recover \
    -pgnout sprt_results.pgn \
    -ratinginterval 10 \
    -concurrency 4 \
    -sprt elo0=0 elo1=40 alpha=0.05 beta=0.05

echo "SPRT test complete. Results saved to sprt_results.pgn"
