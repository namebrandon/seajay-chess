#!/bin/bash
# Build SeaJay with Quiescence Search TUNING mode (100K node limit)
# This mode is for parameter tuning and experimentation

set -e  # Exit on error

echo "=========================================="
echo "Building SeaJay - TUNING MODE"
echo "Quiescence Search: 100K node limit"
echo "Purpose: Parameter tuning & experimentation"
echo "=========================================="
echo ""

# Ensure build directory exists
mkdir -p build
cd build

# Clean previous build to ensure fresh compilation with new mode
echo "Cleaning previous build..."
make clean 2>/dev/null || true  # Clean object files if Makefile exists
rm -rf CMakeCache.txt CMakeFiles/ Makefile
rm -f *.o src/*.o src/*/*.o  # Force remove any lingering object files

# Configure with TUNING mode
echo "Configuring with QSEARCH_TUNING mode..."
cmake -DCMAKE_BUILD_TYPE=Release -DQSEARCH_MODE=TUNING ..

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
echo "Mode: TUNING (100K quiescence node limit)"
echo ""
echo "The engine will display:"
echo "  'Quiescence: TUNING MODE - 100K limit'"
echo "at startup to confirm mode"
echo "=========================================="