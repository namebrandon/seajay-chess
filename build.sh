#!/bin/bash
# Quick build script for SeaJay with Quiescence Search mode support
# Usage: ./build.sh [mode] [build_type]
#   mode: testing, tuning, production (default: production)
#   build_type: Debug, Release (default: Release)
#
# Examples:
#   ./build.sh                  # Production mode, Release build
#   ./build.sh testing          # Testing mode (10K limit), Release build
#   ./build.sh tuning Debug     # Tuning mode (100K limit), Debug build

MODE=${1:-production}
BUILD_TYPE=${2:-Release}

# Convert mode to uppercase for CMAKE
QSEARCH_MODE="PRODUCTION"
MODE_DESC="PRODUCTION (no limits)"

case "${MODE,,}" in
    testing)
        QSEARCH_MODE="TESTING"
        MODE_DESC="TESTING (10K node limit)"
        ;;
    tuning)
        QSEARCH_MODE="TUNING"
        MODE_DESC="TUNING (100K node limit)"
        ;;
    production|prod)
        QSEARCH_MODE="PRODUCTION"
        MODE_DESC="PRODUCTION (no limits)"
        ;;
    *)
        echo "Invalid mode: $MODE"
        echo "Usage: $0 [testing|tuning|production] [Debug|Release]"
        exit 1
        ;;
esac

echo "=========================================="
echo "Building SeaJay Chess Engine"
echo "Build Type: $BUILD_TYPE"
echo "Quiescence Mode: $MODE_DESC"
echo "=========================================="
echo ""

# Show what this mode means
case "${MODE,,}" in
    testing)
        echo "TESTING MODE:"
        echo "  - Quiescence search limited to 10K nodes"
        echo "  - Fast iteration for development"
        echo "  - Not suitable for strength testing"
        ;;
    tuning)
        echo "TUNING MODE:"
        echo "  - Quiescence search limited to 100K nodes"
        echo "  - Good for parameter experimentation"
        echo "  - Balanced speed vs accuracy"
        ;;
    production|prod)
        echo "PRODUCTION MODE:"
        echo "  - No quiescence search limits"
        echo "  - Full engine strength"
        echo "  - Use for SPRT and competitive play"
        ;;
esac
echo ""

mkdir -p build
cd build

# Clean if switching modes
if [ -f CMakeCache.txt ]; then
    CURRENT_MODE=$(grep QSEARCH_MODE CMakeCache.txt | cut -d= -f2)
    if [ "$CURRENT_MODE" != "$QSEARCH_MODE" ]; then
        echo "Switching modes - cleaning build..."
        rm -rf CMakeCache.txt CMakeFiles/
    fi
fi

# Try to use ninja if available, otherwise fall back to make
if command -v ninja >/dev/null 2>&1; then
    echo "Using Ninja build system..."
    cmake -G Ninja -DCMAKE_BUILD_TYPE=$BUILD_TYPE -DQSEARCH_MODE=$QSEARCH_MODE ..
    ninja
else
    echo "Using Make build system..."
    cmake -DCMAKE_BUILD_TYPE=$BUILD_TYPE -DQSEARCH_MODE=$QSEARCH_MODE ..
    make -j
fi

echo ""
echo "=========================================="
echo "Build complete!"
echo "Binary: /workspace/bin/seajay"
echo "Mode: $MODE_DESC"
echo ""
echo "To verify mode, run:"
echo "  echo 'uci' | /workspace/bin/seajay"
echo ""
echo "The engine will display its mode at startup."
echo "=========================================="
