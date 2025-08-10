#!/bin/bash

# Test script for Bug #003: Promotion Move Handling
# This script tests SeaJay against Stockfish to identify promotion move generation bugs

set -e

echo "================================================"
echo "Bug #003 Test Suite: Promotion Move Handling"
echo "================================================"
echo

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Ensure we're in the right directory
cd /workspace

# Build SeaJay if needed
echo "Building SeaJay..."
if [ ! -d "build" ]; then
    mkdir build
fi
cd build
cmake -DCMAKE_BUILD_TYPE=Debug .. > /dev/null 2>&1
make -j > /dev/null 2>&1
cd ..

# Compile the test program
echo "Compiling test program..."
cd tests
g++ -std=c++20 -I../src test_promotion_bug.cpp ../build/libseajay_core.a -o test_promotion_bug
echo

# Function to test a position with Stockfish
test_with_stockfish() {
    local fen="$1"
    local description="$2"
    
    echo -e "${YELLOW}Testing: $description${NC}"
    echo "FEN: $fen"
    
    # Get Stockfish's move count
    local stockfish_output=$(echo -e "position fen $fen\ngo perft 1\nquit" | ../external/engines/stockfish/stockfish 2>/dev/null)
    local stockfish_nodes=$(echo "$stockfish_output" | grep "Nodes searched" | awk '{print $3}')
    
    echo "Stockfish says: $stockfish_nodes moves"
    
    # Get move list from Stockfish (for debugging)
    if [ "$3" == "verbose" ]; then
        echo "Stockfish move breakdown:"
        echo -e "position fen $fen\ngo perft 1\nquit" | ../external/engines/stockfish/stockfish 2>/dev/null | grep -E "^[a-h][1-8]" | head -20
    fi
    
    echo
}

echo "================================================"
echo "PHASE 1: Stockfish Validation"
echo "================================================"
echo

# Test the critical positions with Stockfish
test_with_stockfish "r3k3/P7/8/8/8/8/8/4K3 w - - 0 1" "Bug #003: Blocked pawn on a7" "verbose"
test_with_stockfish "4k3/P7/8/8/8/8/8/4K3 w - - 0 1" "Valid promotion: Clear a8"
test_with_stockfish "rn2k3/P7/8/8/8/8/8/4K3 w - - 0 1" "Promotion with captures"

echo "================================================"
echo "PHASE 2: SeaJay Testing"
echo "================================================"
echo

# Run SeaJay test
./test_promotion_bug

echo
echo "================================================"
echo "PHASE 3: Direct Comparison"
echo "================================================"
echo

# Function to compare SeaJay and Stockfish
compare_engines() {
    local fen="$1"
    local description="$2"
    
    echo -e "${YELLOW}Comparing: $description${NC}"
    echo "FEN: $fen"
    
    # Get Stockfish count
    local sf_nodes=$(echo -e "position fen $fen\ngo perft 1\nquit" | ../external/engines/stockfish/stockfish 2>/dev/null | grep "Nodes searched" | awk '{print $3}')
    
    # Get SeaJay count using perft
    local sj_output=$(echo -e "$fen" | ../build/bin/seajay perft 1 2>/dev/null || echo "ERROR")
    
    # Try to extract move count from SeaJay output
    # This assumes SeaJay outputs in a format we can parse
    # Adjust based on actual SeaJay output format
    
    echo "Stockfish: $sf_nodes moves"
    echo "SeaJay output:"
    echo "$sj_output"
    
    if [ "$sf_nodes" == "$sj_nodes" ]; then
        echo -e "${GREEN}[MATCH]${NC}"
    else
        echo -e "${RED}[MISMATCH]${NC}"
    fi
    echo
}

# Test critical positions
echo "Critical position (Bug #003):"
compare_engines "r3k3/P7/8/8/8/8/8/4K3 w - - 0 1" "Blocked pawn a7"

echo "================================================"
echo "ANALYSIS COMPLETE"
echo "================================================"
echo

echo "To debug further, you can:"
echo "1. Add debug output to move_generation.cpp line 234:"
echo "   if (rankOf(from) == 6 && us == WHITE) {"
echo "       std::cerr << \"Checking square \" << squareToString(to) "
echo "            << \": occupied=\" << (occupied & squareBB(to)) << std::endl;"
echo "   }"
echo
echo "2. Run the test program with gdb:"
echo "   gdb ./test_promotion_bug"
echo "   break move_generation.cpp:236"
echo "   run"
echo
echo "3. Check the specific bitboard values:"
echo "   print occupied"
echo "   print squareBB(56)  # a8 square"
echo "   print occupied & squareBB(56)"