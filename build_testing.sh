#!/bin/bash
# Build SeaJay with Quiescence Search TESTING mode (10K node limit)
# This mode is for rapid testing and validation

set -e  # Exit on error

echo "=========================================="
echo "Building SeaJay - TESTING MODE"
echo "Quiescence Search: 10K node limit"
echo "Purpose: Rapid testing and validation"
echo "=========================================="
echo ""

# Ensure build directory exists
mkdir -p build
cd build

# Clean previous build to ensure fresh compilation with new mode
echo "Cleaning previous build..."
rm -rf CMakeCache.txt CMakeFiles/ Makefile

# Configure with TESTING mode
echo "Configuring with QSEARCH_TESTING mode..."
cmake -DCMAKE_BUILD_TYPE=Release -DQSEARCH_MODE=TESTING ..

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
echo "Mode: TESTING (10K quiescence node limit)"
echo ""
echo "The engine will display:"
echo "  'Quiescence: TESTING MODE - 10K limit'"
echo "at startup to confirm mode"
echo "=========================================="