#!/usr/bin/env python3
"""Parse PGN and show position after specific moves"""

import chess
import chess.pgn

# Read the PGN file
with open("/workspace/external/human-games-20250827.pgn", "r") as f:
    # Skip first game, go to second game
    game1 = chess.pgn.read_game(f)
    game2 = chess.pgn.read_game(f)

print("Game 2: SeaJay (White) vs Brandon Harris (Black)")
print("="*60)

# Create a board and replay moves
board = chess.Board()

# Play through the moves up to move 10...Nxc2
moves_list = list(game2.mainline_moves())

# Play moves up to and including 10...Nxc2
move_count = 0
for i, move in enumerate(moves_list):
    board.push(move)
    move_count += 1
    
    # After move 20 (10...Nxc2 - Black's 10th move)
    if move_count == 20:  # Move 20 is 10...Nxc2
        print(f"\nPosition after 10...Nxc2 (Black knight takes c2, forking Queen and Rook):")
        print(board)
        print(f"\nFEN: {board.fen()}")
        print(f"\nMaterial count:")
        print(f"White: Q={len(board.pieces(chess.QUEEN, chess.WHITE))} R={len(board.pieces(chess.ROOK, chess.WHITE))} B={len(board.pieces(chess.BISHOP, chess.WHITE))} N={len(board.pieces(chess.KNIGHT, chess.WHITE))} P={len(board.pieces(chess.PAWN, chess.WHITE))}")
        print(f"Black: Q={len(board.pieces(chess.QUEEN, chess.BLACK))} R={len(board.pieces(chess.ROOK, chess.BLACK))} B={len(board.pieces(chess.BISHOP, chess.BLACK))} N={len(board.pieces(chess.KNIGHT, chess.BLACK))} P={len(board.pieces(chess.PAWN, chess.BLACK))}")
        print("\nNote: Black knight on c2 is forking White's Queen on g3 and Rook on a1")
        
    # After move 21 (11.Bh6)
    if move_count == 21:
        print(f"\nPosition after 11.Bh6 (White ignores the fork):")
        print(board)
        print(f"\nFEN: {board.fen()}")
        
    # After move 22 (11...Nh5) 
    if move_count == 22:
        print(f"\nPosition after 11...Nh5 (Black moves knight from f6):")
        print(board)
        print(f"\nFEN: {board.fen()}")
        
    # After move 23 (12.Qg4)
    if move_count == 23:
        print(f"\nPosition after 12.Qg4 (White saves Queen):")
        print(board)
        print(f"\nFEN: {board.fen()}")
        
    # After move 24 (12...Nxa1)
    if move_count == 24:
        print(f"\nPosition after 12...Nxa1 (Black captures the Rook):")
        print(board)
        print(f"\nFEN: {board.fen()}")
        print(f"\nMaterial count:")
        print(f"White: Q={len(board.pieces(chess.QUEEN, chess.WHITE))} R={len(board.pieces(chess.ROOK, chess.WHITE))} B={len(board.pieces(chess.BISHOP, chess.WHITE))} N={len(board.pieces(chess.KNIGHT, chess.WHITE))} P={len(board.pieces(chess.PAWN, chess.WHITE))}")
        print(f"Black: Q={len(board.pieces(chess.QUEEN, chess.BLACK))} R={len(board.pieces(chess.ROOK, chess.BLACK))} B={len(board.pieces(chess.BISHOP, chess.BLACK))} N={len(board.pieces(chess.KNIGHT, chess.BLACK))} P={len(board.pieces(chess.PAWN, chess.BLACK))}")
        break