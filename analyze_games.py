#!/usr/bin/env python3
"""
Analyze SeaJay's questionable moves from human games
"""

import subprocess
import re
import time

def send_uci_commands(engine_path, commands):
    """Send UCI commands to engine and return output"""
    process = subprocess.Popen(
        [engine_path],
        stdin=subprocess.PIPE,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True
    )
    
    output, _ = process.communicate(input='\n'.join(commands) + '\nquit\n')
    return output

def get_position_after_moves(initial_fen, moves_uci):
    """Apply moves to FEN and return new position"""
    if initial_fen == "startpos":
        position_cmd = "position startpos"
    else:
        position_cmd = f"position fen {initial_fen}"
    
    if moves_uci:
        position_cmd += f" moves {' '.join(moves_uci)}"
    
    return position_cmd

def analyze_position(engine_path, position_cmd, depth=10, time_ms=5000):
    """Analyze a position with given engine"""
    commands = [
        "uci",
        position_cmd,
        f"go depth {depth} movetime {time_ms}"
    ]
    
    output = send_uci_commands(engine_path, commands)
    
    # Extract best move and evaluation
    bestmove = None
    evaluation = 0
    pv_line = None
    
    for line in output.split('\n'):
        if 'bestmove' in line:
            parts = line.split()
            if len(parts) >= 2:
                bestmove = parts[1]
        elif 'info depth' in line and 'pv' in line:
            # Get the last depth info line with pv
            match = re.search(r'score cp (-?\d+)', line)
            if match:
                evaluation = int(match.group(1))
            elif 'score mate' in line:
                match = re.search(r'score mate (-?\d+)', line)
                if match:
                    mate_in = int(match.group(1))
                    evaluation = 10000 if mate_in > 0 else -10000
            
            # Extract PV line
            if ' pv ' in line:
                pv_start = line.index(' pv ') + 4
                pv_line = line[pv_start:].strip()
    
    return bestmove, evaluation, pv_line

def algebraic_to_uci(move_algebraic, position):
    """Convert algebraic notation to UCI (simplified)"""
    # This is a simplified conversion - would need full chess library for accuracy
    # For now, we'll manually specify the critical positions
    return move_algebraic  # Placeholder

# Game 1 critical positions and moves
# Starting position after move 12: O-O
game1_moves = [
    "d2d4", "e7e6", "g1f3", "c7c5", "c2c3", "c5d4", "c3d4", "g8f6",
    "c1f4", "b8c6", "b1c3", "d8b6", "d1c2", "c6d4", "f3d4", "b6d4",
    "e2e4", "f8b4", "f1d3", "f6e4", "d3e4", "d7d5", "e1g1"
]

# Positions to analyze in Game 1 (after White's move, Black to move)
game1_critical = [
    # After 13.O-O, Black played Bxc3 but should analyze position
    {"after_moves": game1_moves, "move_played": "b4c3", "move_num": "13...Bxc3?!"},
    
    # After 16.Rfe1, Black played Qg4
    {"after_moves": game1_moves + ["b4c3", "f4d6", "c3b2", "a1b1", "d4e4", "f1e1"],
     "move_played": "e4g4", "move_num": "16...Qg4?!"},
    
    # After 17.Rbc1, Black played Kf7
    {"after_moves": game1_moves + ["b4c3", "f4d6", "c3b2", "a1b1", "d4e4", "f1e1", "e4g4", "b1c1"],
     "move_played": "e8f7", "move_num": "17...Kf7?!"},
    
    # After 19.h3, Black played Qf5
    {"after_moves": game1_moves + ["b4c3", "f4d6", "c3b2", "a1b1", "d4e4", "f1e1", "e4g4", "b1c1", "e8f7", "c1c7", "f7g6", "h2h3"],
     "move_played": "g4f5", "move_num": "19...Qf5?!"},
    
    # After 21.Rg3+, Black played Kh5
    {"after_moves": game1_moves + ["b4c3", "f4d6", "c3b2", "a1b1", "d4e4", "f1e1", "e4g4", "b1c1", "e8f7", "c1c7", "f7g6", "h2h3", "g4f5", "e1e3", "e6e5", "e3g3", "g6h5"],
     "move_played": "h5h5", "move_num": "21...Kh5?!"}
]

