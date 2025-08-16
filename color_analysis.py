#!/usr/bin/env python3
"""Complete color analysis of all games in the SPRT test."""

import re

def parse_pgn_file(filename):
    """Parse PGN file and extract all games."""
    
    all_games = []
    current_game = []
    in_game = False
    
    with open(filename, 'r') as f:
        for line in f:
            line = line.strip()
            
            if line.startswith('[Event'):
                if current_game:
                    # Process previous game
                    game_data = parse_game(current_game)
                    if game_data:
                        all_games.append(game_data)
                
                # Start new game
                current_game = [line]
                in_game = True
            elif in_game and line:
                current_game.append(line)
            elif in_game and not line:
                # End of game
                game_data = parse_game(current_game)
                if game_data:
                    all_games.append(game_data)
                current_game = []
                in_game = False
        
        # Handle last game
        if current_game:
            game_data = parse_game(current_game)
            if game_data:
                all_games.append(game_data)
    
    return all_games

def parse_game(game_lines):
    """Parse a single game's lines into structured data."""
    game_data = {}
    
    for line in game_lines:
        if line.startswith('['):
            # Header
            match = re.match(r'\[(\w+)\s+"([^"]+)"\]', line)
            if match:
                key, value = match.groups()
                game_data[key] = value
    
    return game_data

def analyze_all_games(games):
    """Complete analysis of all game results by color."""
    
    # Initialize counters
    stage15_white_wins = 0
    stage15_white_draws = 0
    stage15_white_losses = 0
    stage15_black_wins = 0
    stage15_black_draws = 0
    stage15_black_losses = 0
    
    total_games = len(games)
    
    for game in games:
        white = game.get('White', '')
        black = game.get('Black', '')
        result = game.get('Result', '')
        
        if 'Stage15' in white:
            # Stage 15 playing as White
            if result == '1-0':
                stage15_white_wins += 1
            elif result == '1/2-1/2':
                stage15_white_draws += 1
            elif result == '0-1':
                stage15_white_losses += 1
        
        elif 'Stage15' in black:
            # Stage 15 playing as Black
            if result == '0-1':
                stage15_black_wins += 1
            elif result == '1/2-1/2':
                stage15_black_draws += 1
            elif result == '1-0':
                stage15_black_losses += 1
    
    print(f"=== COMPLETE GAME ANALYSIS ===")
    print(f"Total games: {total_games}")
    print()
    
    print(f"Stage 15 as WHITE:")
    print(f"  Wins: {stage15_white_wins}")
    print(f"  Draws: {stage15_white_draws}")
    print(f"  Losses: {stage15_white_losses}")
    total_white = stage15_white_wins + stage15_white_draws + stage15_white_losses
    if total_white > 0:
        white_score = (stage15_white_wins + 0.5 * stage15_white_draws) / total_white
        print(f"  Score: {white_score:.3f} ({white_score*100:.1f}%)")
    print()
    
    print(f"Stage 15 as BLACK:")
    print(f"  Wins: {stage15_black_wins}")
    print(f"  Draws: {stage15_black_draws}")
    print(f"  Losses: {stage15_black_losses}")
    total_black = stage15_black_wins + stage15_black_draws + stage15_black_losses
    if total_black > 0:
        black_score = (stage15_black_wins + 0.5 * stage15_black_draws) / total_black
        print(f"  Score: {black_score:.3f} ({black_score*100:.1f}%)")
    print()
    
    total_wins = stage15_white_wins + stage15_black_wins
    total_draws = stage15_white_draws + stage15_black_draws
    total_losses = stage15_white_losses + stage15_black_losses
    overall_total = total_wins + total_draws + total_losses
    
    print(f"OVERALL STAGE 15 PERFORMANCE:")
    print(f"  Total Wins: {total_wins}")
    print(f"  Total Draws: {total_draws}")
    print(f"  Total Losses: {total_losses}")
    if overall_total > 0:
        overall_score = (total_wins + 0.5 * total_draws) / overall_total
        print(f"  Overall Score: {overall_score:.3f} ({overall_score*100:.1f}%)")
    
    return {
        'stage15_white_wins': stage15_white_wins,
        'stage15_white_draws': stage15_white_draws,
        'stage15_white_losses': stage15_white_losses,
        'stage15_black_wins': stage15_black_wins,
        'stage15_black_draws': stage15_black_draws,
        'stage15_black_losses': stage15_black_losses
    }

if __name__ == "__main__":
    games = parse_pgn_file('/workspace/sprt_results/stage15_tuned_vs_stage14/games.pgn')
    stats = analyze_all_games(games)