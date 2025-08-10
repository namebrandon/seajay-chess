#!/bin/bash
# Setup GoogleTest for macOS builds
#
# This script downloads and builds GoogleTest specifically for macOS,
# allowing the tests to be built and run on macOS systems.

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
EXTERNAL_DIR="$PROJECT_ROOT/external"
GTEST_DIR="$EXTERNAL_DIR/googletest-macos"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

log() { echo -e "${BLUE}[$(date '+%H:%M:%S')]${NC} $1"; }
success() { echo -e "${GREEN}[SUCCESS]${NC} $1"; }
error() { echo -e "${RED}[ERROR]${NC} $1"; exit 1; }
warn() { echo -e "${YELLOW}[WARNING]${NC} $1"; }

# Check if running on macOS
check_platform() {
    if [[ "$OSTYPE" != "darwin"* ]]; then
        error "This script must be run on macOS"
    fi
    log "Detected macOS platform"
}

# Download GoogleTest
download_googletest() {
    log "Downloading GoogleTest for macOS..."
    
    # Use a specific release for consistency
    GTEST_VERSION="1.14.0"  # Note: without 'v' prefix for directory name
    GTEST_URL="https://github.com/google/googletest/archive/refs/tags/v${GTEST_VERSION}.tar.gz"
    
    # Clean and create directory
    rm -rf "$GTEST_DIR"
    mkdir -p "$GTEST_DIR"
    cd "$GTEST_DIR"
    
    # Download and extract
    log "Downloading GoogleTest v${GTEST_VERSION}..."
    curl -L "$GTEST_URL" -o "googletest-${GTEST_VERSION}.tar.gz" || error "Failed to download GoogleTest"
    
    log "Extracting GoogleTest..."
    tar -xzf "googletest-${GTEST_VERSION}.tar.gz" --strip-components=0 || error "Failed to extract GoogleTest"
    
    # The extraction creates googletest-1.14.0 directory
    if [[ ! -d "googletest-${GTEST_VERSION}" ]]; then
        error "Expected directory googletest-${GTEST_VERSION} not found after extraction"
    fi
    
    success "GoogleTest downloaded"
}

# Build GoogleTest for macOS
build_googletest() {
    log "Building GoogleTest for macOS ARM64..."
    
    # Use the known directory name
    GTEST_VERSION="1.14.0"
    GTEST_SRC="$GTEST_DIR/googletest-${GTEST_VERSION}"
    
    if [[ ! -d "$GTEST_SRC" ]]; then
        error "GoogleTest source directory not found at: $GTEST_SRC"
    fi
    
    log "Found GoogleTest source at: $GTEST_SRC"
    
    # Create build directory
    BUILD_DIR="$GTEST_SRC/build-macos"
    rm -rf "$BUILD_DIR"
    mkdir -p "$BUILD_DIR"
    cd "$BUILD_DIR"
    
    # Configure with CMake for macOS ARM64
    log "Configuring GoogleTest build..."
    log "Running cmake from: $(pwd)"
    log "Source directory: $GTEST_SRC"
    
    cmake "$GTEST_SRC" \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_CXX_COMPILER=clang++ \
        -DCMAKE_CXX_STANDARD=20 \
        -DCMAKE_OSX_ARCHITECTURES=arm64 \
        -DCMAKE_OSX_DEPLOYMENT_TARGET=11.0 \
        -DBUILD_GMOCK=OFF \
        -DINSTALL_GTEST=OFF \
        || error "CMake configuration failed"
    
    # Build
    log "Building GoogleTest..."
    make -j$(sysctl -n hw.ncpu) || error "Build failed"
    
    success "GoogleTest built successfully"
}

# Install GoogleTest libraries
install_googletest() {
    log "Installing GoogleTest libraries for macOS..."
    
    GTEST_VERSION="1.14.0"
    GTEST_SRC="$GTEST_DIR/googletest-${GTEST_VERSION}"
    BUILD_DIR="$GTEST_SRC/build-macos"
    
    # Create macOS-specific lib directory
    MACOS_LIB_DIR="$EXTERNAL_DIR/googletest-macos/lib"
    mkdir -p "$MACOS_LIB_DIR"
    
    # Copy the built libraries
    cp "$BUILD_DIR/lib/libgtest.a" "$MACOS_LIB_DIR/" || error "Failed to copy libgtest.a"
    cp "$BUILD_DIR/lib/libgtest_main.a" "$MACOS_LIB_DIR/" || error "Failed to copy libgtest_main.a"
    
    # Copy headers
    MACOS_INCLUDE_DIR="$EXTERNAL_DIR/googletest-macos/include"
    rm -rf "$MACOS_INCLUDE_DIR"
    cp -r "$GTEST_SRC/googletest/include" "$MACOS_INCLUDE_DIR" || error "Failed to copy headers"
    
    success "GoogleTest installed to $EXTERNAL_DIR/googletest-macos"
    
    # Create a marker file to indicate macOS GoogleTest is available
    echo "GoogleTest for macOS ARM64 - Built on $(date)" > "$EXTERNAL_DIR/googletest-macos/.macos-build"
    
    log "GoogleTest libraries location:"
    log "  Libraries: $MACOS_LIB_DIR"
    log "  Headers: $MACOS_INCLUDE_DIR"
}

# Main execution
main() {
    log "SeaJay Chess Engine - GoogleTest Setup for macOS"
    
    check_platform
    download_googletest
    build_googletest
    install_googletest
    
    success "GoogleTest setup complete!"
    echo ""
    echo "To use GoogleTest in your macOS build:"
    echo "  1. The build_macos.sh script will automatically detect and use these libraries"
    echo "  2. Or manually specify: -DGTEST_ROOT=$EXTERNAL_DIR/googletest-macos"
    echo ""
}

main "$@"