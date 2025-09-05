#!/bin/bash
# TT Remediation Phase 1.3: A/B Testing Baseline Script
# Records baseline metrics before implementing TT store fixes

echo "=== TT Baseline Testing Script ==="
echo "Date: $(date)"
echo "Branch: $(git branch --show-current)"
echo "Commit: $(git rev-parse HEAD)"
echo ""

SEAJAY_BIN="./bin/seajay"
OUTPUT_FILE="tt_baseline_metrics.txt"

# Test positions
POSITIONS=(
    "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"  # Start position
    "r1bqkb1r/pppp1ppp/2n2n2/1B2p3/4P3/5N2/PPPP1PPP/RNBQK2R w KQkq - 4 4"  # Italian
    "r2q1rk1/ppp2ppp/2np1n2/2b1p1B1/2B1P1b1/3P1N2/PPP2PPP/RN1Q1RK1 w - - 0 10"  # Tactical middlegame
)

DEPTHS=(6 8 10 12)
HASH_SIZES=(1 16 64)  # MB

echo "=== Testing Configuration ===" | tee "$OUTPUT_FILE"
echo "Positions: ${#POSITIONS[@]}" | tee -a "$OUTPUT_FILE"
echo "Depths: ${DEPTHS[*]}" | tee -a "$OUTPUT_FILE"
echo "Hash sizes: ${HASH_SIZES[*]} MB" | tee -a "$OUTPUT_FILE"
echo "" | tee -a "$OUTPUT_FILE"

for hash_size in "${HASH_SIZES[@]}"; do
    echo "=== Hash Size: ${hash_size} MB ===" | tee -a "$OUTPUT_FILE"
    
    for pos_idx in "${!POSITIONS[@]}"; do
        position="${POSITIONS[$pos_idx]}"
        echo "" | tee -a "$OUTPUT_FILE"
        echo "Position $((pos_idx+1)): ${position:0:30}..." | tee -a "$OUTPUT_FILE"
        
        for depth in "${DEPTHS[@]}"; do
            echo -n "  Depth $depth: " | tee -a "$OUTPUT_FILE"
            
            # Run search and extract metrics
            result=$(cat << EOF | $SEAJAY_BIN 2>&1 | grep -E "(SearchStats:|TT Collision|Missing TT)"
position fen $position
setoption name Hash value $hash_size
setoption name SearchStats value true
go depth $depth
debug tt
quit
EOF
            )
            
            # Extract key metrics
            nodes=$(echo "$result" | grep -o "nodes=[0-9]*" | cut -d= -f2 | head -1)
            tt_hit_rate=$(echo "$result" | grep -o "hit%=[0-9.]*" | cut -d= -f2 | head -1)
            null_no_store=$(echo "$result" | grep -o "no-store=[0-9]*" | cut -d= -f2 | head -1)
            static_no_store=$(echo "$result" | grep -o "static-no-store=[0-9]*" | cut -d= -f2 | head -1)
            probe_collisions=$(echo "$result" | grep "Probe mismatches" | grep -o "[0-9]* (" | cut -d' ' -f1)
            collision_rate=$(echo "$result" | grep "Probe mismatches" | grep -o "([0-9.]*%)" | tr -d '()%')
            
            echo "nodes=$nodes hit%=$tt_hit_rate null-no-store=$null_no_store static-no-store=$static_no_store collisions=$probe_collisions coll%=$collision_rate" | tee -a "$OUTPUT_FILE"
        done
    done
    echo "" | tee -a "$OUTPUT_FILE"
done

echo "=== Summary Statistics ===" | tee -a "$OUTPUT_FILE"
echo "" | tee -a "$OUTPUT_FILE"

# Calculate averages and trends
echo "Key Findings:" | tee -a "$OUTPUT_FILE"
echo "1. Hit rate trend with depth (expecting decrease currently)" | tee -a "$OUTPUT_FILE"
echo "2. Collision rates at different hash sizes" | tee -a "$OUTPUT_FILE"
echo "3. Missing TT stores for null-move and static-null pruning" | tee -a "$OUTPUT_FILE"
echo "" | tee -a "$OUTPUT_FILE"

echo "Baseline testing complete. Results saved to $OUTPUT_FILE"
echo "Next step: Implement Phase 2 (Add missing TT stores) and re-run for comparison"