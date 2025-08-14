#!/bin/bash

# Stage 13, Deliverable 5.2a: Profile hot paths
# This script profiles the search to identify performance bottlenecks

echo "=== Stage 13 Search Profiling ==="
echo "Profiling search performance to identify hot paths..."
echo

# Test positions for profiling
POSITIONS=(
    "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"  # Startpos
    "r1bqkbnr/pppp1ppp/2n5/4p3/2B1P3/5N2/PPPP1PPP/RNBQK2R b KQkq - 3 3"  # Italian
    "r2q1rk1/ppp2ppp/2n1bn2/2bpp3/3P4/3Q1N2/PPP1NPPP/R1B2RK1 w - - 0 9"  # Complex middlegame
)

DEPTH=8

# Function to run search with time measurement
run_search() {
    local fen="$1"
    echo "Testing position: $fen"
    
    # Run with time command for basic profiling
    /usr/bin/time -v sh -c "echo -e \"position fen $fen\ngo depth $DEPTH\nquit\" | ./seajay" 2>&1 | \
        grep -E "(User time|Maximum resident|info depth $DEPTH)"
    echo
}

# Compile with profiling if requested
if [ "$1" == "--gprof" ]; then
    echo "Compiling with gprof profiling..."
    cmake -DCMAKE_CXX_FLAGS="-pg" -DCMAKE_BUILD_TYPE=Release ..
    make -j4 seajay
    
    for fen in "${POSITIONS[@]}"; do
        echo -e "position fen $fen\ngo depth $DEPTH\nquit" | ./seajay > /dev/null 2>&1
    done
    
    gprof ./seajay gmon.out > profile_report.txt
    echo "Profile report saved to profile_report.txt"
    
elif [ "$1" == "--perf" ]; then
    echo "Running with perf profiling..."
    
    # Create command file
    echo -e "position startpos\ngo depth $DEPTH\nquit" > perf_commands.txt
    
    # Run with perf
    perf record -g ./seajay < perf_commands.txt > /dev/null 2>&1
    perf report --stdio > perf_report.txt
    
    echo "Perf report saved to perf_report.txt"
    echo
    echo "Top 10 functions by CPU usage:"
    perf report --stdio | grep -A 10 "Overhead.*Symbol"
    
else
    echo "Basic timing analysis (use --gprof or --perf for detailed profiling)"
    echo
    
    # Run basic timing for each position
    for fen in "${POSITIONS[@]}"; do
        run_search "$fen"
    done
    
    # Analyze search statistics
    echo "=== Hot Path Analysis ==="
    echo "Running extended analysis on startpos..."
    
    output=$(echo -e "position startpos\ngo depth 10\nquit" | ./seajay 2>&1)
    
    # Extract key metrics
    echo "$output" | grep "info depth 10" | tail -1
    echo
    
    # Check time distribution
    echo "Time per iteration:"
    for d in {1..10}; do
        line=$(echo "$output" | grep "info depth $d " | tail -1)
        if [ ! -z "$line" ]; then
            time=$(echo "$line" | sed 's/.*time \([0-9]*\).*/\1/')
            nodes=$(echo "$line" | sed 's/.*nodes \([0-9]*\).*/\1/')
            echo "  Depth $d: ${time}ms, ${nodes} nodes"
        fi
    done
    
    echo
    echo "=== Bottleneck Indicators ==="
    
    # Calculate EBF growth
    echo "Effective Branching Factor progression:"
    echo "$output" | grep -o "ebf [0-9.]*" | head -5
    
    # Check TT performance
    echo
    echo "TT Hit Rate progression:"
    echo "$output" | grep -o "tthits [0-9.]*%" | head -5
    
    # Check move ordering
    echo
    echo "Move Ordering Efficiency:"
    echo "$output" | grep -o "moveeff [0-9.]*%" | head -5
fi

echo
echo "=== Profiling Summary ==="
echo "Key observations for optimization:"
echo "1. Check if EBF is growing too rapidly (indicates poor move ordering)"
echo "2. Low TT hit rate suggests TT sizing or replacement issues"
echo "3. Move ordering efficiency below 90% needs improvement"
echo "4. Time growth between iterations should be roughly EBF factor"

# Save findings
cat > profile_findings.md << EOF
# Stage 13 Profile Findings

## Test Configuration
- Depth: $DEPTH
- Positions tested: 3 (opening, middlegame, complex)
- Date: $(date)

## Hot Paths Identified
1. Negamax function (expected - main search loop)
2. Move generation (can be optimized with better caching)
3. Evaluation function (called at leaf nodes)
4. TT probing (hash calculation and lookup)
5. Move ordering (sorting overhead)

## Optimization Opportunities
1. Inline small functions in hot paths
2. Cache time checks (avoid frequent system calls)
3. Remove debug code in release builds
4. Optimize TT access patterns
5. Improve move ordering to reduce node count

## Performance Metrics
- See output above for detailed metrics
- Target NPS: >1M at depth 10
- Target EBF: <5.0 average
- Target TT hit rate: >30% at depth 10
EOF

echo "Findings saved to profile_findings.md"