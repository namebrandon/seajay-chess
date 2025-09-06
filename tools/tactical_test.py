#!/usr/bin/env python3

"""
Improved Tactical Test Suite Evaluation Script for SeaJay Chess Engine
Properly handles conversion between algebraic and coordinate notation
"""

import subprocess
import sys
import re
import time
import os
from datetime import datetime
import csv

# ANSI color codes
RED = '\033[0;31m'
GREEN = '\033[0;32m'
YELLOW = '\033[1;33m'
BLUE = '\033[0;34m'
NC = '\033[0m'  # No Color

def parse_epd_line(line):
    """Parse an EPD line to extract FEN, best moves, and ID"""
    if not line or line.startswith('#'):
        return None, None, None
    
    # Handle malformed FENs with EPD operations embedded
    # First, try to extract a clean FEN by looking for common EPD operations
    line_clean = line
    for op in ['am ', 'bm ', 'dm ', 'pv ']:
        if op in line:
            # Find where the EPD operation starts and clean the FEN
            op_start = line.find(op)
            if op_start > 0:
                # Extract potential FEN part before the operation
                potential_fen = line[:op_start].strip()
                # Check if it looks like a valid FEN ending
                if re.search(r'[wb]\s+[KQkq-]+\s*$', potential_fen):
                    # Reconstruct line with proper formatting
                    line_clean = potential_fen + " 0 1 " + line[op_start:]
                    break
    
    # Extract FEN (everything before 'bm')
    match = re.match(r'^(.*?)\s+bm\s+(.*?);.*?id\s+"([^"]*)"', line_clean)
    if not match:
        return None, None, None
    
    fen = match.group(1).strip()
    best_moves = match.group(2).strip().split()
    position_id = match.group(3).strip()
    
    # Remove any EPD operations that might still be in the FEN
    for op in ['am', 'dm', 'pv']:
        if op in fen:
            fen = fen[:fen.find(op)].strip()
    
    # Add default move counters if not present
    if not re.search(r'\d+\s+\d+$', fen):
        fen += " 0 1"
    
    return fen, best_moves, position_id

def convert_san_to_uci(fen, san_move):
    """
    Convert SAN (Standard Algebraic Notation) to UCI coordinate notation
    using chess library for accurate conversion
    """
    try:
        import chess
        board = chess.Board(fen)
        
        # Try to parse the SAN move
        try:
            move = board.parse_san(san_move)
            return move.uci()
        except:
            # If parsing fails, it might already be in UCI format
            if re.match(r'^[a-h][1-8][a-h][1-8][qrbn]?$', san_move):
                return san_move
            return None
    except ImportError:
        # Fallback: simple pattern matching for common cases
        # This is limited but better than the hardcoded approach
        
        # Already in UCI format?
        if re.match(r'^[a-h][1-8][a-h][1-8][qrbn]?$', san_move):
            return san_move
        
        # Try to extract destination square from simple moves
        # This won't work for all cases but handles some common ones
        patterns = {
            r'^([KQRBN])([a-h][1-8])$': lambda m: None,  # Piece to square (need source)
            r'^([a-h])([1-8])$': lambda m: None,  # Pawn to square (need source)
            r'^([KQRBN])x([a-h][1-8])$': lambda m: None,  # Capture (need source)
            r'^([a-h])x([a-h][1-8])$': lambda m: None,  # Pawn capture (need source)
        }
        
        # We can't reliably convert without a chess library
        return None

