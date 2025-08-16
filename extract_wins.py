#!/usr/bin/env python3
"""Extract all Stage 15 win games from the PGN file for analysis."""

import re
import sys

def parse_pgn_file(filename):
    """Parse PGN file and extract all games where Stage 15 won."""
    
    stage15_wins = []
    current_game = []
    in_game = False
    
    with open(filename, 'r') as f:
        for line in f:
            line = line.strip()
            
            if line.startswith('[Event'):
                if current_game:
                    # Process previous game
                    game_data = parse_game(current_game)
                    if game_data and is_stage15_win(game_data):
                        stage15_wins.append(game_data)
                
                # Start new game
                current_game = [line]
                in_game = True
            elif in_game and line:
                current_game.append(line)
            elif in_game and not line:
                # End of game
                game_data = parse_game(current_game)
                if game_data and is_stage15_win(game_data):
                    stage15_wins.append(game_data)
                current_game = []
                in_game = False
        
        # Handle last game
        if current_game:
            game_data = parse_game(current_game)
            if game_data and is_stage15_win(game_data):
                stage15_wins.append(game_data)
    
    return stage15_wins

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

def is_stage15_win(game_data):
    """Check if this is a game where Stage 15 won."""
    result = game_data.get('Result', '')
    white = game_data.get('White', '')
    black = game_data.get('Black', '')
    
    # Stage 15 wins when:
    # 1. Stage 15 is White and result is 1-0
    # 2. Stage 15 is Black and result is 0-1
    
    if result == '1-0' and ('Stage15' in white):
        return True
    elif result == '0-1' and ('Stage15' in black):
        return True
    
    return False

def analyze_wins(wins):
    """Analyze the win patterns."""
    print(f"Total Stage 15 wins: {len(wins)}")
    print()
    
    # Color analysis
    white_wins = sum(1 for game in wins if 'Stage15' in game.get('White', '') and game.get('Result') == '1-0')
    black_wins = len(wins) - white_wins
    
    print(f"Stage 15 wins as White: {white_wins}")
    print(f"Stage 15 wins as Black: {black_wins}")
    print()
    
    # Opening analysis
    openings = {}
    for game in wins:
        opening = game.get('Opening', 'Unknown')
        eco = game.get('ECO', 'Unknown')
        key = f"{eco}: {opening}"
        openings[key] = openings.get(key, 0) + 1
    
    print("Opening distribution in wins:")
    for opening, count in sorted(openings.items(), key=lambda x: x[1], reverse=True):
        print(f"  {opening}: {count}")
    print()
    
    return wins

if __name__ == "__main__":
    wins = parse_pgn_file('/workspace/sprt_results/stage15_tuned_vs_stage14/games.pgn')
    analyze_wins(wins)