#!/bin/bash
# SeaJay Chess Engine - Cross-Compile for macOS from Linux
# 
# This script attempts to cross-compile SeaJay for macOS from a Linux system.
# Note: This requires osxcross toolchain to be installed.
# 
# For native macOS compilation, use build_macos.sh instead.
#
# Usage: ./cross_compile_macos.sh [setup|build|clean]
#        setup - instructions for setting up osxcross
#        build - build the engine (default)
#        clean - remove build directory

set -e

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
BUILD_DIR="$PROJECT_ROOT/build-macos-cross"
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

# Show setup instructions
show_setup() {
    echo "========================================="
    echo "  macOS Cross-Compilation Setup"
    echo "========================================="
    echo ""
    echo "To cross-compile for macOS from Linux, you need osxcross:"
    echo ""
    echo "1. Install dependencies:"
    echo "   sudo apt-get install clang llvm-dev libxml2-dev uuid-dev \\"
    echo "   libssl-dev bash patch make tar xz-utils bzip2 gzip \\"
    echo "   sed cpio libbz2-dev"
    echo ""
    echo "2. Clone osxcross:"
    echo "   git clone https://github.com/tpoechtrager/osxcross"
    echo "   cd osxcross"
    echo ""
    echo "3. Download macOS SDK (you need Xcode):"
    echo "   - Download Xcode from Apple Developer"
    echo "   - Extract SDK using osxcross/tools/gen_sdk_package.sh"
    echo "   - Place in osxcross/tarballs/"
    echo ""
    echo "4. Build osxcross:"
    echo "   UNATTENDED=1 ./build.sh"
    echo ""
    echo "5. Add to PATH:"
    echo "   export PATH=/path/to/osxcross/target/bin:\$PATH"
    echo ""
    echo "6. Run this script again with 'build' option"
    echo ""
    echo "Note: For simpler building on actual macOS hardware,"
    echo "      use build_macos.sh instead."
    exit 0
}

# Check for osxcross
check_osxcross() {
    log "Checking for osxcross toolchain..."
    
    if ! command -v x86_64-apple-darwin20.4-clang++ &> /dev/null && \
       ! command -v arm64-apple-darwin20.4-clang++ &> /dev/null && \
       ! command -v o64-clang++ &> /dev/null; then
        error "osxcross not found in PATH"
        echo ""
        echo "Run '$0 setup' for installation instructions"
        exit 1
    fi
    
    # Find the compiler
    if command -v arm64-apple-darwin20.4-clang++ &> /dev/null; then
        CXX="arm64-apple-darwin20.4-clang++"
        ARCH="arm64"
    elif command -v o64-clang++ &> /dev/null; then
        CXX="o64-clang++"
        ARCH="x86_64"
    else
        CXX="x86_64-apple-darwin20.4-clang++"
        ARCH="x86_64"
    fi
    
    log "Found cross-compiler: $CXX (targeting $ARCH)"
}

# Clean build
clean_build() {
    log "Cleaning previous cross-compilation build..."
    rm -rf "$BUILD_DIR"
    rm -f "$OUTPUT_DIR/$ENGINE_NAME"
    success "Clean complete"
}

# Build the engine
build_engine() {
    log "Cross-compiling SeaJay for macOS..."
    
    mkdir -p "$BUILD_DIR"
    mkdir -p "$OUTPUT_DIR"
    cd "$BUILD_DIR"
    
    # Create a simple Makefile for cross-compilation
    cat > Makefile << 'EOF'
CXX := $$CXX_COMPILER
CXXFLAGS := -std=c++20 -O3 -DNDEBUG -flto -ffast-math -Wall -Wextra
LDFLAGS := -flto

SRC_DIR := $$SRC_PATH
BUILD_DIR := .
TARGET := seajay-macos

# Source files
SOURCES := \
    $(SRC_DIR)/src/main.cpp \
    $(SRC_DIR)/src/core/board.cpp \
    $(SRC_DIR)/src/core/board_safety.cpp \
    $(SRC_DIR)/src/core/move_generation.cpp \
    $(SRC_DIR)/src/core/move_list.cpp \
    $(SRC_DIR)/src/uci/uci.cpp \
    $(SRC_DIR)/src/benchmark/benchmark.cpp

OBJECTS := $(patsubst $(SRC_DIR)/src/%.cpp,$(BUILD_DIR)/%.o,$(SOURCES))

# Include directories
INCLUDES := -I$(SRC_DIR)/src

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CXX) $(LDFLAGS) -o $@ $^

$(BUILD_DIR)/%.o: $(SRC_DIR)/src/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c -o $@ $<

clean:
	rm -rf $(BUILD_DIR)/*.o $(BUILD_DIR)/*/*.o $(TARGET)

.PHONY: all clean
EOF
    
    # Export variables for Makefile
    export CXX_COMPILER="$CXX"
    export SRC_PATH="$PROJECT_ROOT"
    
    log "Building with cross-compiler..."
    make -j$(nproc) || error "Build failed"
    
    # Move binary to output directory
    if [[ -f "seajay-macos" ]]; then
        mv "seajay-macos" "$OUTPUT_DIR/$ENGINE_NAME"
        chmod +x "$OUTPUT_DIR/$ENGINE_NAME"
        success "Binary created: $OUTPUT_DIR/$ENGINE_NAME"
    else
        error "Binary not found after build"
    fi
}

# Display results
show_results() {
    echo ""
    echo "========================================="
    echo "  Cross-Compilation Complete!"
    echo "========================================="
    echo ""
    echo "Binary location:"
    echo "  $OUTPUT_DIR/$ENGINE_NAME"
    echo ""
    echo "Architecture: $ARCH"
    echo ""
    echo "Note: This binary is cross-compiled for macOS."
    echo "Test it on actual macOS hardware to verify it works correctly."
    echo ""
    echo "To copy to a Mac:"
    echo "  scp $OUTPUT_DIR/$ENGINE_NAME user@mac-host:~/"
    echo ""
}

# Main
main() {
    case "$1" in
        setup)
            show_setup
            ;;
        clean)
            clean_build
            ;;
        build|"")
            log "SeaJay Cross-Compilation for macOS"
            check_osxcross
            build_engine
            show_results
            ;;
        *)
            echo "Usage: $0 [setup|build|clean]"
            echo "  setup - show installation instructions"
            echo "  build - build the engine (default)"
            echo "  clean - remove build directory"
            exit 1
            ;;
    esac
}

main "$@"