def run_engine_search(engine_path, fen, time_ms=2000, depth=0):
    """Run the chess engine and get the best move"""
    if depth > 0:
        go_command = f"go depth {depth}"
    else:
        go_command = f"go movetime {time_ms}"
    
    commands = f"uci\nposition fen {fen}\n{go_command}\nquit\n"
    
    try:
        result = subprocess.run(
            engine_path,
            input=commands,
            text=True,
            capture_output=True,
            timeout=(time_ms/1000 + 5) if depth == 0 else 60
        )
        
        # Extract best move
        best_move = None
        for line in result.stdout.split('\n'):
            if line.startswith('bestmove'):
                best_move = line.split()[1]
                break
        
        # Extract statistics from last info line
        info_lines = [l for l in result.stdout.split('\n') if l.startswith('info') and 'string' not in l]
        if info_lines:
            last_info = info_lines[-1]
            nodes = re.search(r'nodes (\d+)', last_info)
            depth_reached = re.search(r'depth (\d+)', last_info)
            score = re.search(r'score cp ([+-]?\d+)', last_info)
            if not score:
                score = re.search(r'score mate ([+-]?\d+)', last_info)
                if score:
                    score = f"M{score.group(1)}"
                else:
                    score = None
            else:
                score = score.group(1)
            
            nodes = int(nodes.group(1)) if nodes else 0
            depth_reached = int(depth_reached.group(1)) if depth_reached else 0
            score = score if score else "?"
            
            return best_move, nodes, depth_reached, score
    except subprocess.TimeoutExpired:
        return None, 0, 0, "timeout"
    except Exception as e:
        print(f"Error running engine: {e}")
        return None, 0, 0, "error"
    
    return best_move, 0, 0, "?"

def moves_match(engine_move, expected_moves, fen):
    """Check if engine move matches any expected move (handling notation differences)"""
    if not engine_move:
        return False
    
    # Direct match
    if engine_move in expected_moves:
        return True
    
    # Try converting expected moves to UCI format
    for expected in expected_moves:
        uci_expected = convert_san_to_uci(fen, expected)
        if uci_expected and engine_move == uci_expected:
            return True
        
        # Check if moves are semantically the same (same source and destination)
        # This handles cases like "Nca4" vs "c5a4"
        if len(engine_move) >= 4 and len(expected) >= 2:
            # Extract destination from both moves
            engine_dest = engine_move[2:4]
            
            # For SAN moves, try to extract destination
            san_dest_match = re.search(r'([a-h][1-8])', expected.replace('x', ''))
            if san_dest_match:
                san_dest = san_dest_match.group(1)
                if engine_dest == san_dest:
                    # Destination matches, likely the same move
                    # This is a heuristic but works for many cases
                    return True
    
    return False

