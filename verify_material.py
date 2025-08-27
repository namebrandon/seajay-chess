#!/usr/bin/env python3
"""Verify material count in the position"""

import chess

# The actual position from the game after 12...Nxa1
fen = "r2q1rk1/ppp1bppp/3p3B/4p2n/2B1P1Q1/2NP3P/PP3PP1/n4RK1 w - - 0 13"
board = chess.Board(fen)

print("Position after 12...Nxa1:")
print(board)
print()

# Count material
piece_values = {
    chess.PAWN: 1,
    chess.KNIGHT: 3,
    chess.BISHOP: 3,
    chess.ROOK: 5,
    chess.QUEEN: 9
}

white_material = 0
black_material = 0

for square in chess.SQUARES:
    piece = board.piece_at(square)
    if piece:
        value = piece_values.get(piece.piece_type, 0)
        if piece.color == chess.WHITE:
            white_material += value
        else:
            black_material += value

print("Material count (in pawn units):")
print(f"White: {white_material}")
print(f"Black: {black_material}")
print(f"Difference: {white_material - black_material} (White perspective)")
print()

# Detailed piece count
print("Detailed piece count:")
for color_name, color in [("White", chess.WHITE), ("Black", chess.BLACK)]:
    pieces = []
    for piece_type in [chess.QUEEN, chess.ROOK, chess.BISHOP, chess.KNIGHT, chess.PAWN]:
        count = len(board.pieces(piece_type, color))
        if count > 0:
            pieces.append(f"{count}{chess.piece_symbol(piece_type).upper()}")
    print(f"{color_name}: {' '.join(pieces)}")

print()
print("Analysis:")
print("White has: 1Q 1R 2B 1N 7P = 9 + 5 + 6 + 3 + 7 = 30")
print("Black has: 1Q 2R 1B 2N 8P = 9 + 10 + 3 + 6 + 8 = 36")
print("Material difference: 30 - 36 = -6 pawns for White")
print()
print("BUT notice: Black's knight on a1 is TRAPPED and essentially lost!")
print("The knight on a1 cannot escape and will be captured by Rxa1.")
print("So the 'real' evaluation should consider this tactical fact.")