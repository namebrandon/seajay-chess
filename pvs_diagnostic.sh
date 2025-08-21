PVS Diagnostic Test Generator
==============================


Expected PVS Behavior Analysis:
================================

In a properly functioning PVS implementation:

1. Scout Search Window:
   - Scout uses null window: [alpha, alpha+1]
   - This is a minimal window to quickly test if move beats alpha

2. Re-search Triggers:
   - Scout fails high: score > alpha
   - But score might be >> alpha+1 due to fail-soft
   - Need full window search to get exact score

3. Your Implementation Issue:
   - Scout search: -(alpha+1), -alpha (negated becomes [alpha, alpha+1])
   - Fail high returns score >= alpha+1
   - Your condition: score > alpha && score < beta
   - Problem: score is often >= beta after scout fail-high!

4. The Fix:
   - Change condition to: if (score > alpha)
   - This triggers re-search whenever scout finds better move
   - But wait... you might be doing fail-hard in scout?


Generating diagnostic script...
--------------------------------

#!/bin/bash
# PVS Diagnostic Script
# This script tests positions that should trigger PVS re-searches

ENGINE="./bin/seajay"
RESULTS_FILE="pvs_diagnostic_results.txt"

echo "PVS Diagnostic Test Results" > $RESULTS_FILE
echo "===========================" >> $RESULTS_FILE
echo "" >> $RESULTS_FILE

echo "Testing: Tactical Position 1" | tee -a $RESULTS_FILE
echo "FEN: r1bqkb1r/pppp1ppp/2n2n2/1B2p3/4P3/5N2/PPPP1PPP/RNBQK2R w KQkq - 4 4" >> $RESULTS_FILE
echo "Description: After 1.e4 e5 2.Nf3 Nc6 3.Bb5 Nf6 - Multiple good moves" >> $RESULTS_FILE
echo "" >> $RESULTS_FILE
$ENGINE << EOF | grep -E "(bestmove|PVS|re-search|depth 6)" | tee -a $RESULTS_FILE
uci
setoption name ShowPVSStats value true
position fen r1bqkb1r/pppp1ppp/2n2n2/1B2p3/4P3/5N2/PPPP1PPP/RNBQK2R w KQkq - 4 4
go depth 6
quit
EOF

echo "" >> $RESULTS_FILE
echo "----------------------------------------" >> $RESULTS_FILE
echo "" >> $RESULTS_FILE

echo "Testing: Complex Middle Game" | tee -a $RESULTS_FILE
echo "FEN: r1bq1rk1/pp2ppbp/2np1np1/8/3PP3/2N2N2/PPP2PPP/R1BQKB1R w KQ - 0 8" >> $RESULTS_FILE
echo "Description: King's Indian structure - many moves have similar evaluations" >> $RESULTS_FILE
echo "" >> $RESULTS_FILE
$ENGINE << EOF | grep -E "(bestmove|PVS|re-search|depth 7)" | tee -a $RESULTS_FILE
uci
setoption name ShowPVSStats value true
position fen r1bq1rk1/pp2ppbp/2np1np1/8/3PP3/2N2N2/PPP2PPP/R1BQKB1R w KQ - 0 8
go depth 7
quit
EOF

echo "" >> $RESULTS_FILE
echo "----------------------------------------" >> $RESULTS_FILE
echo "" >> $RESULTS_FILE

echo "Testing: Endgame with Multiple Paths" | tee -a $RESULTS_FILE
echo "FEN: 8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1" >> $RESULTS_FILE
echo "Description: Rook endgame where move order matters" >> $RESULTS_FILE
echo "" >> $RESULTS_FILE
$ENGINE << EOF | grep -E "(bestmove|PVS|re-search|depth 8)" | tee -a $RESULTS_FILE
uci
setoption name ShowPVSStats value true
position fen 8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1
go depth 8
quit
EOF

echo "" >> $RESULTS_FILE
echo "----------------------------------------" >> $RESULTS_FILE
echo "" >> $RESULTS_FILE

echo "Testing: Position After Exchange" | tee -a $RESULTS_FILE
echo "FEN: rnbqk2r/pp2ppbp/3p1np1/8/3NP3/2N5/PPP2PPP/R1BQKB1R w KQkq - 0 7" >> $RESULTS_FILE
echo "Description: Position after exchange - unstable evaluation" >> $RESULTS_FILE
echo "" >> $RESULTS_FILE
$ENGINE << EOF | grep -E "(bestmove|PVS|re-search|depth 6)" | tee -a $RESULTS_FILE
uci
setoption name ShowPVSStats value true
position fen rnbqk2r/pp2ppbp/3p1np1/8/3NP3/2N5/PPP2PPP/R1BQKB1R w KQkq - 0 7
go depth 6
quit
EOF

echo "" >> $RESULTS_FILE
echo "----------------------------------------" >> $RESULTS_FILE
echo "" >> $RESULTS_FILE

echo "Testing: Critical Tactical Position" | tee -a $RESULTS_FILE
echo "FEN: r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1" >> $RESULTS_FILE
echo "Description: Wild tactical position with many forcing moves" >> $RESULTS_FILE
echo "" >> $RESULTS_FILE
$ENGINE << EOF | grep -E "(bestmove|PVS|re-search|depth 5)" | tee -a $RESULTS_FILE
uci
setoption name ShowPVSStats value true
position fen r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1
go depth 5
quit
EOF

echo "" >> $RESULTS_FILE
echo "----------------------------------------" >> $RESULTS_FILE
echo "" >> $RESULTS_FILE

echo "Test complete. Results saved to $RESULTS_FILE"
cat $RESULTS_FILE