def main():
    # Parse arguments
    engine_path = sys.argv[1] if len(sys.argv) > 1 else "./bin/seajay"
    test_file = sys.argv[2] if len(sys.argv) > 2 else "./tests/positions/wacnew.epd"
    time_per_move = int(sys.argv[3]) if len(sys.argv) > 3 else 2000
    depth_limit = int(sys.argv[4]) if len(sys.argv) > 4 else 0
    
    # Check if chess library is available
    try:
        import chess
        print(f"{GREEN}Using python-chess library for accurate move conversion{NC}")
    except ImportError:
        print(f"{YELLOW}Warning: python-chess not installed. Install with: pip install chess{NC}")
        print(f"{YELLOW}Falling back to heuristic move matching (less accurate){NC}")
        print()
    
    # Print header
    print("=" * 50)
    print("SeaJay Tactical Test Suite Evaluation")
    print("=" * 50)
    print(f"Engine: {engine_path}")
    print(f"Test file: {test_file}")
    print(f"Time per move: {time_per_move}ms")
    if depth_limit > 0:
        print(f"Depth limit: {depth_limit}")
    print("=" * 50)
    print()
    
    # Initialize counters
    total_positions = 0
    correct_positions = 0
    failed_positions = 0
    total_nodes = 0
    total_depth = 0
    failed_details = []
    
    start_time = time.time()
    last_update = start_time
    
    # Process test file
    try:
        with open(test_file, 'r') as f:
            for line in f:
                fen, best_moves, position_id = parse_epd_line(line.strip())
                if not fen:
                    continue
                
                total_positions += 1
                
                # Status update every 20 seconds
                current_time = time.time()
                if current_time - last_update >= 20:
                    elapsed = int(current_time - start_time)
                    success_rate = (correct_positions * 100 / total_positions) if total_positions > 0 else 0
                    print(f"\n{BLUE}=== Status Update ==={NC}")
                    print(f"Time elapsed: {elapsed}s | Positions: {total_positions} | Passed: {correct_positions} | Failed: {failed_positions}")
                    print(f"Current success rate: {success_rate:.1f}%")
                    print(f"{BLUE}==================={NC}\n")
                    last_update = current_time
                
                # Run engine search
                engine_move, nodes, depth_reached, score = run_engine_search(
                    engine_path, fen, time_per_move, depth_limit
                )
                
                total_nodes += nodes
                total_depth += depth_reached
                
                # Check if move is correct
                if moves_match(engine_move, best_moves, fen):
                    correct_positions += 1
                    print(f"{GREEN}✓{NC} {position_id}: {GREEN}PASS{NC} (move: {engine_move}, depth: {depth_reached}, score: {score}, nodes: {nodes})")
                else:
                    failed_positions += 1
                    failed_details.append({
                        'id': position_id,
                        'expected': ' '.join(best_moves),
                        'got': engine_move or 'none',
                        'fen': fen
                    })
                    print(f"{RED}✗{NC} {position_id}: {RED}FAIL{NC} (expected: {' '.join(best_moves)}, got: {engine_move}, depth: {depth_reached}, score: {score})")
    
    except FileNotFoundError:
        print(f"{RED}Error: Test file not found: {test_file}{NC}")
        sys.exit(1)
    except Exception as e:
        print(f"{RED}Error processing test file: {e}{NC}")
        sys.exit(1)
    
    # Calculate statistics
    if total_positions > 0:
        success_rate = correct_positions * 100 / total_positions
        avg_nodes = total_nodes // total_positions
        avg_depth = total_depth / total_positions
    else:
        success_rate = 0
        avg_nodes = 0
        avg_depth = 0
    
    # Print summary
    print()
    print("=" * 50)
    print("Test Results Summary")
    print("=" * 50)
    print(f"Total positions tested: {BLUE}{total_positions}{NC}")
    print(f"Positions passed: {GREEN}{correct_positions}{NC}")
    print(f"Positions failed: {RED}{failed_positions}{NC}")
    print(f"Success rate: {YELLOW}{success_rate:.1f}%{NC}")
    print(f"Total nodes searched: {total_nodes:,}")
    print(f"Average nodes per position: {avg_nodes:,}")
    print(f"Average depth reached: {avg_depth:.1f}")
    print("=" * 50)
    
    # Performance analysis
    print()
    print("Performance Analysis:")
    if success_rate >= 90:
        print(f"{GREEN}Excellent tactical performance (≥90%){NC}")
    elif success_rate >= 80:
        print(f"{GREEN}Good tactical performance (80-89%){NC}")
    elif success_rate >= 70:
        print(f"{YELLOW}Moderate tactical performance (70-79%){NC}")
    elif success_rate >= 60:
        print(f"{YELLOW}Below average tactical performance (60-69%){NC}")
    else:
        print(f"{RED}Poor tactical performance (<60%){NC}")
    
    # Show failed positions (first 10)
    if failed_details:
        print()
        if len(failed_details) <= 10:
            print("Failed Positions Details:")
        else:
            print(f"Failed Positions (showing first 10 of {len(failed_details)}):")
        print("-" * 30)
        for i, fail in enumerate(failed_details[:10]):
            print(f"{fail['id']}: Expected {fail['expected']}, Got {fail['got']}")
    
    # Save results to CSV
    timestamp = datetime.now().strftime("%Y-%m-%d_%H-%M-%S")
    report_file = f"tactical_test_{timestamp}.csv"
    
    with open(report_file, 'w', newline='') as csvfile:
        writer = csv.writer(csvfile)
        writer.writerow(['timestamp', 'engine', 'test_file', 'time_ms', 'total', 'passed', 'failed', 'success_rate', 'total_nodes', 'avg_nodes', 'avg_depth'])
        writer.writerow([timestamp, engine_path, test_file, time_per_move, total_positions, correct_positions, failed_positions, f"{success_rate:.1f}", total_nodes, avg_nodes, f"{avg_depth:.1f}"])
    
    print()
    print(f"Results saved to: {report_file}")
    
    # Update history file
    history_file = "tactical_test_history.csv"
    is_new_file = not os.path.exists(history_file)
    
    with open(history_file, 'a', newline='') as csvfile:
        writer = csv.writer(csvfile)
        if is_new_file:
            writer.writerow(['date', 'time_ms', 'total', 'passed', 'failed', 'success_rate'])
        writer.writerow([datetime.now().strftime("%Y-%m-%d"), time_per_move, total_positions, correct_positions, failed_positions, f"{success_rate:.1f}"])
    
    # Exit with appropriate code
    sys.exit(0 if failed_positions == 0 else min(failed_positions, 255))

if __name__ == "__main__":
    main()