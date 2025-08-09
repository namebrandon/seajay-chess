#!/usr/bin/env python3
"""
SeaJay Chess Engine - Tournament Runner
Tournament wrapper around fast-chess for easy tournament setup
"""

import os
import json
import subprocess
import argparse
import tempfile
from pathlib import Path
from typing import Dict, Optional
import time

class TournamentRunner:
    def __init__(self, fastchess_path: str = '/workspace/external/testers/fast-chess/fastchess'):
        self.fastchess_path = Path(fastchess_path)
        if not self.fastchess_path.exists():
            raise FileNotFoundError(f"fast-chess not found: {self.fastchess_path}")
    
    def get_time_controls(self) -> Dict[str, str]:
        """Predefined time controls"""
        return {
            'bullet': '1+0.01',
            'quick': '5+0.05',
            'standard': '10+0.1',
            'long': '30+0.3',
            'test': '0.1+0.001'
        }
    
    def run_tournament(self, 
                      engine1_path: str, 
                      engine2_path: str,
                      games: int = 100,
                      time_control: str = 'quick',
                      opening_book: Optional[str] = None,
                      concurrency: int = 1,
                      output_dir: Optional[str] = None) -> Dict:
        """Run tournament between two engines"""
        
        # Validate engines
        if not Path(engine1_path).exists():
            raise FileNotFoundError(f"Engine 1 not found: {engine1_path}")
        if not Path(engine2_path).exists():
            raise FileNotFoundError(f"Engine 2 not found: {engine2_path}")
        
        # Get time control
        time_controls = self.get_time_controls()
        tc = time_controls.get(time_control, time_control)
        
        # Setup output directory
        if not output_dir:
            output_dir = tempfile.mkdtemp(prefix="tournament_")
        Path(output_dir).mkdir(parents=True, exist_ok=True)
        
        pgn_file = Path(output_dir) / "games.pgn"
        
        print(f"Running tournament: {games} games at {tc}")
        print(f"Output: {output_dir}")
        
        # Build command
        cmd = [
            str(self.fastchess_path),
            "-engine", f"cmd={engine1_path}", "name=Engine1",
            "-engine", f"cmd={engine2_path}", "name=Engine2",
            "-each", f"tc={tc}",
            "-rounds", str(games),
            "-concurrency", str(concurrency),
            "-pgn", f"file={pgn_file}",
            "-recover"
        ]
        
        if opening_book and Path(opening_book).exists():
            cmd.extend(["-openings", f"file={opening_book}"])
        
        # Run tournament
        start_time = time.time()
        result = subprocess.run(cmd, capture_output=True, text=True)
        duration = time.time() - start_time
        
        # Parse results from output
        wins1 = wins2 = draws = 0
        for line in result.stdout.split('\n'):
            if 'Wins:' in line:
                import re
                numbers = re.findall(r'\d+', line)
                if len(numbers) >= 3:
                    wins1, wins2, draws = int(numbers[0]), int(numbers[1]), int(numbers[2])
                    break
        
        return {
            'games_played': wins1 + wins2 + draws,
            'engine1_wins': wins1,
            'engine2_wins': wins2,
            'draws': draws,
            'duration': duration,
            'pgn_file': str(pgn_file),
            'output_dir': output_dir
        }

def main():
    parser = argparse.ArgumentParser(description='SeaJay Tournament Runner')
    parser.add_argument('engine1', help='Path to first engine')
    parser.add_argument('engine2', help='Path to second engine')
    parser.add_argument('--games', type=int, default=100, help='Number of games')
    parser.add_argument('--time-control', default='quick', help='Time control')
    parser.add_argument('--opening-book', help='Opening book PGN file')
    parser.add_argument('--concurrency', type=int, default=1, help='Concurrent games')
    parser.add_argument('--output-dir', help='Output directory')
    
    args = parser.parse_args()
    
    runner = TournamentRunner()
    result = runner.run_tournament(
        args.engine1, args.engine2,
        games=args.games,
        time_control=args.time_control,
        opening_book=args.opening_book,
        concurrency=args.concurrency,
        output_dir=args.output_dir
    )
    
    print(f"\nResults: W1={result['engine1_wins']} W2={result['engine2_wins']} D={result['draws']}")
    print(f"PGN saved to: {result['pgn_file']}")
    
    return 0

if __name__ == '__main__':
    exit(main())