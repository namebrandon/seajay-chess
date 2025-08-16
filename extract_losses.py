#!/usr/bin/env python3
"""Extract all Stage 15 loss games from the PGN file for analysis."""

import re
import sys

def parse_pgn_file(filename):
    """Parse PGN file and extract all games where Stage 15 lost."""
    
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
            elif in_game and line:
                current_game.append(line)
            elif in_game and not line:
                # End of game
                game_data = parse_game(current_game)
                if game_data and is_stage15_loss(game_data):
                    stage15_losses.append(game_data)
                current_game = []
                in_game = False
        
        # Handle last game
        if current_game:
            game_data = parse_game(current_game)
            if game_data and is_stage15_loss(game_data):
                stage15_losses.append(game_data)
    
    return stage15_losses

def parse_game(game_lines):
    """Parse a single game's lines into structured data."""
    game_data = {}
    moves = []
    
    for line in game_lines:
        if line.startswith('['):
            # Header
            match = re.match(r'\[(\w+)\s+"([^"]+)"\]', line)
            if match:
                key, value = match.groups()
                game_data[key] = value
        elif not line.startswith('[') and line:
            # Moves
            moves.append(line)
    
    game_data['moves'] = ' '.join(moves)
    return game_data

def is_stage15_loss(game_data):
    """Check if this is a game where Stage 15 lost (result 0-1)."""
    if game_data.get('Result') != '0-1':
        return False
    
    # Stage 15 loses when:
    # 1. Stage 15 is White and result is 0-1
    # 2. Stage 15 is Black and result is 1-0 (but we're looking for 0-1, so this means Stage 15 was White)
    
    white = game_data.get('White', '')
    black = game_data.get('Black', '')
    
    # For 0-1 result, White lost. Check if White was Stage 15
    return 'Stage15' in white or 'stage15' in white.lower()

def analyze_losses(losses):
    """Analyze the loss patterns."""
    print(f"Total Stage 15 losses: {len(losses)}")
    print()
    
    # Color analysis
    white_losses = sum(1 for game in losses if 'Stage15' in game.get('White', ''))
    black_losses = len(losses) - white_losses
    
    print(f"Stage 15 losses as White: {white_losses}")
    print(f"Stage 15 losses as Black: {black_losses}")
    print()
    
    # Opening analysis
    openings = {}
    for game in losses:
        opening = game.get('Opening', 'Unknown')
        eco = game.get('ECO', 'Unknown')
        key = f"{eco}: {opening}"
        openings[key] = openings.get(key, 0) + 1
    
    print("Opening distribution in losses:")
    for opening, count in sorted(openings.items(), key=lambda x: x[1], reverse=True):
        print(f"  {opening}: {count}")
    print()
    
    # Game length analysis
    plycounts = [int(game.get('PlyCount', '0')) for game in losses if game.get('PlyCount', '').isdigit()]
    if plycounts:
        avg_plycount = sum(plycounts) / len(plycounts)
        print(f"Average game length: {avg_plycount:.1f} plies")
        print(f"Shortest loss: {min(plycounts)} plies")
        print(f"Longest loss: {max(plycounts)} plies")
    
    return losses

def print_sample_games(losses, num_games=3):
    """Print sample loss games for detailed analysis."""
    print(f"\n=== SAMPLE LOSS GAMES ===")
    
    for i, game in enumerate(losses[:num_games]):
        print(f"\nGame {i+1}:")
        print(f"White: {game.get('White', 'Unknown')}")
        print(f"Black: {game.get('Black', 'Unknown')}")
        print(f"Result: {game.get('Result', 'Unknown')}")
        print(f"Opening: {game.get('ECO', 'Unknown')} - {game.get('Opening', 'Unknown')}")
        print(f"Plies: {game.get('PlyCount', 'Unknown')}")
        print(f"Duration: {game.get('GameDuration', 'Unknown')}")
        print(f"Moves: {game.get('moves', '')[:200]}...")

if __name__ == "__main__":
    losses = parse_pgn_file('/workspace/sprt_results/stage15_tuned_vs_stage14/games.pgn')
    analyze_losses(losses)
    print_sample_games(losses, 5)