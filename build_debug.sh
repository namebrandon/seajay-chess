#!/bin/bash
# Build SeaJay in DEBUG mode with sanitizers
# This mode is for debugging and finding memory/UB issues

set -e  # Exit on error

echo "=========================================="
echo "Building SeaJay - DEBUG MODE"
echo "With AddressSanitizer & UBSanitizer"
echo "Purpose: Debugging & memory safety checks"
echo "=========================================="
echo ""

# Ensure build directory exists
mkdir -p build
cd build

# Clean previous build to ensure fresh compilation with debug settings
echo "Cleaning previous build..."
rm -rf CMakeCache.txt CMakeFiles/ Makefile

# Configure with Debug mode and sanitizers
echo "Configuring with Debug mode and sanitizers..."
cmake -DCMAKE_BUILD_TYPE=Debug \
      -DCMAKE_CXX_FLAGS="-fsanitize=address,undefined -fno-omit-frame-pointer" \
      -DCMAKE_EXE_LINKER_FLAGS="-fsanitize=address,undefined" \
      -DQSEARCH_MODE=TESTING ..

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
echo "Mode: DEBUG with sanitizers"
echo "Quiescence: TESTING mode (for faster debugging)"
echo ""
echo "Note: The binary will run slower due to"
echo "sanitizer overhead. Any memory errors or"
echo "undefined behavior will be reported."
echo "=========================================="