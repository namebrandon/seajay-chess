#!/bin/bash

# Quick WAC Test Wrapper
# Runs the WAC tactical test suite with 1 second per position
# This provides a fast tactical performance check

# Color codes for output
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Configuration
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
WORKSPACE_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
ENGINE_PATH="${1:-$WORKSPACE_DIR/bin/seajay}"
TEST_FILE="$WORKSPACE_DIR/tests/positions/wacnew.epd"
TIME_PER_MOVE=1000  # 1 second per position

echo -e "${BLUE}==========================================${NC}"
echo -e "${BLUE}WAC Quick Test (1 second per position)${NC}"
echo -e "${BLUE}==========================================${NC}"
echo "Engine: $ENGINE_PATH"
echo "Test suite: $TEST_FILE"
echo "Time per position: 1 second"
echo -e "${BLUE}==========================================${NC}"
echo

# Check if engine exists
if [ ! -f "$ENGINE_PATH" ]; then
    echo -e "${YELLOW}Warning: Engine not found at $ENGINE_PATH${NC}"
    echo "Usage: $0 [engine_path]"
    echo "Default: ./bin/seajay"
    exit 1
fi

# Check if test file exists
if [ ! -f "$TEST_FILE" ]; then
    echo -e "${YELLOW}Warning: WAC test file not found at $TEST_FILE${NC}"
    exit 1
fi

# Estimate runtime
POSITION_COUNT=$(grep -c "bm" "$TEST_FILE" 2>/dev/null || echo "300")
ESTIMATED_TIME=$((POSITION_COUNT + 10))  # Add buffer for overhead
echo -e "Estimated runtime: ~${ESTIMATED_TIME} seconds ($(($ESTIMATED_TIME / 60)) minutes)"
echo

# Run the tactical test
"$SCRIPT_DIR/tactical_test.sh" "$ENGINE_PATH" "$TEST_FILE" "$TIME_PER_MOVE"

# Check exit code
EXIT_CODE=$?
if [ $EXIT_CODE -eq 0 ]; then
    echo -e "\n${GREEN}All positions solved!${NC}"
else
    echo -e "\n${YELLOW}$EXIT_CODE positions failed${NC}"
fi

# Show latest results file
LATEST_RESULT=$(ls -t tactical_test_*.csv 2>/dev/null | head -1)
if [ ! -z "$LATEST_RESULT" ]; then
    echo -e "\nDetailed results saved to: ${BLUE}$LATEST_RESULT${NC}"
fi

exit $EXIT_CODE