#!/usr/bin/env python3

import re
import subprocess
from collections import defaultdict

def parse_pgn_moves(pgn_content):
    """Parse PGN content and extract games with moves."""
    games = []
    current_game = None
    
    lines = pgn_content.strip().split('\n')
    i = 0
    
    while i < len(lines):
        line = lines[i].strip()
        
        if line.startswith('['):
            # Header line
            if line.startswith('[Event'):
                if current_game and 'moves' in current_game:
                    games.append(current_game)
                current_game = {'headers': {}, 'moves': ''}
            
            if current_game:
                match = re.match(r'\[(\w+)\s+"([^"]+)"\]', line)
                if match:
                    current_game['headers'][match.group(1)] = match.group(2)
        elif line and not line.startswith('[') and current_game:
            # Moves line
            current_game['moves'] += ' ' + line
        
        i += 1
    
    if current_game and 'moves' in current_game:
        games.append(current_game)
    
    return games

def extract_move_pairs(moves_text):
    """Extract move pairs with evaluations from the game text."""
    move_pairs = []
    
    # Pattern to match moves with evaluations like: Bd6 {+0.32 20/0 1220 1942218}
    pattern = r'([A-Z]?[a-h]?[1-8]?x?[a-h][1-8][=QRBN]?(?:\+{1,2}|#)?)\s*\{([+-]?[0-9.]+|[+-]?M\d+)'
    
    moves = re.findall(pattern, moves_text)
    
    for i in range(0, len(moves), 2):
        if i+1 < len(moves):
            white_move, white_eval = moves[i]
            black_move, black_eval = moves[i+1]
            
            # Convert evaluations to numeric values
            white_eval_num = parse_eval(white_eval)
            black_eval_num = parse_eval(black_eval)
            
            move_pairs.append({
                'move_num': i//2 + 1,
                'white_move': white_move,
                'white_eval': white_eval_num,
                'black_move': black_move,
                'black_eval': black_eval_num
            })
    
    return move_pairs

def parse_eval(eval_str):
    """Convert evaluation string to numeric value."""
    if 'M' in eval_str:
        # Mate score
        mate_num = int(re.search(r'M(\d+)', eval_str).group(1))
        return 299.0 if '+' in eval_str else -299.0
    else:
        try:
            return float(eval_str)
        except:
            return 0.0

def analyze_position_with_stockfish(fen, depth=20):
    """Use Stockfish to analyze a position."""
    stockfish_path = '/workspace/external/engines/stockfish/stockfish'
    
    process = subprocess.Popen(
        stockfish_path,
        stdin=subprocess.PIPE,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True
    )
    
    commands = f"""position fen {fen}
go depth {depth}
quit
"""
    
    stdout, _ = process.communicate(commands)
    
    # Extract evaluation from output
    eval_match = re.search(r'score cp (-?\d+)', stdout)
    mate_match = re.search(r'score mate (-?\d+)', stdout)
    best_move_match = re.search(r'bestmove (\S+)', stdout)
    
    if mate_match:
        mate_in = int(mate_match.group(1))
        eval_score = 299.0 if mate_in > 0 else -299.0
    elif eval_match:
        eval_score = int(eval_match.group(1)) / 100.0
    else:
        eval_score = 0.0
    
    best_move = best_move_match.group(1) if best_move_match else None
    
    return eval_score, best_move

def analyze_games(pgn_file):
    """Analyze all games in a PGN file."""
    with open(pgn_file, 'r') as f:
        content = f.read()
    
    games = parse_pgn_moves(content)
    
    statistics = {
        'total_games': 0,
        'seajay_wins': 0,
        'seajay_losses': 0,
        'seajay_draws': 0,
        'avg_game_length': 0,
        'blunders': [],  # Moves where eval dropped by >2.0
        'critical_errors': [],  # Moves where eval dropped by >1.0
        'time_trouble_errors': [],
        'opening_problems': [],
        'endgame_problems': []
    }
    
    total_moves = 0
    
    for game_idx, game in enumerate(games):
        statistics['total_games'] += 1
        
        # Determine if SeaJay was white or black
        white_player = game['headers'].get('White', '')
        black_player = game['headers'].get('Black', '')
        result = game['headers'].get('Result', '')
        fen = game['headers'].get('FEN', '')
        
        seajay_is_white = 'SeaJay' in white_player
        seajay_is_black = 'SeaJay' in black_player
        
        if not (seajay_is_white or seajay_is_black):
            continue
        
        # Count results
        if result == '1-0':
            if seajay_is_white:
                statistics['seajay_wins'] += 1
            else:
                statistics['seajay_losses'] += 1
        elif result == '0-1':
            if seajay_is_black:
                statistics['seajay_wins'] += 1
            else:
                statistics['seajay_losses'] += 1
        elif result == '1/2-1/2':
            statistics['seajay_draws'] += 1
        
        # Extract and analyze moves
        move_pairs = extract_move_pairs(game['moves'])
        total_moves += len(move_pairs)
        
        # Analyze for blunders and critical errors
        for i, move_pair in enumerate(move_pairs):
            if seajay_is_white:
                if i > 0:
                    eval_drop = move_pairs[i-1]['black_eval'] - move_pair['white_eval']
                    if eval_drop > 2.0:
                        statistics['blunders'].append({
                            'game': game_idx + 1,
                            'move_num': move_pair['move_num'],
                            'move': move_pair['white_move'],
                            'eval_drop': eval_drop,
                            'position': f"Game {game_idx+1}, move {move_pair['move_num']}",
                            'vs': black_player
                        })
                    elif eval_drop > 1.0:
                        statistics['critical_errors'].append({
                            'game': game_idx + 1,
                            'move_num': move_pair['move_num'],
                            'move': move_pair['white_move'],
                            'eval_drop': eval_drop,
                            'position': f"Game {game_idx+1}, move {move_pair['move_num']}",
                            'vs': black_player
                        })
            
            if seajay_is_black:
                if i > 0:
                    eval_drop = move_pair['black_eval'] - move_pairs[i-1]['white_eval']
                    if eval_drop < -2.0:
                        statistics['blunders'].append({
                            'game': game_idx + 1,
                            'move_num': move_pair['move_num'],
                            'move': move_pair['black_move'],
                            'eval_drop': -eval_drop,
                            'position': f"Game {game_idx+1}, move {move_pair['move_num']}",
                            'vs': white_player
                        })
                    elif eval_drop < -1.0:
                        statistics['critical_errors'].append({
                            'game': game_idx + 1,
                            'move_num': move_pair['move_num'],
                            'move': move_pair['black_move'],
                            'eval_drop': -eval_drop,
                            'position': f"Game {game_idx+1}, move {move_pair['move_num']}",
                            'vs': white_player
                        })
        
        # Check for opening problems (first 10 moves)
        for move_pair in move_pairs[:10]:
            if seajay_is_white and move_pair['white_eval'] < -1.0:
                statistics['opening_problems'].append({
                    'game': game_idx + 1,
                    'move_num': move_pair['move_num'],
                    'eval': move_pair['white_eval'],
                    'vs': black_player
                })
            elif seajay_is_black and move_pair['black_eval'] > 1.0:
                statistics['opening_problems'].append({
                    'game': game_idx + 1,
                    'move_num': move_pair['move_num'],
                    'eval': move_pair['black_eval'],
                    'vs': white_player
                })
    
    if statistics['total_games'] > 0:
        statistics['avg_game_length'] = total_moves / statistics['total_games']
    
    return statistics

