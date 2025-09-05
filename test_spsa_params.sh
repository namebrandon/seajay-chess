#!/bin/bash
# Test SeaJay with SPSA-tuned pruning parameters
# Values from tuning session provided by user

echo "Testing SeaJay with SPSA-tuned pruning parameters..."
echo ""

# Create UCI commands with SPSA values
cat << EOF | ./bin/seajay 2>&1 | grep -E "(depth 10|nodes|nps|Move ordering|Cutoff|TT moves:|info string ===)"
uci
setoption name NodeExplosionDiagnostics value true
setoption name FutilityBase value 240
setoption name FutilityScale value 73
setoption name RazorMargin1 value 274
setoption name RazorMargin2 value 468
setoption name NullMoveEvalMargin value 198
setoption name NullMoveStaticMargin value 87
setoption name NullMoveMinDepth value 2
setoption name NullMoveReductionBase value 4
setoption name NullMoveReductionDepth6 value 4
setoption name NullMoveReductionDepth12 value 5
setoption name MoveCountLimit3 value 7
setoption name MoveCountLimit4 value 15
setoption name MoveCountLimit5 value 20
setoption name MoveCountLimit6 value 25
setoption name MoveCountHistoryThreshold value 0
setoption name LMRMinDepth value 2
setoption name LMRMinMoveNumber value 2
position fen r1b1k2r/pp4pp/3Bpp2/3p4/6q1/8/PQ3PPP/2R1R1K1 b kq - 3 17
go depth 10
quit
EOF

echo ""
echo "Now testing with default parameters for comparison..."
echo ""

# Test with default parameters
cat << EOF | ./bin/seajay 2>&1 | grep -E "(depth 10|nodes|nps|Move ordering|Cutoff|TT moves:|info string ===)"
uci
setoption name NodeExplosionDiagnostics value true
position fen r1b1k2r/pp4pp/3Bpp2/3p4/6q1/8/PQ3PPP/2R1R1K1 b kq - 3 17
go depth 10
quit
EOF