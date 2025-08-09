#!/bin/bash
# SeaJay Chess Engine - macOS (ARM64) Build Script
# 
# This script compiles SeaJay for macOS ARM64 (Apple Silicon) systems.
# It creates a separate build directory to avoid interfering with Linux development.
# The resulting binary is renamed with -macos suffix for clarity.
#
# Usage: ./build_macos.sh [clean]
#        clean - removes the macOS build directory before building

set -e  # Exit on error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Script configuration
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
BUILD_DIR="$PROJECT_ROOT/build-macos"
OUTPUT_DIR="$PROJECT_ROOT/bin-macos"
ENGINE_NAME="seajay-macos"

log() {
    echo -e "${BLUE}[$(date '+%H:%M:%S')]${NC} $1"
}

error() {
    echo -e "${RED}[ERROR]${NC} $1" >&2
    exit 1
}

warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

# Check if running on macOS
check_platform() {
    if [[ "$OSTYPE" != "darwin"* ]]; then
        warning "This script is intended for macOS but you're running on $OSTYPE"
        echo "Do you want to continue anyway? (y/n)"
        read -r response
        if [[ "$response" != "y" ]]; then
            log "Build cancelled"
            exit 0
        fi
    fi
    
    # Check for ARM64 architecture
    if [[ $(uname -m) == "arm64" ]]; then
        log "Detected Apple Silicon (ARM64) architecture"
    elif [[ $(uname -m) == "x86_64" ]]; then
        warning "Detected Intel (x86_64) architecture"
        warning "This script is optimized for Apple Silicon but will continue"
    else
        warning "Unknown architecture: $(uname -m)"
    fi
}

# Check for required tools
check_requirements() {
    log "Checking build requirements..."
    
    # Check for C++ compiler
    if command -v clang++ &> /dev/null; then
        log "Found clang++: $(clang++ --version | head -1)"
    elif command -v g++ &> /dev/null; then
        log "Found g++: $(g++ --version | head -1)"
    else
        error "No C++ compiler found. Please install Xcode Command Line Tools:"
        echo "  xcode-select --install"
        exit 1
    fi
    
    # Check for CMake
    if ! command -v cmake &> /dev/null; then
        error "CMake not found. Please install CMake:"
        echo "  brew install cmake"
        exit 1
    fi
    
    log "Found CMake: $(cmake --version | head -1)"
}

# Clean build directory if requested
clean_build() {
    if [[ "$1" == "clean" ]]; then
        log "Cleaning previous macOS build..."
        rm -rf "$BUILD_DIR"
        rm -rf "$OUTPUT_DIR"
        success "Clean complete"
    fi
}

# Create build directories
setup_directories() {
    log "Setting up build directories..."
    mkdir -p "$BUILD_DIR"
    mkdir -p "$OUTPUT_DIR"
}

# Configure CMake for macOS ARM64
configure_cmake() {
    log "Configuring CMake for macOS ARM64..."
    cd "$BUILD_DIR"
    
    # CMake configuration optimized for Apple Silicon
    # Note: We don't override CMAKE_RUNTIME_OUTPUT_DIRECTORY to respect CMakeLists.txt settings
    cmake "$PROJECT_ROOT" \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_CXX_COMPILER=clang++ \
        -DCMAKE_CXX_STANDARD=20 \
        -DCMAKE_CXX_FLAGS="-O3 -arch arm64 -mtune=native -flto -ffast-math -DNDEBUG" \
        -DCMAKE_OSX_ARCHITECTURES=arm64 \
        -DCMAKE_OSX_DEPLOYMENT_TARGET=11.0 \
        || error "CMake configuration failed"
    
    success "CMake configuration complete"
}

# Build the engine
build_engine() {
    log "Building SeaJay for macOS ARM64..."
    cd "$BUILD_DIR"
    
    # Determine number of cores for parallel build
    if [[ "$OSTYPE" == "darwin"* ]]; then
        CORES=$(sysctl -n hw.ncpu)
    else
        CORES=$(nproc 2>/dev/null || echo 4)
    fi
    
    log "Building with $CORES parallel jobs..."
    make -j"$CORES" || error "Build failed"
    
    success "Build complete"
}