def main():
    print("=" * 80)
    print("SEAJAY ENGINE WEAKNESS ANALYSIS")
    print("=" * 80)
    
    # Analyze games vs 4ku
    print("\n1. GAMES VS 4KU (3000+ ELO)")
    print("-" * 40)
    stats_4ku = analyze_games('/workspace/external/2025_08_25-4ku_seajay.pgn')
    
    print(f"Total games: {stats_4ku['total_games']}")
    print(f"SeaJay record: {stats_4ku['seajay_wins']}W - {stats_4ku['seajay_losses']}L - {stats_4ku['seajay_draws']}D")
    print(f"Win rate: {stats_4ku['seajay_wins']/stats_4ku['total_games']*100:.1f}%")
    print(f"Average game length: {stats_4ku['avg_game_length']:.1f} move pairs")
    print(f"Total blunders (>2.0 eval drop): {len(stats_4ku['blunders'])}")
    print(f"Critical errors (>1.0 eval drop): {len(stats_4ku['critical_errors'])}")
    print(f"Opening problems: {len(stats_4ku['opening_problems'])}")
    
    # Analyze games vs Laser
    print("\n2. GAMES VS LASER (3000+ ELO)")
    print("-" * 40)
    stats_laser = analyze_games('/workspace/external/2025_08_25-Laser_seajay.pgn')
    
    print(f"Total games: {stats_laser['total_games']}")
    print(f"SeaJay record: {stats_laser['seajay_wins']}W - {stats_laser['seajay_losses']}L - {stats_laser['seajay_draws']}D")
    print(f"Win rate: {stats_laser['seajay_wins']/stats_laser['total_games']*100:.1f}%")
    print(f"Average game length: {stats_laser['avg_game_length']:.1f} move pairs")
    print(f"Total blunders (>2.0 eval drop): {len(stats_laser['blunders'])}")
    print(f"Critical errors (>1.0 eval drop): {len(stats_laser['critical_errors'])}")
    print(f"Opening problems: {len(stats_laser['opening_problems'])}")
    
    # Combine and analyze patterns
    print("\n3. MAJOR WEAKNESSES IDENTIFIED")
    print("-" * 40)
    
    all_blunders = stats_4ku['blunders'] + stats_laser['blunders']
    all_errors = stats_4ku['critical_errors'] + stats_laser['critical_errors']
    
    print(f"\nTotal blunders across all games: {len(all_blunders)}")
    print(f"Total critical errors: {len(all_errors)}")
    
    # Show worst blunders
    if all_blunders:
        print("\nWorst blunders (top 5):")
        sorted_blunders = sorted(all_blunders, key=lambda x: x['eval_drop'], reverse=True)
        for i, blunder in enumerate(sorted_blunders[:5]):
            print(f"  {i+1}. {blunder['position']}: {blunder['move']} (eval drop: {blunder['eval_drop']:.2f}) vs {blunder['vs']}")
    
    # Analyze patterns
    print("\n4. PATTERN ANALYSIS")
    print("-" * 40)
    
    # Check move distribution of errors
    early_errors = sum(1 for e in all_errors if e['move_num'] <= 15)
    mid_errors = sum(1 for e in all_errors if 15 < e['move_num'] <= 35)
    late_errors = sum(1 for e in all_errors if e['move_num'] > 35)
    
    print(f"Error distribution by game phase:")
    print(f"  Opening (moves 1-15): {early_errors} errors")
    print(f"  Middlegame (moves 16-35): {mid_errors} errors")
    print(f"  Endgame (moves 36+): {late_errors} errors")
    
    # Average search depth analysis from move text
    print("\n5. SEARCH DEPTH ANALYSIS")
    print("-" * 40)
    print("Analyzing search depths from move annotations...")
    
    # This would require parsing the depth from move annotations
    # Format: {eval depth/selective_depth ...}
    
    return stats_4ku, stats_laser

if __name__ == "__main__":
    main()