# Game 2 critical positions (White is SeaJay)
game2_moves = [
    "b1c3", "e7e5", "e2e4", "b8c6", "g1f3", "f8e7", "f1b5", "d7d6",
    "e1g1", "g8f6", "d2d3", "e8g8", "b5c4", "c8g4", "h2h3", "g4f3",
    "d1f3", "c6d4", "f3g3"
]

game2_critical = [
    # After 9...Nd4, White played 10.Qg3?!
    {"after_moves": game2_moves[:-1], "move_played": "f3g3", "move_num": "10.Qg3?!", "white_to_move": True},
    
    # After 10...Nxc2, White played 11.Bh6?!
    {"after_moves": game2_moves + ["d4c2"], "move_played": "c1h6", "move_num": "11.Bh6?!", "white_to_move": True},
    
    # After 16.Qd2 Qh4, White played 17.a4?!
    {"after_moves": game2_moves + ["d4c2", "c1h6", "f6h5", "g3g4", "c2a1", "a1a1", "e7f6", "g4h5", "g7h6",
                                     "h5h6", "f6g7", "h6d2", "d8h4"],
     "move_played": "a2a4", "move_num": "17.a4?!", "white_to_move": True},
    
    # After 23.b3, this was questionable
    {"after_moves": game2_moves + ["d4c2", "c1h6", "f6h5", "g3g4", "c2a1", "a1a1", "e7f6", "g4h5", "g7h6",
                                     "h5h6", "f6g7", "h6d2", "d8h4", "a2a4", "a7a6", "d2e3", "g7h6", "e3f3",
                                     "h4f4", "f3d1", "c7c6", "a4a5", "a8b8", "c3a4", "f4d2", "a4b6", "f8d8", "b2b3"],
     "move_played": "b2b3", "move_num": "23.b3?!", "white_to_move": True},
    
    # After 34.Rf5, White played this
    {"after_moves": game2_moves + ["d4c2", "c1h6", "f6h5", "g3g4", "c2a1", "a1a1", "e7f6", "g4h5", "g7h6",
                                     "h5h6", "f6g7", "h6d2", "d8h4", "a2a4", "a7a6", "d2e3", "g7h6", "e3f3",
                                     "h4f4", "f3d1", "c7c6", "a4a5", "a8b8", "c3a4", "f4d2", "a4b6", "f8d8", "b2b3",
                                     "d2d1", "a1d1", "h6g5", "g1h2", "g5e7", "h2h1", "d8e8", "g2g3", "b8b8", "f2f4",
                                     "e5f4", "d1f1", "e7d8", "b6d7", "b8c8", "f1f4", "e8e7", "d7f6", "g8g7", "f4f5"],
     "move_played": "f4f5", "move_num": "34.Rf5?!", "white_to_move": True}
]