# Rename and finalize the binary
finalize_binary() {
    log "Finalizing binary..."
    
    # CMake places the binary directly in bin-macos/ due to CMAKE_RUNTIME_OUTPUT_DIRECTORY
    # which is set to ${CMAKE_BINARY_DIR}/../bin, and our build dir is build-macos
    # So the binary ends up in bin-macos/seajay
    BUILT_BINARY="$OUTPUT_DIR/seajay"
    
    # Check if the binary exists
    if [[ -f "$BUILT_BINARY" ]]; then
        # Rename with -macos suffix
        mv "$BUILT_BINARY" "$OUTPUT_DIR/$ENGINE_NAME"
        
        # Make sure it's executable
        chmod +x "$OUTPUT_DIR/$ENGINE_NAME"
        
        # Strip debug symbols for smaller size
        if command -v strip &> /dev/null; then
            log "Stripping debug symbols..."
            strip "$OUTPUT_DIR/$ENGINE_NAME"
        fi
        
        success "Binary created: $OUTPUT_DIR/$ENGINE_NAME"
    else
        error "Binary not found at expected location: $BUILT_BINARY"
        log "Checking possible locations..."
        find "$BUILD_DIR" -name "seajay*" -type f 2>/dev/null || true
        find "$OUTPUT_DIR" -name "seajay*" -type f 2>/dev/null || true
    fi
}

# Test the binary
test_binary() {
    log "Testing the binary..."
    
    # Basic UCI test
    if echo -e "uci\nquit" | "$OUTPUT_DIR/$ENGINE_NAME" | grep -q "uciok"; then
        success "UCI protocol test passed"
    else
        warning "UCI protocol test failed or timed out"
    fi
    
    # Display binary info
    log "Binary information:"
    file "$OUTPUT_DIR/$ENGINE_NAME" 2>/dev/null || true
    ls -lh "$OUTPUT_DIR/$ENGINE_NAME"
    
    # Quick bench test
    log "Running quick benchmark..."
    if timeout 10 "$OUTPUT_DIR/$ENGINE_NAME" bench 2 2>/dev/null | grep -q "nps"; then
        success "Benchmark test passed"
    else
        warning "Benchmark test failed or timed out"
    fi
}

# Print usage instructions
print_usage() {
    echo ""
    echo "========================================="
    echo "  SeaJay macOS Build Complete!"
    echo "========================================="
    echo ""
    echo "Binary location:"
    echo "  $OUTPUT_DIR/$ENGINE_NAME"
    echo ""
    echo "To use with a chess GUI:"
    echo "  1. Open your chess GUI (Arena, Cute Chess, etc.)"
    echo "  2. Add new engine"
    echo "  3. Browse to: $OUTPUT_DIR/$ENGINE_NAME"
    echo ""
    echo "To test from command line:"
    echo "  $OUTPUT_DIR/$ENGINE_NAME"
    echo ""
    echo "To run benchmark:"
    echo "  $OUTPUT_DIR/$ENGINE_NAME bench"
    echo ""
    if [[ "$OSTYPE" == "darwin"* ]]; then
        echo "To copy to Applications folder:"
        echo "  cp $OUTPUT_DIR/$ENGINE_NAME /Applications/"
    fi
    echo ""
}

# Main execution
main() {
    log "SeaJay Chess Engine - macOS ARM64 Build Script"
    log "Project root: $PROJECT_ROOT"
    
    # Check platform
    check_platform
    
    # Check requirements
    check_requirements
    
    # Clean if requested
    clean_build "$1"
    
    # Setup directories
    setup_directories
    
    # Configure CMake
    configure_cmake
    
    # Build
    build_engine
    
    # Finalize binary
    finalize_binary
    
    # Test
    test_binary
    
    # Print usage
    print_usage
    
    success "Build process complete!"
}

# Handle errors
trap 'error "Build failed on line $LINENO"' ERR

# Run main function
main "$@"