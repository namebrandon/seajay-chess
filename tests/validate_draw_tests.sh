#!/bin/bash

# Stockfish Draw Detection Validation Script
# Validates all draw detection test cases against Stockfish 16
# Usage: ./validate_draw_tests.sh

STOCKFISH="/workspace/external/engines/stockfish/stockfish"

# Color codes for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo "========================================="
echo "STOCKFISH DRAW DETECTION VALIDATION"
echo "========================================="

# Function to check if position is draw in Stockfish
check_draw() {
    local position="$1"
    local expected="$2"
    local test_name="$3"
    
    echo -e "\n${YELLOW}Test: $test_name${NC}"
    echo "Position: $position"
    
    # Get Stockfish evaluation
    result=$(echo -e "$position\nd\nquit" | $STOCKFISH 2>/dev/null | grep -E "(Draw|Checkmate|Check)" || echo "Not Draw")
    
    # Check for specific draw types
    if echo "$result" | grep -q "Draw by repetition"; then
        draw_type="repetition"
        is_draw="true"
    elif echo "$result" | grep -q "Draw by 50-move rule"; then
        draw_type="fifty-move"
        is_draw="true"
    elif echo "$result" | grep -q "Draw by insufficient material"; then
        draw_type="insufficient"
        is_draw="true"
    elif echo "$result" | grep -q "Stalemate"; then
        draw_type="stalemate"
        is_draw="true"
    elif echo "$result" | grep -q "Checkmate"; then
        draw_type="checkmate"
        is_draw="false"
    else
        draw_type="none"
        is_draw="false"
    fi
    
    if [ "$is_draw" = "$expected" ]; then
        echo -e "${GREEN}✓ PASS${NC}: Draw detection matches (Expected: $expected, Got: $is_draw, Type: $draw_type)"
    else
        echo -e "${RED}✗ FAIL${NC}: Draw detection mismatch (Expected: $expected, Got: $is_draw, Type: $draw_type)"
    fi
    
    # Also show Stockfish's evaluation for reference
    echo -e "$position\ngo depth 1\nquit" | $STOCKFISH 2>/dev/null | grep -E "(cp|mate)" | head -1
}

# ============================================================================
# THREEFOLD REPETITION TESTS
# ============================================================================

echo -e "\n${YELLOW}=== THREEFOLD REPETITION TESTS ===${NC}"

check_draw "position startpos moves Nc3 Nc6 Nb1 Nb8 Nc3 Nc6 Nb1 Nb8 Nc3" "true" "Basic Knight Shuttling"

check_draw "position fen 8/8/8/8/8/8/8/6K1 w - - 0 1 moves Kh1 Kh8 Kh2 Kg8 Kg1 Kh8 Kh1 Kg8 Kh2 Kh8 Kg1" "true" "King Triangulation"

check_draw "position fen r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1 moves Ra2 Ra7 Ra1 Ra8 Ra2 Ra7 Ra1 Ra8 Ke2" "false" "Castling Rights Change"

# ============================================================================
# FIFTY-MOVE RULE TESTS
# ============================================================================

echo -e "\n${YELLOW}=== FIFTY-MOVE RULE TESTS ===${NC}"

check_draw "position fen 8/8/8/8/3K4/8/3k4/8 w - - 100 50" "true" "Exactly 100 halfmoves"

check_draw "position fen 8/8/8/8/3K4/8/3k4/8 w - - 99 50" "false" "99 halfmoves"

check_draw "position fen 8/8/8/8/3K4/3P4/3k4/8 w - - 0 75" "false" "Pawn move reset"

# ============================================================================
# INSUFFICIENT MATERIAL TESTS
# ============================================================================

echo -e "\n${YELLOW}=== INSUFFICIENT MATERIAL TESTS ===${NC}"

check_draw "position fen 8/8/8/3k4/8/3K4/8/8 w - - 0 1" "true" "K vs K"

check_draw "position fen 8/8/8/3k4/8/3KN3/8/8 w - - 0 1" "true" "KN vs K"

check_draw "position fen 8/8/8/3k4/8/3KB3/8/8 w - - 0 1" "true" "KB vs K"

check_draw "position fen 8/2b5/8/3k4/8/3KB3/8/8 w - - 0 1" "true" "KB vs KB same color"

check_draw "position fen 8/3b4/8/3k4/8/3KB3/8/8 w - - 0 1" "false" "KB vs KB opposite color"

check_draw "position fen 8/8/8/3k4/8/3KNN2/8/8 w - - 0 1" "false" "KNN vs K"

check_draw "position fen 8/8/8/3k4/8/3KP3/8/8 w - - 0 1" "false" "KP vs K"

# ============================================================================
# PERFT VALIDATION FOR DRAW POSITIONS
# ============================================================================

echo -e "\n${YELLOW}=== PERFT VALIDATION FOR DRAWS ===${NC}"

# Test that moves are still generated correctly in draw positions
echo "Testing move generation in draw position (should still generate legal moves):"
echo -e "position fen 8/8/8/3k4/8/3K4/8/8 w - - 100 50\ngo perft 1\nquit" | $STOCKFISH 2>/dev/null | grep "Nodes searched"

echo -e "\n========================================="
echo "VALIDATION COMPLETE"
echo "========================================="