#!/usr/bin/env python3

import re
import subprocess
import chess
import chess.pgn
from io import StringIO

def analyze_critical_positions():
    """Analyze specific critical positions from the games."""
    
    critical_positions = [
        {
            'name': 'Game 1 vs 4ku - Tactical miss leading to mate',
            'fen': 'r1b1kb1r/ppp2ppp/5n2/3pq3/8/3B4/PPPNNPPP/R1BQK2R b KQkq - 0 1',
            'seajay_move': 'Nf3',
            'position_after': 'Position after Black played Bd6',
            'game_context': 'SeaJay as White made Nf3, evaluated at -2.24 (depth 8)'
        },
        {
            'name': 'Game 3 vs 4ku - Pawn storm failure',
            'fen': 'r2qkb1r/pp1npppp/2pp1n2/8/2PP4/4PB1P/PP3PP1/RNBQ1RK1 b kq - 0 1',
            'seajay_move': 'g4',
            'position_after': 'Position after Black played e6',
            'game_context': 'SeaJay as White played g4, initiating wrong plan'
        },
        {
            'name': 'Game 7 vs 4ku - Tactical blindness',
            'fen': 'r2qkb1r/pp2pppp/3p1n2/2p3Bb/2B1P3/3P1n1P/PPPN1PP1/R2Q1RK1 w kq - 0 1',
            'seajay_move': 'Nxf3',
            'position_after': 'Losing position after Ne4 invasion',
            'game_context': 'SeaJay missed the knight fork threat'
        },
        {
            'name': 'Game 1 vs Laser - King safety disaster',
            'fen': 'r3kb1r/ppp2ppp/2nq1n2/1B1ppb2/6P1/P1N1PN1P/1PPP1P2/R1BQK2R b KQkq - 0 1',
            'seajay_move': 'd4',
            'position_after': 'Opening up center with unsafe king',
            'game_context': 'SeaJay as White opened center while uncastled'
        },
        {
            'name': 'Game 5 vs Laser - Piece coordination failure',
            'fen': 'r1bqkb1r/ppp2ppp/5n2/n2p4/3PpN2/1B2P3/PPP2PPP/RNBQK2R w KQkq - 0 1',
            'seajay_move': 'O-O',
            'position_after': 'Castling into danger',
            'game_context': 'SeaJay castled when tactics were already present'
        }
    ]
    
    print("\n" + "=" * 80)
    print("CRITICAL POSITION ANALYSIS WITH STOCKFISH")
    print("=" * 80)
    
    for pos in critical_positions:
        print(f"\n{pos['name']}")
        print("-" * 60)
        print(f"FEN: {pos['fen']}")
        print(f"Context: {pos['game_context']}")
        
        # Analyze with Stockfish
        eval_score, best_move = analyze_position_with_stockfish(pos['fen'])
        print(f"\nStockfish evaluation: {eval_score:+.2f}")
        print(f"Stockfish best move: {best_move}")
        print(f"SeaJay played: {pos['seajay_move']}")
        
        # Calculate the difference if we can
        if best_move and best_move != pos['seajay_move']:
            print(f">> SeaJay missed the best move!")

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

