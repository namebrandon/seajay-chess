#!/bin/bash

# Stockfish validation script for Stage 9b draw positions
STOCKFISH="/workspace/external/engines/stockfish/stockfish"

echo "=== Stockfish Draw Detection Validation ==="
echo "Testing various draw scenarios with Stockfish for comparison"
echo

# Test 1: Basic Threefold Repetition
echo "Test 1: Basic Threefold Repetition"
echo "Position after: Nc3 Nc6 Nb1 Nb8 Nc3 Nc6 Nb1 Nb8 Nc3"
echo -e "position startpos moves b1c3 b8c6 c3b1 c6b8 b1c3 b8c6 c3b1 c6b8 b1c3\nd\nquit" | $STOCKFISH 2>/dev/null | grep -E "Fen:|Repetition|Checkers"
echo

# Test 2: Fifty-Move Rule at 100 halfmoves
echo "Test 2: Fifty-Move Rule at 100 halfmoves"
echo -e "position fen \"8/8/8/4k3/8/3K4/8/8 w - - 100 1\"\nd\nquit" | $STOCKFISH 2>/dev/null | grep -E "Fen:|Halfmoves|Rule50"
echo

# Test 3: Fifty-Move Rule at 99 halfmoves (not yet draw)
echo "Test 3: Fifty-Move Rule at 99 halfmoves (should NOT be draw)"
echo -e "position fen \"8/8/8/4k3/8/3K4/8/8 w - - 99 1\"\nd\nquit" | $STOCKFISH 2>/dev/null | grep -E "Fen:|Halfmoves|Rule50"
echo

# Test 4: Insufficient Material K vs K
echo "Test 4: Insufficient Material K vs K"
echo -e "position fen \"8/8/8/4k3/8/3K4/8/8 w - - 0 1\"\neval\nquit" | $STOCKFISH 2>/dev/null | head -20 | grep -E "Total|Final|Material"
echo

# Test 5: Insufficient Material KN vs K
echo "Test 5: Insufficient Material KN vs K"
echo -e "position fen \"8/8/8/4k3/8/3K4/8/N7 w - - 0 1\"\neval\nquit" | $STOCKFISH 2>/dev/null | head -20 | grep -E "Total|Final|Material"
echo

# Test 6: Insufficient Material KB vs K
echo "Test 6: Insufficient Material KB vs K"
echo -e "position fen \"8/8/8/4k3/8/3K4/B7/8 w - - 0 1\"\neval\nquit" | $STOCKFISH 2>/dev/null | head -20 | grep -E "Total|Final|Material"
echo

# Test 7: Insufficient Material KB vs KB (same color squares)
echo "Test 7: Insufficient Material KB vs KB (same color - both light squares)"
echo -e "position fen \"8/8/8/4k3/2b5/8/B7/3K4 w - - 0 1\"\neval\nquit" | $STOCKFISH 2>/dev/null | head -20 | grep -E "Total|Final|Material"
echo

# Test 8: Sufficient Material KB vs KB (opposite color squares)
echo "Test 8: Sufficient Material KB vs KB (opposite colors)"
echo -e "position fen \"8/8/8/4k3/8/1b6/B7/3K4 w - - 0 1\"\neval\nquit" | $STOCKFISH 2>/dev/null | head -20 | grep -E "Total|Final|Material"
echo

# Test 9: Testing castling rights affecting repetition
echo "Test 9: Position with castling rights"
echo "Initial position with castling:"
echo -e "position fen \"r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1\"\nd\nquit" | $STOCKFISH 2>/dev/null | grep -E "Fen:|Castling"
echo "After Ra1-a2-a1 (loses queenside castling):"
echo -e "position fen \"r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1\" moves a1a2 a8a7 a2a1 a7a8\nd\nquit" | $STOCKFISH 2>/dev/null | grep -E "Fen:|Castling"
echo

# Test 10: En passant square test
echo "Test 10: En passant square existence"
echo "After c7-c5 creating en passant square:"
echo -e "position fen \"8/2p5/3p4/KP6/7r/8/8/k7 w - - 0 1\" moves b5b6 c7c5\nd\nquit" | $STOCKFISH 2>/dev/null | grep -E "Fen:|En passant"
echo

echo "=== Perft validation for complex positions ==="
echo

# Perft test for a position with potential repetitions
echo "Perft 3 from a game position (for move generation validation):"
echo -e "position fen \"r1bqkb1r/pppp1ppp/2n2n2/4p3/2B1P3/5N2/PPPP1PPP/RNBQK2R w KQkq - 0 1\"\ngo perft 3\nquit" | $STOCKFISH 2>/dev/null | grep "Nodes"

echo
echo "Validation complete. Compare these results with SeaJay's output."
echo "Note: Stockfish may show different evaluation values but should agree on:"
echo "  - Whether positions are drawn by repetition/fifty-move/insufficient material"
echo "  - Castling rights changes"
echo "  - En passant square presence"