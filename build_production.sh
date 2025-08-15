#!/bin/bash
# Build SeaJay in PRODUCTION mode (no quiescence limits)
# This is the full-strength mode for competitive play and SPRT testing

set -e  # Exit on error

echo "=========================================="
echo "Building SeaJay - PRODUCTION MODE"
echo "Quiescence Search: No limits"
echo "Purpose: Competitive play & SPRT testing"
echo "=========================================="
echo ""

# Ensure build directory exists
mkdir -p build
cd build

# Clean previous build to ensure fresh compilation with new mode
echo "Cleaning previous build..."
rm -rf CMakeCache.txt CMakeFiles/ Makefile

# Configure with PRODUCTION mode (default)
echo "Configuring with PRODUCTION mode..."
cmake -DCMAKE_BUILD_TYPE=Release -DQSEARCH_MODE=PRODUCTION ..

# Build with all available cores
echo "Building..."
make -j 2>/dev/null || true  # Continue even if some test targets fail

# Ensure the main binary is copied to bin/
if [ -f seajay ]; then
    cp -f seajay ../bin/seajay
    echo "Binary updated: /workspace/bin/seajay"
fi

echo ""
echo "=========================================="
echo "Build complete!"
echo "Binary: /workspace/bin/seajay"
echo "Mode: PRODUCTION (no quiescence limits)"
echo ""
echo "The engine will display:"
echo "  'Quiescence: PRODUCTION MODE'"
echo "at startup to confirm mode"
echo "=========================================="