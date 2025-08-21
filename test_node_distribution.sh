#!/bin/bash

echo "=== Node Distribution Analysis ==="
echo ""
echo "Testing Tactical Position vs Quiet Position"
echo "============================================="
echo ""

# Tactical position
echo "TACTICAL POSITION (Kiwipete):"
echo "position fen r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1"
echo ""

RESULT=$(echo -e "uci\nsetoption name ShowPVSStats value true\nposition fen r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1\ngo depth 8\nquit" | ./bin/seajay 2>/dev/null | grep "depth 8")

# Extract values
NODES=$(echo "$RESULT" | grep -o "nodes [0-9]*" | awk '{print $2}')
SELDEPTH=$(echo "$RESULT" | grep -o "seldepth [0-9]*" | awk '{print $2}')
TTHITS=$(echo "$RESULT" | grep -o "tthits [0-9.]*%" | awk '{print $2}')

echo "  Depth 8 Results:"
echo "  - Total nodes: $NODES"
echo "  - Selective depth: $SELDEPTH (extension: $((SELDEPTH - 8)))"
echo "  - TT hit rate: $TTHITS"

# Get PVS stats
PVS_RESULT=$(echo -e "uci\nsetoption name ShowPVSStats value true\nposition fen r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1\ngo depth 8\nquit" | ./bin/seajay 2>/dev/null | grep "PVS re-search rate" | tail -1)
echo "  - PVS re-search rate: $(echo "$PVS_RESULT" | grep -o "[0-9.]*%")"

echo ""
echo "============================================="
echo ""

# Starting position (quiet)
echo "QUIET POSITION (Starting):"
echo "position startpos"
echo ""

RESULT=$(echo -e "uci\nsetoption name ShowPVSStats value true\nposition startpos\ngo depth 8\nquit" | ./bin/seajay 2>/dev/null | grep "depth 8")

# Extract values
NODES=$(echo "$RESULT" | grep -o "nodes [0-9]*" | awk '{print $2}')
SELDEPTH=$(echo "$RESULT" | grep -o "seldepth [0-9]*" | awk '{print $2}')
TTHITS=$(echo "$RESULT" | grep -o "tthits [0-9.]*%" | awk '{print $2}')

echo "  Depth 8 Results:"
echo "  - Total nodes: $NODES"
echo "  - Selective depth: $SELDEPTH (extension: $((SELDEPTH - 8)))"
echo "  - TT hit rate: $TTHITS"

# Get PVS stats
PVS_RESULT=$(echo -e "uci\nsetoption name ShowPVSStats value true\nposition startpos\ngo depth 8\nquit" | ./bin/seajay 2>/dev/null | grep "PVS re-search rate" | tail -1)
echo "  - PVS re-search rate: $(echo "$PVS_RESULT" | grep -o "[0-9.]*%")"

echo ""
echo "============================================="
echo ""
echo "ANALYSIS:"
echo "- Tactical position has much higher seldepth (more quiescence)"
echo "- Tactical position has different TT behavior"
echo "- PVS re-search rates differ significantly"