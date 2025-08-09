#!/usr/bin/env python3
"""
SeaJay Chess Engine - Automated Perft Test Runner
Comprehensive perft validation script with parallel execution
"""

import json
import subprocess
import concurrent.futures
import time
import argparse
from typing import Dict, List, Optional
from dataclasses import dataclass
from pathlib import Path

@dataclass
class PerftPosition:
    name: str
    fen: str
    depth: int
    expected: int
    required: bool = True

class PerftRunner:
    def __init__(self, engine_path: str, stockfish_path: Optional[str] = None):
        self.engine_path = Path(engine_path)
        self.stockfish_path = Path(stockfish_path) if stockfish_path else None
        
        if not self.engine_path.exists():
            raise FileNotFoundError(f"Engine not found: {self.engine_path}")

    def load_positions(self) -> List[PerftPosition]:
        """Load perft positions from Master Project Plan"""
        return [
            PerftPosition("Starting Position - Depth 5", 
                         "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", 
                         5, 4865609),
            PerftPosition("Starting Position - Depth 6", 
                         "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", 
                         6, 119060324),
            PerftPosition("Kiwipete - Depth 4", 
                         "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -", 
                         4, 4085603),
            PerftPosition("Kiwipete - Depth 5", 
                         "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -", 
                         5, 193690690),
            PerftPosition("Position 3 - Depth 5", 
                         "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - -", 
                         5, 674624),
            PerftPosition("Position 3 - Depth 6", 
                         "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - -", 
                         6, 11030083),
            PerftPosition("Position 4 - Depth 4", 
                         "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1", 
                         4, 422333),
            PerftPosition("Position 5 - Depth 4", 
                         "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8", 
                         4, 2103487),
            PerftPosition("Position 6 - Depth 4", 
                         "r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10", 
                         4, 3894594),
        ]

    def run_perft(self, position: PerftPosition, timeout: int = 300) -> Dict:
        """Run perft test on a single position"""
        start_time = time.time()
        
        # Run perft using SeaJay's perft command format
        cmd = f"echo -e 'position fen {position.fen}\\nperft {position.depth}\\nquit' | {self.engine_path}"
        
        try:
            result = subprocess.run(cmd, shell=True, capture_output=True, text=True, timeout=timeout)
            
            # Parse perft output - look for node count
            nodes = 0
            for line in result.stdout.split('\n'):
                if 'Nodes:' in line or 'nodes' in line.lower():
                    import re
                    numbers = re.findall(r'\d+', line)
                    if numbers:
                        nodes = int(numbers[-1])
                        break
            
            status = 'pass' if nodes == position.expected else 'fail'
            
            return {
                'position': position.name,
                'status': status,
                'expected': position.expected,
                'actual': nodes,
                'time': time.time() - start_time
            }
            
        except subprocess.TimeoutExpired:
            return {'position': position.name, 'status': 'timeout', 'time': timeout}
        except Exception as e:
            return {'position': position.name, 'status': 'error', 'error': str(e)}

    def run_tests(self, positions: List[PerftPosition], max_workers: int = 4) -> Dict:
        """Run all perft tests with parallel execution"""
        print(f"Running {len(positions)} perft tests...")
        
        with concurrent.futures.ThreadPoolExecutor(max_workers=max_workers) as executor:
            results = list(executor.map(self.run_perft, positions))
        
        # Calculate summary
        passed = sum(1 for r in results if r['status'] == 'pass')
        failed = sum(1 for r in results if r['status'] == 'fail')
        
        summary = {
            'total_tests': len(results),
            'passed': passed,
            'failed': failed,
            'success_rate': (passed / len(results) * 100) if results else 0,
            'results': results
        }
        
        # Print results
        for result in results:
            status_icon = '✓' if result['status'] == 'pass' else '✗'
            print(f"{status_icon} {result['position']}: {result['status']}")
            if result['status'] == 'fail':
                print(f"  Expected: {result['expected']}, Got: {result['actual']}")
        
        print(f"\nSummary: {passed}/{len(results)} tests passed ({summary['success_rate']:.1f}%)")
        
        return summary

def main():
    parser = argparse.ArgumentParser(description='SeaJay Perft Test Runner')
    parser.add_argument('--engine', default='/workspace/bin/seajay', help='Path to engine')
    parser.add_argument('--workers', type=int, default=4, help='Parallel workers')
    parser.add_argument('--output', help='JSON output file')
    parser.add_argument('--required-only', action='store_true', help='Run only required tests')
    
    args = parser.parse_args()
    
    runner = PerftRunner(args.engine)
    positions = runner.load_positions()
    
    if args.required_only:
        positions = [p for p in positions if p.required]
    
    summary = runner.run_tests(positions, args.workers)
    
    if args.output:
        with open(args.output, 'w') as f:
            json.dump(summary, f, indent=2)
    
    return 0 if summary['success_rate'] == 100.0 else 1

if __name__ == '__main__':
    exit(main())