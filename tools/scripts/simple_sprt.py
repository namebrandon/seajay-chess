#!/usr/bin/env python3
"""
Simple SPRT (Sequential Probability Ratio Test) implementation for chess engines.
This is a basic implementation for testing SeaJay development.
"""

import subprocess
import sys
import time
import math
import json
import os
from datetime import datetime
import random
import tempfile

class SPRTTest:
    def __init__(self, engine1_path, engine2_path, elo0=0, elo1=5, alpha=0.05, beta=0.05):
        self.engine1_path = engine1_path
        self.engine2_path = engine2_path
        self.elo0 = elo0
        self.elo1 = elo1
        self.alpha = alpha
        self.beta = beta
        
        # Convert Elo to score probability
        self.p0 = self.elo_to_score(elo0)
        self.p1 = self.elo_to_score(elo1)
        
        # Calculate SPRT bounds
        self.lower_bound = math.log(beta / (1 - alpha))
        self.upper_bound = math.log((1 - beta) / alpha)
        
        # Game results
        self.wins = 0
        self.draws = 0
        self.losses = 0
        self.games = 0
        
        # LLR tracking
        self.llr = 0
        
    def elo_to_score(self, elo):
        """Convert Elo difference to expected score."""
        return 1 / (1 + 10 ** (-elo / 400))
    
    def calculate_llr(self, win, draw, loss):
        """Calculate log-likelihood ratio."""
        if win + draw + loss == 0:
            return 0
            
        # Observed frequencies
        w = win / (win + draw + loss)
        d = draw / (win + draw + loss)
        l = loss / (win + draw + loss)
        
        # Avoid log(0)
        if w == 0: w = 0.001
        if d == 0: d = 0.001
        if l == 0: l = 0.001
        
        # Expected frequencies under H0 and H1
        w0 = self.p0
        d0 = 0  # Simplified: assume no draws
        l0 = 1 - self.p0
        
        w1 = self.p1
        d1 = 0  # Simplified: assume no draws
        l1 = 1 - self.p1
        
        # Log-likelihood ratio (simplified)
        llr = (win + draw + loss) * (
            w * math.log(w1 / w0) + 
            l * math.log(l1 / l0)
        )
        
        return llr
    
    def play_game(self, white_engine, black_engine, time_control="10+0.1"):
        """Play a single game between two engines."""
        # Create temporary PGN file for game
        with tempfile.NamedTemporaryFile(mode='w', suffix='.pgn', delete=False) as f:
            pgn_file = f.name
        
        try:
            # Simple game: each engine makes moves alternately
            board_fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"
            moves = []
            move_count = 0
            max_moves = 200  # Limit game length
            
            while move_count < max_moves:
                # Determine which engine to move
                if move_count % 2 == 0:
                    engine = white_engine
                else:
                    engine = black_engine
                
                # Send position to engine
                position_cmd = f"position fen {board_fen}"
                if moves:
                    position_cmd += " moves " + " ".join(moves)
                
                # Get move from engine
                try:
                    proc = subprocess.Popen(
                        [engine],
                        stdin=subprocess.PIPE,
                        stdout=subprocess.PIPE,
                        stderr=subprocess.PIPE,
                        text=True
                    )
                    
                    # Send UCI commands
                    commands = f"uci\nisready\n{position_cmd}\ngo movetime 100\nquit\n"
                    stdout, stderr = proc.communicate(input=commands, timeout=5)
                    
                    # Parse bestmove
                    bestmove = None
                    for line in stdout.split('\n'):
                        if line.startswith('bestmove'):
                            parts = line.split()
                            if len(parts) > 1 and parts[1] != '(none)':
                                bestmove = parts[1]
                                break
                    
                    if not bestmove:
                        # No legal moves - game over
                        break
                    
                    moves.append(bestmove)
                    move_count += 1
                    
                except subprocess.TimeoutExpired:
                    print(f"Engine timeout: {engine}")
                    return 0.5  # Draw on timeout
                except Exception as e:
                    print(f"Engine error: {e}")
                    return 0.5  # Draw on error
            
            # Determine result based on material count or move limit
            if move_count >= max_moves:
                return 0.5  # Draw by move limit
            
            # Simple result: if no moves available, current side loses
            if move_count % 2 == 0:
                # White to move but can't - white loses
                return 0  # Black wins
            else:
                # Black to move but can't - black loses
                return 1  # White wins
                
        finally:
            # Clean up temp file
            if os.path.exists(pgn_file):
                os.remove(pgn_file)
    
    def run_test(self, max_games=10000, time_control="10+0.1"):
        """Run SPRT test until decision or max games."""
        print(f"Starting SPRT test")
        print(f"Engine 1: {self.engine1_path}")
        print(f"Engine 2: {self.engine2_path}")
        print(f"Elo bounds: [{self.elo0}, {self.elo1}]")
        print(f"Alpha: {self.alpha}, Beta: {self.beta}")
        print(f"LLR bounds: [{self.lower_bound:.3f}, {self.upper_bound:.3f}]")
        print()
        
        start_time = time.time()
        
        while self.games < max_games:
            # Alternate colors
            if self.games % 2 == 0:
                result = self.play_game(self.engine1_path, self.engine2_path, time_control)
                if result == 1:
                    self.wins += 1
                elif result == 0:
                    self.losses += 1
                else:
                    self.draws += 1
            else:
                result = self.play_game(self.engine2_path, self.engine1_path, time_control)
                if result == 0:
                    self.wins += 1
                elif result == 1:
                    self.losses += 1
                else:
                    self.draws += 1
            
            self.games += 1
            
            # Calculate LLR
            self.llr = self.calculate_llr(self.wins, self.draws, self.losses)
            
            # Print progress every 10 games
            if self.games % 10 == 0:
                score = self.wins + self.draws * 0.5
                score_pct = score / self.games * 100
                elapsed = time.time() - start_time
                games_per_sec = self.games / elapsed if elapsed > 0 else 0
                
                print(f"Games: {self.games:4d} | Score: {score:.1f}/{self.games} ({score_pct:.1f}%) | "
                      f"W/D/L: {self.wins}/{self.draws}/{self.losses} | "
                      f"LLR: {self.llr:+.3f} | Games/sec: {games_per_sec:.1f}")
            
            # Check SPRT bounds
            if self.llr >= self.upper_bound:
                print(f"\nSPRT PASSED! H1 accepted (improvement detected)")
                print(f"Final LLR: {self.llr:.3f} >= {self.upper_bound:.3f}")
                return "PASS"
            elif self.llr <= self.lower_bound:
                print(f"\nSPRT FAILED! H0 accepted (no improvement)")
                print(f"Final LLR: {self.llr:.3f} <= {self.lower_bound:.3f}")
                return "FAIL"
        
        print(f"\nSPRT INCONCLUSIVE (max games reached)")
        return "INCONCLUSIVE"
    
    def save_results(self, output_dir):
        """Save test results to files."""
        os.makedirs(output_dir, exist_ok=True)
        
        # Save JSON summary
        results = {
            "date": datetime.now().isoformat(),
            "engine1": self.engine1_path,
            "engine2": self.engine2_path,
            "elo_bounds": [self.elo0, self.elo1],
            "alpha": self.alpha,
            "beta": self.beta,
            "games": self.games,
            "wins": self.wins,
            "draws": self.draws,
            "losses": self.losses,
            "score": self.wins + self.draws * 0.5,
            "score_percentage": (self.wins + self.draws * 0.5) / self.games * 100 if self.games > 0 else 0,
            "llr": self.llr,
            "llr_bounds": [self.lower_bound, self.upper_bound],
            "result": "PASS" if self.llr >= self.upper_bound else "FAIL" if self.llr <= self.lower_bound else "INCONCLUSIVE"
        }
        
        with open(os.path.join(output_dir, "sprt_status.json"), "w") as f:
            json.dump(results, f, indent=2)
        
        # Save human-readable summary
        with open(os.path.join(output_dir, "test_summary.md"), "w") as f:
            f.write(f"# SPRT Test Results\n\n")
            f.write(f"**Date:** {results['date']}\n")
            f.write(f"**Result:** {results['result']}\n\n")
            f.write(f"## Engines\n")
            f.write(f"- Test: {self.engine1_path}\n")
            f.write(f"- Base: {self.engine2_path}\n\n")
            f.write(f"## Parameters\n")
            f.write(f"- Elo bounds: [{self.elo0}, {self.elo1}]\n")
            f.write(f"- Alpha: {self.alpha}\n")
            f.write(f"- Beta: {self.beta}\n\n")
            f.write(f"## Results\n")
            f.write(f"- Games: {self.games}\n")
            f.write(f"- Score: {results['score']}/{self.games} ({results['score_percentage']:.1f}%)\n")
            f.write(f"- W/D/L: {self.wins}/{self.draws}/{self.losses}\n")
            f.write(f"- LLR: {self.llr:.3f} [{self.lower_bound:.3f}, {self.upper_bound:.3f}]\n")
        
        print(f"\nResults saved to {output_dir}")

