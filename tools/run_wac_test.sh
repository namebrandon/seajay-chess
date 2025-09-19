#!/bin/bash

# Simple WAC Test Runner
# Runs the Win At Chess tactical test suite

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
WORKSPACE_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"

# Default settings
ENGINE="${1:-$WORKSPACE_DIR/bin/seajay}"
TIME_MS="${2:-850}"  # 1/2 second per position by default

# Colors
BLUE='\033[0;34m'
GREEN='\033[0;32m'
NC='\033[0m'

echo -e "${BLUE}╔════════════════════════════════════════════╗${NC}"
echo -e "${BLUE}║     WAC Tactical Test Suite (300 positions) ║${NC}"
echo -e "${BLUE}╚════════════════════════════════════════════╝${NC}"
echo
echo "Usage: $0 [engine_path] [time_per_move_ms]"
echo
echo "Engine: $ENGINE"
echo "Time per move: ${TIME_MS}ms"
echo "Estimated time: ~$((300 * TIME_MS / 1000 + 30)) seconds"
echo

# Run the test
python3 "$SCRIPT_DIR/tactical_test.py" "$ENGINE" "$WORKSPACE_DIR/tests/positions/wacnew.epd" "$TIME_MS"
