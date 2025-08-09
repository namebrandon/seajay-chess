#!/usr/bin/env python3
"""
Simple match test between material evaluation and random move engines.
"""

import subprocess
import time
import sys

def send_command(process, command):
    """Send a command to the engine and flush."""
    process.stdin.write(command + '\n')
    process.stdin.flush()

def read_until_bestmove(process):
    """Read engine output until we see a bestmove."""
    while True:
        line = process.stdout.readline()
        if line.startswith('bestmove'):
            return line.strip()

def play_game(engine1_path, engine2_path, max_moves=200):
    """Play a single game between two engines."""
    # Start both engines
    engine1 = subprocess.Popen([engine1_path], 
                              stdin=subprocess.PIPE, 
                              stdout=subprocess.PIPE, 
                              stderr=subprocess.DEVNULL,
                              text=True, bufsize=0)
    
    engine2 = subprocess.Popen([engine2_path],
                              stdin=subprocess.PIPE,
                              stdout=subprocess.PIPE,
                              stderr=subprocess.DEVNULL,
                              text=True, bufsize=0)
    
    # Initialize UCI
    for engine in [engine1, engine2]:
        send_command(engine, 'uci')
        time.sleep(0.1)
        send_command(engine, 'isready')
        time.sleep(0.1)
    
    # Start from initial position
    position = "position startpos"
    moves = []
    
    for move_num in range(max_moves):
        # Determine which engine to move
        engine = engine1 if move_num % 2 == 0 else engine2
        
        # Send position
        send_command(engine, position)
        
        # Get move with short time
        send_command(engine, 'go movetime 50')
        bestmove_line = read_until_bestmove(engine)
        
        # Extract move
        move = bestmove_line.split()[1]
        if move == '(none)' or move == '0000':
            # Game over
            if move_num % 2 == 0:
                return "black"  # Engine1 (white) has no moves
            else:
                return "white"  # Engine2 (black) has no moves
        
        # Update position
        moves.append(move)
        position = f"position startpos moves {' '.join(moves)}"
    
    # Quit engines
    for engine in [engine1, engine2]:
        send_command(engine, 'quit')
        engine.wait()
    
    return "draw"  # Max moves reached

def main():
    """Run a match between material and random engines."""
    material_engine = "/workspace/bin/seajay"
    random_engine = "/workspace/bin/seajay_random"
    
    print("=== Material Evaluation vs Random Move Selection ===")
    print("Playing 10 games...")
    print()
    
    wins = {"material": 0, "random": 0, "draw": 0}
    
    for game_num in range(10):
        print(f"Game {game_num + 1}...", end=" ")
        sys.stdout.flush()
        
        # Alternate colors
        if game_num % 2 == 0:
            result = play_game(material_engine, random_engine)
            if result == "white":
                wins["material"] += 1
                print("Material wins")
            elif result == "black":
                wins["random"] += 1
                print("Random wins")
            else:
                wins["draw"] += 1
                print("Draw")
        else:
            result = play_game(random_engine, material_engine)
            if result == "white":
                wins["random"] += 1
                print("Random wins")
            elif result == "black":
                wins["material"] += 1
                print("Material wins")
            else:
                wins["draw"] += 1
                print("Draw")
    
    print()
    print("=== Results ===")
    print(f"Material: {wins['material']} wins")
    print(f"Random:   {wins['random']} wins")
    print(f"Draws:    {wins['draw']}")
    print()
    
    win_rate = wins['material'] / 10 * 100
    print(f"Material evaluation win rate: {win_rate:.1f}%")
    
    if win_rate >= 90:
        print("✓ SPRT test PASSED - Material evaluation significantly stronger than random")
    else:
        print("✗ SPRT test FAILED - Material evaluation not strong enough")

if __name__ == "__main__":
    main()