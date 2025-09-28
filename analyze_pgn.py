#!/usr/bin/env python3

import re
import sys
from typing import List, Tuple, Optional

def extract_evaluations(pgn_content: str) -> List[Tuple[int, int, str, int]]:
    """
    Extract evaluation differences from PGN content.
    Returns list of (game_number, move_number, move_text, eval_difference)
    """

    results = []
    game_number = 0

    # Simple approach: split into games by [Event header
    games = re.split(r'\n\[Event ', pgn_content)
    if games[0].startswith('[Event'):
        games[0] = games[0][7:]  # Remove initial [Event

    print(f"Found {len(games)} game sections")

    for game_text in games:
        if not game_text.strip():
            continue

        game_number += 1

        # Look for the actual moves line
        lines = game_text.split('\n')
        moves_line = ""

        for line in lines:
            # Look for the line with actual moves (contains move numbers and evals)
            if re.search(r'\d+\.\s+\w+\s+\{.*\[%eval', line):
                moves_line = line
                break

        if not moves_line:
            continue

        # Parse move pairs with their evaluations
        move_pattern = r'(\d+)\.\s+(\w+(?:\+|#)?)\s+\{[^}]*\[%eval\s+(-?\d+),\d+\][^}]*\}\s+(\w+(?:\+|#)?)\s+\{[^}]*\[%eval\s+(-?\d+),\d+\][^}]*\}'
        move_pairs = re.findall(move_pattern, moves_line)

        for move_data in move_pairs:
            move_num = int(move_data[0])
            white_move = move_data[1]
            white_eval_val = int(move_data[2])
            black_move = move_data[3]
            black_eval_val = int(move_data[4])

            # Skip extreme values (likely mate scores)
            if abs(white_eval_val) > 2000 or abs(black_eval_val) > 2000:
                continue

            # Calculate absolute difference
            eval_diff = abs(white_eval_val - black_eval_val)

            # Create move text
            move_text = f"{move_num}. {white_move} {black_move}"

            results.append((game_number, move_num, move_text, eval_diff, white_eval_val, black_eval_val))

    return results

def main():
    try:
        with open('/workspace/external/20250928-seajay-fritz11.pgn', 'r') as f:
            pgn_content = f.read()
    except Exception as e:
        print(f"Error reading file: {e}")
        return

    print("Analyzing PGN file for evaluation differences...")

    differences = extract_evaluations(pgn_content)

    if not differences:
        print("No evaluation differences found!")
        return

    # Sort by evaluation difference (descending)
    differences.sort(key=lambda x: x[3], reverse=True)

    # Get top 20
    top_20 = differences[:20]

    print(f"\nTop 20 Evaluation Differences (from {len(differences)} total analyzed moves):")
    print("=" * 80)
    print(f"{'Rank':<4} {'Game':<4} {'Move':<15} {'Difference':<10} {'Move Text'}")
    print("-" * 80)

    for i, (game_num, move_num, move_text, diff, white_eval, black_eval) in enumerate(top_20, 1):
        # Truncate move text if too long
        truncated_move = move_text[:40] + "..." if len(move_text) > 40 else move_text
        print(f"{i:<4} {game_num:<4} {move_num:<15} {diff:<10} {truncated_move}")

    print("\nDetailed analysis of top 20:")
    print("=" * 100)

    for i, (game_num, move_num, move_text, diff, white_eval, black_eval) in enumerate(top_20, 1):
        print(f"\n{i}. Game {game_num}, Move {move_num} - Difference: {diff} centipawns")
        print(f"   Move: {move_text}")
        print(f"   White evaluation: {white_eval} cp, Black evaluation: {black_eval} cp")

if __name__ == "__main__":
    main()