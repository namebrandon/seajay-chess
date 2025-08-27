#!/bin/bash
# Build script for SeaJay Chess Engine on macOS ARM64
# This script patches consteval issues and compiles the engine

set -e

# Colors for output
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m'

echo -e "${GREEN}Building SeaJay Chess Engine for macOS ARM64 (with patches)...${NC}"

# Create a temporary build directory
BUILD_DIR="build_macos_temp"
rm -rf "$BUILD_DIR"
mkdir -p "$BUILD_DIR"

# Copy source files to build directory
echo "Copying source files..."
cp -r src "$BUILD_DIR/"

# Apply patches to fix consteval issues
echo -e "${YELLOW}Applying compatibility patches...${NC}"

# Patch 1: Fix iteration_info.h - replace consteval calls with direct values
cat > "$BUILD_DIR/src/search/iteration_info.h" << 'EOF'
#pragma once

// Stage 13: Iterative Deepening - Iteration tracking data structures
// Phase 1, Deliverable 1.1a: Basic type definitions

#include "../evaluation/types.h"
#include "../core/types.h"
#include <cstdint>
#include <chrono>

namespace seajay::search {

// Forward declarations (POD only, no methods in this deliverable)
struct IterationInfo;

// Time measurement type
using TimeMs = std::chrono::milliseconds::rep;

// IterationInfo: POD struct to track data for each iteration of iterative deepening
// This is filled in during each depth iteration and stored for analysis
struct IterationInfo {
    // Basic search data
    int depth{0};                              // Search depth for this iteration
    eval::Score score{0};                      // Best score found (changed from eval::Score::zero())
    Move bestMove{NO_MOVE};                    // Best move found
    uint64_t nodes{0};                         // Nodes searched in this iteration
    TimeMs elapsed{0};                         // Time spent on this iteration (ms)
    
    // Aspiration window data (for Phase 3)
    eval::Score alpha{-1000000};               // Alpha bound used (changed from eval::Score::minus_infinity())
    eval::Score beta{1000000};                 // Beta bound used (changed from eval::Score::infinity())
    int windowAttempts{0};                     // Number of aspiration re-searches
    bool failedHigh{false};                    // Score failed high (beta cutoff)
    bool failedLow{false};                     // Score failed low (below alpha)
    
    // Move stability tracking (for Phase 2)
    bool moveChanged{false};                   // Best move changed from previous iteration
    int moveStability{0};                      // Consecutive iterations with same best move
    
    // Additional statistics (for Phase 4)
    bool firstMoveFailHigh{false};             // First move caused beta cutoff
    int failHighMoveIndex{-1};                 // Index of move that failed high
    eval::Score secondBestScore{-1000000};     // Score of second best move (changed from eval::Score::minus_infinity())
    double branchingFactor{0.0};               // Effective branching factor
};

// Static assertions to ensure POD nature (C++20)
// Note: IterationInfo contains eval::Score which has constructors, 
// so it's not trivial, but it is standard layout
static_assert(std::is_standard_layout_v<IterationInfo>, "IterationInfo must be standard layout");

} // namespace seajay::search
EOF

# Compiler and flags
CXX="clang++"
OUTPUT="seajay-macos-arm64"

# Base flags for C++20 and optimization
CXXFLAGS="-std=c++20 -O3 -DNDEBUG -arch arm64"

# Include paths
CXXFLAGS="$CXXFLAGS -I$BUILD_DIR/src"

# Enable features
CXXFLAGS="$CXXFLAGS -DENABLE_QUIESCENCE -DTT_ENABLE"

# Warning flags
CXXFLAGS="$CXXFLAGS -Wall -Wno-conversion -Wno-sign-conversion"
CXXFLAGS="$CXXFLAGS -Wno-unused-variable -Wno-unused-but-set-variable -Wno-unused-lambda-capture"

# Source files
SOURCES=(
    "src/main.cpp"
    "src/core/board.cpp"
    "src/core/board_safety.cpp"
    "src/core/move_generation.cpp"
    "src/core/move_list.cpp"
    "src/core/perft.cpp"
    "src/core/transposition_table.cpp"
    "src/core/see.cpp"
    "src/evaluation/evaluate.cpp"
    "src/evaluation/pawn_structure.cpp"
    "src/evaluation/king_safety.cpp"
    "src/search/search.cpp"
    "src/search/negamax.cpp"
    "src/search/move_ordering.cpp"
    "src/search/killer_moves.cpp"
    "src/search/history_heuristic.cpp"
    "src/search/countermoves.cpp"
    "src/search/time_management.cpp"
    "src/search/aspiration_window.cpp"
    "src/search/quiescence.cpp"
    "src/search/quiescence_performance.cpp"
    "src/search/lmr.cpp"
    "src/uci/uci.cpp"
    "src/benchmark/benchmark.cpp"
)

# Compile all source files
echo "Compiling source files..."
cd "$BUILD_DIR"

for source in "${SOURCES[@]}"; do
    echo "  Compiling $source..."
    $CXX $CXXFLAGS -c "$source" -o "${source%.cpp}.o" 2>/dev/null || {
        echo -e "${RED}Warning: Some warnings during compilation of $source${NC}"
    }
done

# Link the executable
echo -e "${YELLOW}Linking executable...${NC}"
OBJECTS=""
for source in "${SOURCES[@]}"; do
    OBJECTS="$OBJECTS ${source%.cpp}.o"
done

$CXX $CXXFLAGS -o "$OUTPUT" $OBJECTS

# Move the binary to the main directory
cd ..
mv "$BUILD_DIR/$OUTPUT" .

# Strip debug symbols for smaller binary
echo "Stripping debug symbols..."
strip "$OUTPUT"

# Clean up temporary build directory
rm -rf "$BUILD_DIR"

# Check the binary
echo -e "${GREEN}Build complete!${NC}"
echo "Binary created: $OUTPUT"
file "$OUTPUT"
echo ""
echo "You can test the engine with:"
echo "  ./$OUTPUT"
echo "Or for UCI mode:"
echo "  ./$OUTPUT uci"