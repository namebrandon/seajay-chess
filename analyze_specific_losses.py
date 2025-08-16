#!/usr/bin/env python3
"""Analyze specific loss games to understand the regression patterns."""

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
            elif not in_game and not line:
                continue
        
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
    
    # Stage 15 loses when:
    # 1. Stage 15 is White and result is 0-1
    # 2. Stage 15 is Black and result is 1-0
    
    if result == '0-1' and 'Stage15' in white:
        return True
    elif result == '1-0' and 'Stage15' in black:
        return True
    
    return False

def analyze_loss_game(game, game_num):
    """Analyze a specific loss game for patterns."""
    print(f"\n=== LOSS GAME #{game_num} ===")
    print(f"White: {game.get('White', 'Unknown')}")
    print(f"Black: {game.get('Black', 'Unknown')}")
    print(f"Result: {game.get('Result', 'Unknown')}")
    print(f"Opening: {game.get('ECO', '?')} - {game.get('Opening', 'Unknown')}")
    print(f"Plies: {game.get('PlyCount', '?')}")
    print(f"Duration: {game.get('GameDuration', '?')}")
    print(f"Termination: {game.get('Termination', 'Unknown')}")
    
    # Analyze the moves
    moves = game.get('moves', '')
    
    # Look for evaluation patterns
    eval_pattern = r'[+-]?\d+\.\d+/\d+'
    evals = re.findall(eval_pattern, moves)
    
    if evals:
        print(f"Evaluation progression (first 10): {evals[:10]}")
        
        # Look for evaluation drops
        stage15_white = 'Stage15' in game.get('White', '')
        
        # Extract evaluations and depths
        eval_data = []
        for eval_str in evals[:20]:  # First 20 evaluations
            try:
                if '/' in eval_str:
                    eval_val, depth = eval_str.split('/')
                    eval_data.append((float(eval_val), int(depth)))
            except:
                continue
        
        if eval_data:
            print(f"Early evaluations: {eval_data[:8]}")
            
            # Look for significant drops
            for i in range(1, min(len(eval_data), 15)):
                prev_eval, prev_depth = eval_data[i-1]
                curr_eval, curr_depth = eval_data[i]
                
                # Adjust for perspective (Stage15 as White = positive good, as Black = negative good)
                if stage15_white:
                    drop = prev_eval - curr_eval
                else:
                    drop = curr_eval - prev_eval
                
                if drop > 2.0:  # Significant evaluation drop
                    print(f"  BIG DROP at move {i+1}: {prev_eval} -> {curr_eval} (drop: {drop:.2f})")
    
    # Look for short games (tactical blunders)
    ply_count = int(game.get('PlyCount', '0'))
    if ply_count < 40:
        print(f"  WARNING: Very short game ({ply_count} plies) - likely tactical blunder")
    
    # Check for timeout
    if 'timeout' in game.get('Termination', '').lower():
        print(f"  WARNING: Game ended by timeout")
    
    # Print the actual moves for very short games
    if ply_count < 30:
        print(f"Full game moves:")
        print(f"  {moves}")

def main():
    losses = parse_pgn_file('/workspace/sprt_results/stage15_tuned_vs_stage14/games.pgn')
    
    print(f"Found {len(losses)} Stage 15 losses")
    
    # Analyze each loss
    for i, game in enumerate(losses):
        analyze_loss_game(game, i+1)
    
    # Summary analysis
    print(f"\n=== LOSS SUMMARY ===")
    
    # Game length distribution
    ply_counts = [int(game.get('PlyCount', '0')) for game in losses if game.get('PlyCount', '').isdigit()]
    short_games = sum(1 for p in ply_counts if p < 40)
    medium_games = sum(1 for p in ply_counts if 40 <= p < 80)
    long_games = sum(1 for p in ply_counts if p >= 80)
    
    print(f"Game length distribution:")
    print(f"  Short games (<40 plies): {short_games}")
    print(f"  Medium games (40-79 plies): {medium_games}")
    print(f"  Long games (80+ plies): {long_games}")
    
    # Color analysis
    white_losses = sum(1 for game in losses if 'Stage15' in game.get('White', ''))
    black_losses = len(losses) - white_losses
    
    print(f"Color distribution:")
    print(f"  Losses as White: {white_losses}")
    print(f"  Losses as Black: {black_losses}")

if __name__ == "__main__":
    main()