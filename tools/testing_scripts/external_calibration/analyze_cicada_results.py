#!/usr/bin/env python3
"""
Analyze SPRT test results for SeaJay vs Cicada comparison
"""

import sys
import re
import os
from pathlib import Path

def parse_console_output(filepath):
    """Parse the console output file for test results"""
    
    if not os.path.exists(filepath):
        print(f"Error: File not found: {filepath}")
        return None
    
    with open(filepath, 'r') as f:
        content = f.read()
    
    results = {}
    
    # Parse score line: "Score of SeaJay vs Cicada: W - L - D [win_rate]"
    score_match = re.search(r'Score of (.+?) vs (.+?): (\d+) - (\d+) - (\d+) \[([0-9.]+)\]', content)
    if score_match:
        results['engine1'] = score_match.group(1)
        results['engine2'] = score_match.group(2)
        results['wins'] = int(score_match.group(3))
        results['losses'] = int(score_match.group(4))
        results['draws'] = int(score_match.group(5))
        results['win_rate'] = float(score_match.group(6))
        results['total_games'] = results['wins'] + results['losses'] + results['draws']
    
    # Parse Elo difference
    elo_match = re.search(r'Elo: ([+-]?[0-9.]+) \+/- ([0-9.]+)', content)
    if elo_match:
        results['elo_diff'] = float(elo_match.group(1))
        results['elo_error'] = float(elo_match.group(2))
    
    # Parse SPRT result
    sprt_match = re.search(r'SPRT: LLR = ([+-]?[0-9.]+) \[([+-]?[0-9.]+), ([+-]?[0-9.]+)\]', content)
    if sprt_match:
        results['llr'] = float(sprt_match.group(1))
        results['llr_lower'] = float(sprt_match.group(2))
        results['llr_upper'] = float(sprt_match.group(3))
    
    # Parse SPRT decision
    if 'H1 accepted' in content:
        results['decision'] = 'H1_ACCEPTED'
        results['conclusion'] = 'SeaJay is within 100 ELO of Cicada'
    elif 'H0 accepted' in content:
        results['decision'] = 'H0_ACCEPTED'
        results['conclusion'] = 'SeaJay is more than 100 ELO weaker than Cicada'
    else:
        results['decision'] = 'INCONCLUSIVE'
        results['conclusion'] = 'Test did not reach statistical significance'
    
    return results

def generate_report(results):
    """Generate a detailed analysis report"""
    
    if not results:
        return "No results to analyze"
    
    report = []
    report.append("=" * 60)
    report.append("SPRT TEST ANALYSIS: SeaJay vs Cicada")
    report.append("=" * 60)
    report.append("")
    
    # Basic statistics
    report.append("GAME STATISTICS:")
    report.append(f"  Total games: {results.get('total_games', 'N/A')}")
    report.append(f"  SeaJay wins: {results.get('wins', 'N/A')}")
    report.append(f"  SeaJay losses: {results.get('losses', 'N/A')}")
    report.append(f"  Draws: {results.get('draws', 'N/A')}")
    
    if 'total_games' in results and results['total_games'] > 0:
        win_pct = (results['wins'] / results['total_games']) * 100
        loss_pct = (results['losses'] / results['total_games']) * 100
        draw_pct = (results['draws'] / results['total_games']) * 100
        report.append(f"  Win rate: {win_pct:.1f}%")
        report.append(f"  Loss rate: {loss_pct:.1f}%")
        report.append(f"  Draw rate: {draw_pct:.1f}%")
    
    report.append("")
    
    # Elo analysis
    report.append("ELO DIFFERENCE:")
    if 'elo_diff' in results:
        elo = results['elo_diff']
        error = results.get('elo_error', 0)
        report.append(f"  Estimated: {elo:+.1f} ± {error:.1f} ELO")
        report.append(f"  95% confidence interval: [{elo-2*error:+.1f}, {elo+2*error:+.1f}]")
        
        # Interpretation
        if elo > 0:
            report.append(f"  → SeaJay appears {elo:.0f} ELO stronger than Cicada")
        else:
            report.append(f"  → SeaJay appears {abs(elo):.0f} ELO weaker than Cicada")
    
    report.append("")
    
    # SPRT analysis
    report.append("SPRT ANALYSIS:")
    if 'llr' in results:
        report.append(f"  Log-likelihood ratio: {results['llr']:.2f}")
        report.append(f"  Bounds: [{results.get('llr_lower', 'N/A')}, {results.get('llr_upper', 'N/A')}]")
        report.append(f"  Decision: {results.get('decision', 'N/A')}")
    
    report.append("")
    
    # Conclusion
    report.append("CONCLUSION:")
    report.append(f"  {results.get('conclusion', 'Unable to determine')}")
    
    if 'elo_diff' in results:
        target_elo = -100  # Our hypothesis threshold
        actual_diff = results['elo_diff']
        
        report.append("")
        report.append("PERFORMANCE ASSESSMENT:")
        
        if actual_diff > 0:
            report.append(f"  ✓ SeaJay is STRONGER than Cicada by {actual_diff:.0f} ELO")
            report.append("  → Excellent result! SeaJay exceeds expectations.")
        elif actual_diff >= target_elo:
            gap = abs(actual_diff)
            report.append(f"  ✓ SeaJay is within target range ({gap:.0f} ELO weaker)")
            report.append("  → Good result! SeaJay is competitive with Cicada.")
        else:
            gap = abs(actual_diff)
            deficit = gap - 100
            report.append(f"  ✗ SeaJay is {gap:.0f} ELO weaker (missed target by {deficit:.0f} ELO)")
            report.append("  → More development needed to reach competitive level.")
    
    report.append("")
    report.append("=" * 60)
    
    return "\n".join(report)

def main():
    if len(sys.argv) < 2:
        # Try to find the most recent console output
        test_dir = Path("/workspace/sprt_results/SPRT-2025-EXT-001-CICADA")
        if test_dir.exists():
            console_files = list(test_dir.glob("console_output_*.txt"))
            if console_files:
                filepath = str(max(console_files, key=os.path.getctime))
                print(f"Using most recent file: {filepath}")
            else:
                print("Usage: python3 analyze_cicada_results.py <console_output_file>")
                print("No console output files found in test directory")
                sys.exit(1)
        else:
            print("Usage: python3 analyze_cicada_results.py <console_output_file>")
            print("Test directory not found")
            sys.exit(1)
    else:
        filepath = sys.argv[1]
    
    results = parse_console_output(filepath)
    
    if results:
        report = generate_report(results)
        print(report)
        
        # Save report
        output_path = filepath.replace("console_output", "analysis_report").replace(".txt", ".txt")
        with open(output_path, 'w') as f:
            f.write(report)
        print(f"\nReport saved to: {output_path}")
    else:
        print("Failed to parse results")

if __name__ == "__main__":
    main()