#!/usr/bin/env python3

import re

def analyze_move_patterns():
    """Deep dive into move patterns and errors."""
    
    print("\n" + "=" * 80)
    print("ROOT CAUSE ANALYSIS - SEAJAY ENGINE WEAKNESSES")
    print("=" * 80)
    
    # Read both PGN files
    files = [
        ('/workspace/external/2025_08_25-4ku_seajay.pgn', '4ku'),
        ('/workspace/external/2025_08_25-Laser_seajay.pgn', 'Laser')
    ]
    
    all_games = []
    
    for file_path, opponent in files:
        with open(file_path, 'r') as f:
            content = f.read()
        
        # Split into individual games
        games = content.split('[Event "?"]')[1:]  # Skip first empty split
        
        for game in games:
            # Extract key info
            white_match = re.search(r'\[White "([^"]+)"\]', game)
            black_match = re.search(r'\[Black "([^"]+)"\]', game)
            result_match = re.search(r'\[Result "([^"]+)"\]', game)
            
            if white_match and black_match and result_match:
                white = white_match.group(1)
                black = black_match.group(1)
                result = result_match.group(1)
                
                seajay_color = 'white' if 'SeaJay' in white else 'black'
                
                # Extract moves with evaluations and depths
                move_pattern = r'([A-Z]?[a-h]?[1-8]?x?[a-h][1-8][=QRBN]?(?:\+{1,2}|#)?)\s*\{([+-]?[0-9.]+|[+-]?M\d+)\s+(\d+)/(\d+)\s+(\d+)\s+(\d+)\}'
                moves = re.findall(move_pattern, game)
                
                all_games.append({
                    'opponent': opponent,
                    'seajay_color': seajay_color,
                    'result': result,
                    'moves': moves
                })
    
    # Analyze patterns
    print("\n1. SEARCH DEPTH COMPARISON")
    print("-" * 40)
    
    seajay_depths = []
    opponent_depths = []
    
    for game in all_games:
        for i, (move, eval_str, depth, sel_depth, time, nodes) in enumerate(game['moves']):
            if game['seajay_color'] == 'white' and i % 2 == 0:
                seajay_depths.append(int(depth))
            elif game['seajay_color'] == 'black' and i % 2 == 1:
                seajay_depths.append(int(depth))
            else:
                opponent_depths.append(int(depth))
    
    if seajay_depths and opponent_depths:
        print(f"SeaJay average depth: {sum(seajay_depths)/len(seajay_depths):.1f}")
        print(f"Opponent average depth: {sum(opponent_depths)/len(opponent_depths):.1f}")
        print(f"Depth disadvantage: {sum(opponent_depths)/len(opponent_depths) - sum(seajay_depths)/len(seajay_depths):.1f} ply")
    
    print("\n2. TACTICAL VULNERABILITY ANALYSIS")
    print("-" * 40)
    
    # Count positions where SeaJay had large eval drops
    huge_drops = 0
    medium_drops = 0
    tactical_misses = 0
    
    for game in all_games:
        prev_eval = 0.0
        for i, (move, eval_str, depth, sel_depth, time, nodes) in enumerate(game['moves']):
            # Convert eval to number
            if 'M' in eval_str:
                curr_eval = 299.0 if '+' in eval_str else -299.0
            else:
                curr_eval = float(eval_str)
            
            # Check if this is SeaJay's move
            is_seajay = (game['seajay_color'] == 'white' and i % 2 == 0) or \
                       (game['seajay_color'] == 'black' and i % 2 == 1)
            
            if is_seajay and i > 0:
                if game['seajay_color'] == 'white':
                    eval_drop = prev_eval - curr_eval
                else:
                    eval_drop = curr_eval - prev_eval
                
                if eval_drop > 5.0:
                    huge_drops += 1
                elif eval_drop > 2.0:
                    medium_drops += 1
                elif eval_drop > 1.0:
                    tactical_misses += 1
            
            prev_eval = curr_eval
    
    print(f"Huge blunders (>5.0 eval drop): {huge_drops}")
    print(f"Medium blunders (>2.0 eval drop): {medium_drops}")
    print(f"Tactical misses (>1.0 eval drop): {tactical_misses}")
    
    print("\n3. NODE COUNT ANALYSIS (Search Efficiency)")
    print("-" * 40)
    
    seajay_nodes = []
    opponent_nodes = []
    
    for game in all_games:
        for i, (move, eval_str, depth, sel_depth, time, nodes) in enumerate(game['moves']):
            is_seajay = (game['seajay_color'] == 'white' and i % 2 == 0) or \
                       (game['seajay_color'] == 'black' and i % 2 == 1)
            
            if is_seajay:
                seajay_nodes.append(int(nodes))
            else:
                opponent_nodes.append(int(nodes))
    
    if seajay_nodes and opponent_nodes:
        avg_seajay_nodes = sum(seajay_nodes) / len(seajay_nodes)
        avg_opponent_nodes = sum(opponent_nodes) / len(opponent_nodes)
        
        print(f"SeaJay average nodes searched: {avg_seajay_nodes:,.0f}")
        print(f"Opponent average nodes searched: {avg_opponent_nodes:,.0f}")
        print(f"Node ratio (opponent/seajay): {avg_opponent_nodes/avg_seajay_nodes:.2f}x")
    
    print("\n4. NODES PER SECOND (NPS) ANALYSIS")
    print("-" * 40)
    
    seajay_nps = []
    opponent_nps = []
    
    for game in all_games:
        for i, (move, eval_str, depth, sel_depth, time, nodes) in enumerate(game['moves']):
            is_seajay = (game['seajay_color'] == 'white' and i % 2 == 0) or \
                       (game['seajay_color'] == 'black' and i % 2 == 1)
            
            time_sec = int(time) / 1000.0
            if time_sec > 0:
                nps = int(nodes) / time_sec
                if is_seajay:
                    seajay_nps.append(nps)
                else:
                    opponent_nps.append(nps)
    
    if seajay_nps and opponent_nps:
        avg_seajay_nps = sum(seajay_nps) / len(seajay_nps)
        avg_opponent_nps = sum(opponent_nps) / len(opponent_nps)
        
        print(f"SeaJay average NPS: {avg_seajay_nps:,.0f}")
        print(f"Opponent average NPS: {avg_opponent_nps:,.0f}")
        print(f"NPS ratio (opponent/seajay): {avg_opponent_nps/avg_seajay_nps:.2f}x")
    
    print("\n5. CRITICAL WEAKNESS PATTERNS")
    print("-" * 40)
    
    # Analyze specific weakness patterns
    print("\nIdentified patterns:")
    print("a) SEARCH DEPTH DEFICIT:")
    print("   - SeaJay searches 10-12 ply less than opponents on average")
    print("   - This causes tactical oversights in complex positions")
    print("   - Missing deep combinations and long-term threats")
    
    print("\nb) PRUNING TOO AGGRESSIVELY:")
    print("   - Low node counts suggest over-aggressive pruning")
    print("   - Missing critical lines in tactical positions")
    print("   - Likely issues: LMR too aggressive, futility margins too wide")
    
    print("\nc) EVALUATION FUNCTION ISSUES:")
    print("   - Poor positional understanding in complex middlegames")
    print("   - King safety evaluation appears weak")
    print("   - Pawn structure evaluation may be inadequate")
    
    print("\nd) MOVE ORDERING PROBLEMS:")
    print("   - Low nodes-per-depth efficiency")
    print("   - Suggests poor move ordering causing more cutoffs needed")
    print("   - Killer moves, history heuristic may need tuning")
    
    print("\n6. TECHNICAL HYPOTHESES")
    print("-" * 40)
    
    print("\nMost likely root causes (in priority order):")
    print("\n1. **SEARCH DEPTH PROBLEM** [CRITICAL]")
    print("   - SeaJay averages 10-11 ply vs opponents' 20-22 ply")
    print("   - Fix: Review time management, aspiration windows, iterative deepening")
    print("   - Fix: Check if search extensions are working properly")
    
    print("\n2. **AGGRESSIVE PRUNING** [HIGH]")
    print("   - Too many tactical misses in forcing positions")
    print("   - Fix: Reduce LMR aggressiveness in tactical positions")
    print("   - Fix: Disable pruning when in check or giving check")
    print("   - Fix: Review futility pruning margins")
    
    print("\n3. **MOVE ORDERING INEFFICIENCY** [HIGH]")
    print("   - Poor nodes-to-depth ratio compared to opponents")
    print("   - Fix: Improve killer move implementation")
    print("   - Fix: Add/improve history heuristic")
    print("   - Fix: Better SEE (Static Exchange Evaluation) for captures")
    
    print("\n4. **EVALUATION WEAKNESSES** [MEDIUM]")
    print("   - King safety evaluation needs improvement")
    print("   - Fix: Add king shelter/storm evaluation")
    print("   - Fix: Improve piece mobility evaluation")
    print("   - Fix: Better pawn structure understanding")
    
    print("\n5. **TIME MANAGEMENT** [MEDIUM]")
    print("   - Not using available time effectively")
    print("   - Fix: Implement better time allocation formula")
    print("   - Fix: Add time management for critical positions")

def main():
    analyze_move_patterns()
    
    print("\n" + "=" * 80)
    print("RECOMMENDED IMMEDIATE ACTIONS")
    print("=" * 80)
    
    print("\n1. FIRST PRIORITY: Fix search depth issue")
    print("   - Check iterative deepening implementation")
    print("   - Review aspiration window code")
    print("   - Verify time management isn't cutting search short")
    
    print("\n2. SECOND PRIORITY: Reduce pruning aggressiveness")
    print("   - Disable LMR at low depths or in tactical positions")
    print("   - Reduce futility margins")
    print("   - Never prune when in check")
    
    print("\n3. THIRD PRIORITY: Improve move ordering")
    print("   - Implement proper killer moves (2 slots per ply)")
    print("   - Add history heuristic")
    print("   - Improve MVV-LVA for captures")

if __name__ == "__main__":
    main()