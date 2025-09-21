#!/bin/bash
# Build script for SeaJay Chess Engine - CROSS-PLATFORM LOCAL DEVELOPMENT
# =============================================================================
# This script is for LOCAL DEVELOPMENT ONLY!
# It supports Linux/macOS as well as Windows (MSYS2/MinGW) environments.
# For OpenBench builds, continue using: make (which applies AVX2/BMI2 tuning).
# =============================================================================
#
# Usage: ./build.sh [Debug|Release]
#
# Examples:
#   ./build.sh                  # Release build (default)
#   ./build.sh Debug            # Debug build with sanitizers
#   ./build.sh Release          # Explicit release build

set -euo pipefail

BUILD_TYPE=${1:-Release}

# Detect execution environment so we can apply Windows specific settings
# when running under MSYS/MinGW shells.
detect_environment() {
    if [[ "${OSTYPE}" == "msys" ]] || [[ "${OSTYPE}" == "cygwin" ]] || [[ -n "${MSYSTEM:-}" ]]; then
        case "${MSYSTEM:-}" in
            MINGW64|MINGW32|UCRT64)
                echo "mingw"
                ;;
            *)
                echo "msys"
                ;;
        esac
    elif [[ "${OSTYPE}" == "darwin"* ]]; then
        echo "macos"
    elif [[ "${OSTYPE}" == "linux-gnu"* ]]; then
        echo "linux"
    else
        echo "unknown"
    fi
}

ENVIRONMENT=$(detect_environment)

echo "=========================================="
echo "Building SeaJay Chess Engine"
echo "Build Type: ${BUILD_TYPE}"
echo "Environment: ${ENVIRONMENT}"
echo "=========================================="
echo ""

case "${ENVIRONMENT}" in
    msys)
        echo "WARNING: Detected MSYS shell (POSIX layer)."
        echo "For GUI compatible Windows binaries use the \"MSYS2 MinGW 64-bit\" shell."
        echo "Continuing with MSYS build — binary may depend on msys-2.0.dll."
        echo ""
        ;;
    mingw)
        echo "WINDOWS BUILD (MinGW):"
        echo "  - Static linking enabled for standalone seajay.exe"
        echo "  - Compatible with Windows chess GUIs"
        echo ""
        ;;
    macos)
        echo "macOS build detected."
        echo ""
        ;;
    linux)
        echo "Linux build detected."
        echo ""
        ;;
    *)
        ;;
esac

if [[ "${BUILD_TYPE}" == "Debug" ]]; then
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
if [[ -f CMakeCache.txt ]]; then
    CURRENT_TYPE=$(grep CMAKE_BUILD_TYPE CMakeCache.txt | cut -d= -f2)
    if [[ "${CURRENT_TYPE}" != "${BUILD_TYPE}" ]]; then
        echo "Switching build types - cleaning build..."
        rm -rf CMakeCache.txt CMakeFiles/
    fi
fi

# Always use Makefiles for compatibility with OpenBench and to keep the
# sequential -flto workaround consistent across platforms.
echo "Using Make build system (OpenBench compatible)..."

# Clear potentially problematic environment variables that could cause
# illegal instruction errors or interfere with LTO jobserver behaviour.
unset CXXFLAGS
unset CFLAGS
unset MAKEFLAGS
unset CMAKE_GENERATOR
unset CMAKE_GENERATOR_TOOLSET
unset CMAKE_GENERATOR_INSTANCE

if command -v make >/dev/null 2>&1; then
    MAKE_CMD="$(command -v make)"
else
    MAKE_CMD="make"
fi

if [[ "${ENVIRONMENT}" == "mingw" ]] || [[ "${ENVIRONMENT}" == "msys" ]]; then
    if command -v mingw32-make >/dev/null 2>&1; then
        MAKE_CMD="$(command -v mingw32-make)"
    else
        echo "ERROR: mingw32-make not found. Install it via 'pacman -S mingw-w64-x86_64-make' from MSYS2." >&2
        exit 1
    fi
fi

MAKE_CMD_NAME="$(basename "${MAKE_CMD}")"

EXTRA_CMAKE_ARGS=()
if [[ "${ENVIRONMENT}" == "mingw" ]] || [[ "${ENVIRONMENT}" == "msys" ]]; then
    EXTRA_CMAKE_ARGS+=("-DCMAKE_CXX_FLAGS=-static -static-libgcc -static-libstdc++")
    EXTRA_CMAKE_ARGS+=("-DCMAKE_EXE_LINKER_FLAGS=-static")
    EXTRA_CMAKE_ARGS+=("-DCMAKE_MAKE_PROGRAM=${MAKE_CMD}")
    echo "Applying Windows static linking flags..."
fi

CMAKE_ARGS=("-DCMAKE_BUILD_TYPE=${BUILD_TYPE}")
CMAKE_ARGS+=("${EXTRA_CMAKE_ARGS[@]}")

GENERATOR="Unix Makefiles"
if [[ "${ENVIRONMENT}" == "mingw" ]]; then
    GENERATOR="MinGW Makefiles"
elif [[ "${ENVIRONMENT}" == "msys" ]]; then
    GENERATOR="MSYS Makefiles"
fi

CMAKE_ARGS+=("..")

MAKEFLAGS= cmake -G "${GENERATOR}" "${CMAKE_ARGS[@]}"

# Parallel builds currently trip GCC's LTO jobserver hand-off on this
# environment (and on MSYS/MinGW). Running sequentially keeps the link
# step stable and still finishes within a minute.
MAKEFLAGS= "${MAKE_CMD}" seajay

echo ""
echo "=========================================="
echo "Build complete!"

if [[ "${ENVIRONMENT}" == "mingw" ]] || [[ "${ENVIRONMENT}" == "msys" ]]; then
    BINARY_NAME="seajay.exe"
else
    BINARY_NAME="seajay"
fi

RELATIVE_BINARY_PATH="../bin/${BINARY_NAME}"

if [[ "${ENVIRONMENT}" == "mingw" ]] || [[ "${ENVIRONMENT}" == "msys" ]]; then
    echo "Binary: ${RELATIVE_BINARY_PATH}"
    echo ""
    echo "Windows notes:"
    echo "  ✓ Static linking enabled for standalone executable"
    echo "  ✓ Compatible with UCI chess GUIs"
    if [[ "${ENVIRONMENT}" == "msys" ]]; then
        echo "  ⚠ May require msys-2.0.dll if run outside MSYS shell"
    fi
else
    echo "Binary: ${RELATIVE_BINARY_PATH}"
fi

echo ""
echo "Quiescence search node limits are now controlled via UCI:"
echo "  setoption name QSearchNodeLimit value 0       # Unlimited (default)"
echo "  setoption name QSearchNodeLimit value 10000   # Testing mode equivalent"
echo "  setoption name QSearchNodeLimit value 100000  # Tuning mode equivalent"
echo ""
echo "To verify build, run:"
echo "  echo 'uci' | ${RELATIVE_BINARY_PATH}"
echo "=========================================="