def main():
    seajay_path = "/workspace/bin/seajay"
    stockfish_path = "/workspace/external/engines/stockfish/stockfish"
    
    print("="*80)
    print("ANALYZING SEAJAY'S QUESTIONABLE MOVES FROM HUMAN GAMES")
    print("="*80)
    print()
    
    # Analyze starting position first (SeaJay evaluation issue mentioned)
    print("ANALYZING STARTING POSITION:")
    print("-"*40)
    
    pos_cmd = "position startpos"
    sj_best, sj_eval, sj_pv = analyze_position(seajay_path, pos_cmd, depth=10, time_ms=3000)
    sf_best, sf_eval, sf_pv = analyze_position(stockfish_path, pos_cmd, depth=20, time_ms=1000)
    
    print(f"SeaJay evaluation: {sj_eval/100:.2f} Best: {sj_best}")
    print(f"SeaJay PV: {sj_pv[:60] if sj_pv else 'None'}")
    print(f"Stockfish evaluation: {sf_eval/100:.2f} Best: {sf_best}")
    print(f"Stockfish PV: {sf_pv[:60] if sf_pv else 'None'}")
    print()
    
    # Analyze Game 1 critical positions
    print("="*80)
    print("GAME 1: Brandon Harris (White) vs SeaJay (Black) - 1-0")
    print("="*80)
    print()
    
    for position in game1_critical:
        print(f"Position after White's move, before {position['move_num']}:")
        print("-"*40)
        
        pos_cmd = get_position_after_moves("startpos", position["after_moves"])
        
        # SeaJay analysis
        sj_best, sj_eval, sj_pv = analyze_position(seajay_path, pos_cmd, depth=10, time_ms=5000)
        
        # Stockfish analysis
        sf_best, sf_eval, sf_pv = analyze_position(stockfish_path, pos_cmd, depth=20, time_ms=2000)
        
        # Analyze position after the actual move
        pos_after_cmd = get_position_after_moves("startpos", position["after_moves"] + [position["move_played"]])
        _, sj_eval_after, _ = analyze_position(seajay_path, pos_after_cmd, depth=10, time_ms=2000)
        _, sf_eval_after, _ = analyze_position(stockfish_path, pos_after_cmd, depth=15, time_ms=1000)
        
        print(f"Move played: {position['move_played']}")
        print(f"SeaJay suggested: {sj_best} (eval: {sj_eval/100:.2f})")
        print(f"SeaJay PV: {sj_pv[:80] if sj_pv else 'None'}")
        print(f"Stockfish suggested: {sf_best} (eval: {sf_eval/100:.2f})")
        print(f"Stockfish PV: {sf_pv[:80] if sf_pv else 'None'}")
        print(f"Position after actual move - SeaJay: {-sj_eval_after/100:.2f}, Stockfish: {-sf_eval_after/100:.2f}")
        print(f"Evaluation difference: SeaJay vs Stockfish = {(sj_eval - sf_eval)/100:.2f}")
        print()
    
    # Analyze Game 2 critical positions  
    print("="*80)
    print("GAME 2: SeaJay (White) vs Brandon Harris (Black) - 0-1")
    print("="*80)
    print()
    
    for position in game2_critical:
        print(f"Position before {position['move_num']}:")
        print("-"*40)
        
        pos_cmd = get_position_after_moves("startpos", position["after_moves"])
        
        # SeaJay analysis
        sj_best, sj_eval, sj_pv = analyze_position(seajay_path, pos_cmd, depth=10, time_ms=5000)
        
        # Stockfish analysis  
        sf_best, sf_eval, sf_pv = analyze_position(stockfish_path, pos_cmd, depth=20, time_ms=2000)
        
        # Analyze position after the actual move
        if "move_played" in position:
            pos_after_cmd = get_position_after_moves("startpos", position["after_moves"] + [position["move_played"]])
            _, sj_eval_after, _ = analyze_position(seajay_path, pos_after_cmd, depth=10, time_ms=2000)
            _, sf_eval_after, _ = analyze_position(stockfish_path, pos_after_cmd, depth=15, time_ms=1000)
            
            print(f"Move played: {position['move_played']}")
            print(f"Position after actual move - SeaJay: {-sj_eval_after/100:.2f}, Stockfish: {-sf_eval_after/100:.2f}")
        
        print(f"SeaJay suggested: {sj_best} (eval: {sj_eval/100:.2f})")
        print(f"SeaJay PV: {sj_pv[:80] if sj_pv else 'None'}")
        print(f"Stockfish suggested: {sf_best} (eval: {sf_eval/100:.2f})")
        print(f"Stockfish PV: {sf_pv[:80] if sf_pv else 'None'}")
        print(f"Evaluation difference: SeaJay vs Stockfish = {(sj_eval - sf_eval)/100:.2f}")
        print()

if __name__ == "__main__":
    main()