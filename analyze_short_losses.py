#!/usr/bin/env python3
"""Analyze the shortest loss games to identify immediate tactical issues."""

import re

def parse_pgn_file(filename):
    """Parse PGN file and extract all Stage 15 loss games with full details."""
    
    stage15_losses = []
    current_game = []
    in_game = False
    
    with open(filename, 'r') as f:
        for line in f:
            line = line.strip()
            
            if line.startswith('[Event'):
                if current_game:
                    # Process previous game
                    game_data = parse_game(current_game)
                    if game_data and is_stage15_loss(game_data):
                        stage15_losses.append(game_data)
                
                # Start new game
                current_game = [line]
                in_game = True
            elif in_game:
                current_game.append(line)
        
        # Handle last game
        if current_game:
            game_data = parse_game(current_game)
            if game_data and is_stage15_loss(game_data):
                stage15_losses.append(game_data)
    
    return stage15_losses

def parse_game(game_lines):
    """Parse a single game's lines into structured data."""
    game_data = {}
    moves_lines = []
    
    for line in game_lines:
        if line.startswith('[') and '"' in line:
            # Header
            match = re.match(r'\[(\w+)\s+"([^"]+)"\]', line)
            if match:
                key, value = match.groups()
                game_data[key] = value
        elif not line.startswith('[') and line:
            # Moves
            moves_lines.append(line)
    
    game_data['moves'] = ' '.join(moves_lines)
    return game_data

def is_stage15_loss(game_data):
    """Check if this is a game where Stage 15 lost."""
    result = game_data.get('Result', '')
    white = game_data.get('White', '')
    black = game_data.get('Black', '')
    
    if result == '0-1' and 'Stage15' in white:
        return True
    elif result == '1-0' and 'Stage15' in black:
        return True
    
    return False

def main():
    losses = parse_pgn_file('/workspace/sprt_results/stage15_tuned_vs_stage14/games.pgn')
    
    # Filter for very short games
    short_losses = []
    for game in losses:
        ply_count = int(game.get('PlyCount', '0'))
        if ply_count < 50:  # Very short games
            short_losses.append((game, ply_count))
    
    # Sort by length
    short_losses.sort(key=lambda x: x[1])
    
    print(f"=== ANALYSIS OF SHORT LOSSES (<50 plies) ===")
    print(f"Found {len(short_losses)} short losses out of {len(losses)} total losses")
    print()
    
    for i, (game, ply_count) in enumerate(short_losses):
        print(f"\n--- SHORT LOSS #{i+1} ---")
        print(f"Plies: {ply_count}")
        print(f"White: {game.get('White', 'Unknown')}")
        print(f"Black: {game.get('Black', 'Unknown')}")
        print(f"Opening: {game.get('ECO', '?')} - {game.get('Opening', 'Unknown')}")
        print(f"Termination: {game.get('Termination', 'Unknown')}")
        print(f"Duration: {game.get('GameDuration', '?')}")
        
        # Show full moves for very short games
        moves = game.get('moves', '')
        if ply_count < 30:
            print(f"FULL GAME:")
            print(f"  {moves}")
        else:
            print(f"MOVES (first 100 chars): {moves[:100]}...")
        
        # Look for timeout
        termination = game.get('Termination', '').lower()
        if 'timeout' in termination or 'time forfeit' in termination:
            print("  *** TIMEOUT LOSS ***")
        
        # Extract first few evaluations
        eval_pattern = r'[+-]?\d+\.\d+/\d+'
        evals = re.findall(eval_pattern, moves)
        if evals and len(evals) >= 4:
            print(f"  First 4 evaluations: {evals[:4]}")

if __name__ == "__main__":
    main()