def analyze_depth_patterns():
    """Analyze search depth patterns from the games."""
    print("\n" + "=" * 80)
    print("SEARCH DEPTH ANALYSIS")
    print("=" * 80)
    
    # Parse games and extract depth info
    seajay_depths = []
    opponent_depths = []
    
    files = [
        '/workspace/external/2025_08_25-4ku_seajay.pgn',
        '/workspace/external/2025_08_25-Laser_seajay.pgn'
    ]
    
    for file in files:
        with open(file, 'r') as f:
            content = f.read()
        
        # Find all moves with depth info
        # Format: move {eval depth/selective_depth time nodes}
        pattern = r'([A-Z]?[a-h]?[1-8]?x?[a-h][1-8][=QRBN]?(?:\+{1,2}|#)?)\s*\{[+-]?[0-9.M]+ (\d+)/(\d+)'
        
        matches = re.findall(pattern, content)
        
        # Check if SeaJay game based on headers
        if 'SeaJay' in content:
            for match in matches:
                move, depth, sel_depth = match
                depth_val = int(depth)
                sel_depth_val = int(sel_depth)
                
                # Determine if this is SeaJay's move (rough heuristic)
                # This is approximate - would need proper PGN parsing for accuracy
                if depth_val < 15:  # SeaJay typically searches less deep
                    seajay_depths.append(depth_val)
                else:
                    opponent_depths.append(depth_val)
    
    if seajay_depths:
        avg_seajay = sum(seajay_depths) / len(seajay_depths)
        max_seajay = max(seajay_depths)
        min_seajay = min(seajay_depths)
        
        print(f"\nSeaJay search depths:")
        print(f"  Average: {avg_seajay:.1f}")
        print(f"  Maximum: {max_seajay}")
        print(f"  Minimum: {min_seajay}")
        print(f"  Total moves analyzed: {len(seajay_depths)}")
    
    if opponent_depths:
        avg_opponent = sum(opponent_depths) / len(opponent_depths)
        max_opponent = max(opponent_depths)
        min_opponent = min(opponent_depths)
        
        print(f"\nOpponent search depths:")
        print(f"  Average: {avg_opponent:.1f}")
        print(f"  Maximum: {max_opponent}")
        print(f"  Minimum: {min_opponent}")
        print(f"  Total moves analyzed: {len(opponent_depths)}")
    
    # Depth distribution
    print("\nSeaJay depth distribution:")
    depth_buckets = {}
    for d in seajay_depths:
        bucket = (d // 5) * 5
        depth_buckets[bucket] = depth_buckets.get(bucket, 0) + 1
    
    for bucket in sorted(depth_buckets.keys()):
        print(f"  Depth {bucket}-{bucket+4}: {depth_buckets[bucket]} moves")

def analyze_time_management():
    """Analyze time usage patterns."""
    print("\n" + "=" * 80)
    print("TIME MANAGEMENT ANALYSIS")
    print("=" * 80)
    
    # Extract time usage from games
    # Format in moves: {eval depth/sel time nodes}
    
    files = [
        '/workspace/external/2025_08_25-4ku_seajay.pgn',
        '/workspace/external/2025_08_25-Laser_seajay.pgn'
    ]
    
    seajay_times = []
    opponent_times = []
    
    for file in files:
        with open(file, 'r') as f:
            content = f.read()
        
        # Extract time control
        tc_match = re.search(r'\[TimeControl "([^"]+)"\]', content)
        if tc_match:
            print(f"\nTime control for {file.split('/')[-1]}: {tc_match.group(1)}")
        
        # Extract times from moves
        pattern = r'\{[+-]?[0-9.M]+ \d+/\d+ (\d+) \d+\}'
        times = re.findall(pattern, content)
        
        # Rough heuristic: alternate between players
        for i, time in enumerate(times):
            time_ms = int(time)
            if i % 2 == 0:  # Approximate
                seajay_times.append(time_ms)
            else:
                opponent_times.append(time_ms)
    
    if seajay_times:
        avg_time = sum(seajay_times) / len(seajay_times)
        print(f"\nSeaJay average time per move: {avg_time:.0f} ms")
        
        # Check for time pressure
        quick_moves = sum(1 for t in seajay_times if t < 500)
        slow_moves = sum(1 for t in seajay_times if t > 2000)
        
        print(f"Quick moves (<500ms): {quick_moves} ({quick_moves/len(seajay_times)*100:.1f}%)")
        print(f"Slow moves (>2000ms): {slow_moves} ({slow_moves/len(seajay_times)*100:.1f}%)")

def main():
    analyze_critical_positions()
    analyze_depth_patterns()
    analyze_time_management()
    
    print("\n" + "=" * 80)
    print("SUMMARY OF FINDINGS")
    print("=" * 80)

if __name__ == "__main__":
    main()