#!/bin/bash
# SeaJay Chess Engine - Universal macOS Build Wrapper
#
# This script automatically detects your platform and builds SeaJay for macOS:
# - On macOS: Uses native compilation
# - On Linux: Attempts cross-compilation (requires osxcross)
#
# The resulting binary is always named seajay-macos to avoid confusion
# with the Linux development binary.

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Colors
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

echo -e "${BLUE}SeaJay Chess Engine - macOS Build${NC}"
echo ""

# Detect platform
if [[ "$OSTYPE" == "darwin"* ]]; then
    echo -e "${GREEN}Detected macOS - using native compilation${NC}"
    exec "$SCRIPT_DIR/build_macos.sh" "$@"
elif [[ "$OSTYPE" == "linux-gnu"* ]]; then
    echo -e "${YELLOW}Detected Linux - attempting cross-compilation${NC}"
    echo "Note: This requires osxcross toolchain"
    echo ""
    exec "$SCRIPT_DIR/cross_compile_macos.sh" "$@"
else
    echo "Unsupported platform: $OSTYPE"
    echo "This script only supports macOS and Linux"
    exit 1
fi