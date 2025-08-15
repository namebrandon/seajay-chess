#!/bin/bash
# Stage 15 Day 6.5: SPRT Testing Preparation for SEE Pruning
# Expected ELO gain: 30-50 ELO for aggressive pruning

echo "=== Stage 15 Day 6.5: SPRT Test Preparation ==="
echo ""
echo "This script prepares configurations for SPRT testing of SEE pruning"
echo ""

# Create test configurations directory
mkdir -p /workspace/sprt_configs

# Configuration 1: Conservative vs No Pruning
cat > /workspace/sprt_configs/see_pruning_conservative.ini << EOF
[SPRT]
# Test: SEE Conservative Pruning vs No Pruning
# Expected gain: 15-25 ELO
test_name = SEE_Conservative_Pruning
elo0 = 0
elo1 = 20
alpha = 0.05
beta = 0.05

[Engine1]
name = SeaJay-Conservative
command = /workspace/bin/seajay
options = SEEPruning=conservative SEEMode=production

[Engine2]
name = SeaJay-NoPruning
command = /workspace/bin/seajay
options = SEEPruning=off SEEMode=production

[TimeControl]
base_time = 10000
increment = 100
EOF

# Configuration 2: Aggressive vs No Pruning
cat > /workspace/sprt_configs/see_pruning_aggressive.ini << EOF
[SPRT]
# Test: SEE Aggressive Pruning vs No Pruning
# Expected gain: 30-50 ELO
test_name = SEE_Aggressive_Pruning
elo0 = 0
elo1 = 40
alpha = 0.05
beta = 0.05

[Engine1]
name = SeaJay-Aggressive
command = /workspace/bin/seajay
options = SEEPruning=aggressive SEEMode=production

[Engine2]
name = SeaJay-NoPruning  
command = /workspace/bin/seajay
options = SEEPruning=off SEEMode=production

[TimeControl]
base_time = 10000
increment = 100
EOF

# Configuration 3: Aggressive vs Conservative
cat > /workspace/sprt_configs/see_pruning_aggressive_vs_conservative.ini << EOF
[SPRT]
# Test: SEE Aggressive vs Conservative Pruning
# Expected gain: 15-25 ELO
test_name = SEE_Aggressive_vs_Conservative
elo0 = 0
elo1 = 20
alpha = 0.05
beta = 0.05

[Engine1]
name = SeaJay-Aggressive
command = /workspace/bin/seajay
options = SEEPruning=aggressive SEEMode=production

[Engine2]
name = SeaJay-Conservative
command = /workspace/bin/seajay
options = SEEPruning=conservative SEEMode=production

[TimeControl]
base_time = 10000
increment = 100
EOF

# Configuration 4: SEE+Pruning vs MVV-LVA only
cat > /workspace/sprt_configs/see_full_vs_mvvlva.ini << EOF
[SPRT]
# Test: Full SEE with Aggressive Pruning vs MVV-LVA only
# Expected gain: 40-60 ELO (combined benefit)
test_name = SEE_Full_vs_MVVLVA
elo0 = 0
elo1 = 50
alpha = 0.05
beta = 0.05

[Engine1]
name = SeaJay-SEE-Full
command = /workspace/bin/seajay
options = SEEPruning=aggressive SEEMode=production

[Engine2]
name = SeaJay-MVVLVA
command = /workspace/bin/seajay
options = SEEPruning=off SEEMode=off

[TimeControl]
base_time = 10000
increment = 100
EOF

echo "Created SPRT configurations in /workspace/sprt_configs/"
echo ""
echo "Configurations created:"
echo "1. see_pruning_conservative.ini - Conservative pruning vs No pruning (expected +20 ELO)"
echo "2. see_pruning_aggressive.ini - Aggressive pruning vs No pruning (expected +40 ELO)"
echo "3. see_pruning_aggressive_vs_conservative.ini - Aggressive vs Conservative (expected +20 ELO)"
echo "4. see_full_vs_mvvlva.ini - Full SEE+Pruning vs MVV-LVA only (expected +50 ELO)"
echo ""

# Create cutechess-cli test script
cat > /workspace/sprt_configs/run_sprt_test.sh << 'SCRIPT'
#!/bin/bash
# Run SPRT test with cutechess-cli

if [ "$#" -ne 1 ]; then
    echo "Usage: $0 <config_file>"
    echo "Example: $0 see_pruning_aggressive.ini"
    exit 1
fi

CONFIG=$1

# Check if cutechess-cli is available
if ! command -v cutechess-cli &> /dev/null; then
    echo "cutechess-cli not found. Please install it first."
    exit 1
fi

# Run the SPRT test
cutechess-cli \
    -engine conf=SeaJay-Test cmd=/workspace/bin/seajay \
    -engine conf=SeaJay-Base cmd=/workspace/bin/seajay \
    -each proto=uci tc=10+0.1 \
    -games 2000 \
    -repeat \
    -recover \
    -pgnout sprt_results.pgn \
    -ratinginterval 10 \
    -concurrency 4 \
    -sprt elo0=0 elo1=40 alpha=0.05 beta=0.05

echo "SPRT test complete. Results saved to sprt_results.pgn"
SCRIPT

chmod +x /workspace/sprt_configs/run_sprt_test.sh

echo "Created SPRT test runner: /workspace/sprt_configs/run_sprt_test.sh"
echo ""
echo "To run an SPRT test:"
echo "  cd /workspace/sprt_configs"
echo "  ./run_sprt_test.sh <config_file>"
echo ""
echo "Note: You'll need cutechess-cli installed to run the actual tests."
echo ""

# Create a simple A/B comparison script
cat > /workspace/tests/compare_see_modes.sh << 'COMPARE'
#!/bin/bash
# Simple A/B comparison of SEE pruning modes

echo "=== SEE Pruning Mode Comparison ==="
echo ""

SEAJAY="/workspace/bin/seajay"
DEPTH=8

# Test positions for comparison
declare -a POSITIONS=(
    "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"
    "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1"
    "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1"
    "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1"
    "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8"
)

echo "Testing each mode at depth $DEPTH..."
echo ""

for mode in "off" "conservative" "aggressive"; do
    echo "Mode: $mode"
    total_nodes=0
    total_time=0
    
    for pos in "${POSITIONS[@]}"; do
        result=$(echo -e "uci\nsetoption name SEEPruning value $mode\nsetoption name SEEMode value production\nposition fen $pos\ngo depth $DEPTH\nquit" | $SEAJAY 2>&1)
        
        nodes=$(echo "$result" | grep "info depth $DEPTH" | tail -1 | sed -n 's/.*nodes \([0-9]*\).*/\1/p')
        time=$(echo "$result" | grep "info depth $DEPTH" | tail -1 | sed -n 's/.*time \([0-9]*\).*/\1/p')
        
        if [ -n "$nodes" ]; then
            total_nodes=$((total_nodes + nodes))
        fi
        if [ -n "$time" ]; then
            total_time=$((total_time + time))
        fi
    done
    
    echo "  Total nodes: $total_nodes"
    echo "  Total time: ${total_time}ms"
    if [ $total_time -gt 0 ]; then
        avg_nps=$((total_nodes * 1000 / total_time))
        echo "  Average NPS: $avg_nps"
    fi
    echo ""
done

echo "Comparison complete!"
COMPARE

chmod +x /workspace/tests/compare_see_modes.sh

echo "Created comparison script: /workspace/tests/compare_see_modes.sh"
echo ""
echo "SPRT preparation complete!"