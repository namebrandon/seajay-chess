#!/bin/bash
# Test that evaluation is identical for various positions

positions=(
    "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"
    "4k3/pppppppp/8/8/8/8/PPPPPPPP/4K3 w - - 0 1"
    "4k3/8/8/3P4/8/8/8/4K3 w - - 0 1"
    "4k3/8/8/3PP3/8/8/8/4K3 w - - 0 1"
    "4k3/8/8/2PPP3/8/8/8/4K3 w - - 0 1"
)

for fen in "${positions[@]}"; do
    echo "Testing: $fen"
    eval=$(echo -e "position fen $fen\neval\nquit" | ./bin/seajay 2>/dev/null | grep "Final evaluation" | awk '{print $3}')
    echo "Evaluation: $eval"
done
