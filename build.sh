#!/bin/bash
# Build script for SeaJay Chess Engine
# Stage 14 Remediation: Simplified build - all modes controlled via UCI
# Usage: ./build.sh [Debug|Release]
#
# Examples:
#   ./build.sh                  # Release build (default)
#   ./build.sh Debug            # Debug build with sanitizers
#   ./build.sh Release          # Explicit release build

BUILD_TYPE=${1:-Release}

echo "=========================================="
echo "Building SeaJay Chess Engine"
echo "Build Type: $BUILD_TYPE"
echo "=========================================="
echo ""

if [ "$BUILD_TYPE" == "Debug" ]; then
    echo "DEBUG BUILD:"
    echo "  - Debug symbols enabled"
    echo "  - Optimizations disabled"
    echo "  - Assertions enabled"
    echo "  - Suitable for debugging"
else
    echo "RELEASE BUILD:"
    echo "  - Full optimizations enabled"
    echo "  - No debug symbols"
    echo "  - Maximum performance"
    echo "  - Suitable for play and testing"
fi
echo ""

mkdir -p build
cd build

# Clean if switching build types
if [ -f CMakeCache.txt ]; then
    CURRENT_TYPE=$(grep CMAKE_BUILD_TYPE CMakeCache.txt | cut -d= -f2)
    if [ "$CURRENT_TYPE" != "$BUILD_TYPE" ]; then
        echo "Switching build types - cleaning build..."
        rm -rf CMakeCache.txt CMakeFiles/
    fi
fi

# CRITICAL: Always use Make for OpenBench compatibility
# Never use ninja even if available
echo "Using Make build system (OpenBench compatible)..."
cmake -DCMAKE_BUILD_TYPE=$BUILD_TYPE ..
make -j seajay

echo ""
echo "=========================================="
echo "Build complete!"
echo "Binary: /workspace/bin/seajay"
echo ""
echo "Quiescence search node limits are now controlled via UCI:"
echo "  setoption name QSearchNodeLimit value 0       # Unlimited (default)"
echo "  setoption name QSearchNodeLimit value 10000   # Testing mode equivalent"
echo "  setoption name QSearchNodeLimit value 100000  # Tuning mode equivalent"
echo ""
echo "To verify build, run:"
echo "  echo 'uci' | /workspace/bin/seajay"
echo "=========================================="