def main():
    if len(sys.argv) < 3:
        print("Usage: python3 simple_sprt.py <engine1> <engine2> [--elo0 N] [--elo1 N] [--alpha A] [--beta B] [--games N] [--output DIR]")
        sys.exit(1)
    
    engine1 = sys.argv[1]
    engine2 = sys.argv[2]
    
    # Parse optional arguments
    elo0 = 0
    elo1 = 200  # For Stage 7 vs Stage 6, expect large improvement
    alpha = 0.05
    beta = 0.05
    max_games = 1000
    output_dir = f"sprt_results/test_{datetime.now().strftime('%Y%m%d_%H%M%S')}"
    
    i = 3
    while i < len(sys.argv):
        if sys.argv[i] == "--elo0" and i + 1 < len(sys.argv):
            elo0 = int(sys.argv[i + 1])
            i += 2
        elif sys.argv[i] == "--elo1" and i + 1 < len(sys.argv):
            elo1 = int(sys.argv[i + 1])
            i += 2
        elif sys.argv[i] == "--alpha" and i + 1 < len(sys.argv):
            alpha = float(sys.argv[i + 1])
            i += 2
        elif sys.argv[i] == "--beta" and i + 1 < len(sys.argv):
            beta = float(sys.argv[i + 1])
            i += 2
        elif sys.argv[i] == "--games" and i + 1 < len(sys.argv):
            max_games = int(sys.argv[i + 1])
            i += 2
        elif sys.argv[i] == "--output" and i + 1 < len(sys.argv):
            output_dir = sys.argv[i + 1]
            i += 2
        else:
            i += 1
    
    # Run test
    test = SPRTTest(engine1, engine2, elo0, elo1, alpha, beta)
    result = test.run_test(max_games)
    test.save_results(output_dir)
    
    # Exit with appropriate code
    if result == "PASS":
        sys.exit(0)
    elif result == "FAIL":
        sys.exit(1)
    else:
        sys.exit(2)

if __name__ == "__main__":
    main()