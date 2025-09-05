#!/bin/bash

# Node Explosion Diagnostic Tool for SeaJay
# Compares node counts across multiple engines and positions

# Color codes for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
BOLD='\033[1m'
NC='\033[0m' # No Color

# Test positions array
declare -a POSITIONS=(
    "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1:Starting"
    "r1b1k2r/pp4pp/3Bpp2/3p4/6q1/8/PQ3PPP/2R1R1K1 b kq - 3 17:Middlegame"
    "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1:Endgame"
    "r1bqkb1r/pppp1ppp/2n2n2/1B2p3/4P3/5N2/PPPP1PPP/RNBQK2R w KQkq - 4 4:Tactical"
    "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1:Complex"
)

# Depths to test
DEPTHS="5 7 10"

# Output file
TIMESTAMP=$(date +%Y%m%d_%H%M%S)
REPORT_FILE="node_explosion_report_${TIMESTAMP}.txt"
CSV_FILE="node_explosion_data_${TIMESTAMP}.csv"

echo -e "${BOLD}==========================================
Node Explosion Diagnostic for SeaJay
==========================================${NC}"

# Initialize CSV
echo "Position,Depth,SeaJay,Stash,Komodo,Laser,SeaJay/Stash,SeaJay/Komodo,SeaJay/Laser" > "$CSV_FILE"

# Initialize report
{
    echo "Node Explosion Diagnostic Report"
    echo "Generated: $(date)"
    echo "========================================"
    echo
} > "$REPORT_FILE"

# Function to extract node count from analysis output
extract_nodes() {
    local engine="$1"
    local output="$2"
    echo "$output" | grep -A 20 "Performance:" | grep "$engine" | awk '{print $2}'
}

# Main testing loop
for pos_data in "${POSITIONS[@]}"; do
    IFS=':' read -r fen name <<< "$pos_data"
    
    echo -e "\n${BOLD}Testing position: ${YELLOW}$name${NC}"
    echo -e "${BLUE}FEN: $fen${NC}"
    
    {
        echo
        echo "Position: $name"
        echo "FEN: $fen"
        echo "----------------------------------------"
    } >> "$REPORT_FILE"
    
    for depth in $DEPTHS; do
        echo -e "  Depth ${depth}..."
        
        # Run analysis
        output=$(./tools/analyze_position.sh "$fen" depth "$depth" 2>&1)
        
        # Extract node counts
        seajay_nodes=$(echo "$output" | grep -A 4 "Performance:" | grep "SeaJay" | awk '{print $2}')
        stash_nodes=$(echo "$output" | grep -A 4 "Performance:" | grep "Stash" | awk '{print $2}')
        komodo_nodes=$(echo "$output" | grep -A 4 "Performance:" | grep "Komodo" | awk '{print $2}')
        laser_nodes=$(echo "$output" | grep -A 4 "Performance:" | grep "Laser" | awk '{print $2}')
        
        # Calculate ratios (handle division by zero)
        if [[ -n "$seajay_nodes" && -n "$stash_nodes" && "$stash_nodes" -gt 0 ]]; then
            ratio_stash=$(echo "scale=2; $seajay_nodes / $stash_nodes" | bc)
        else
            ratio_stash="N/A"
        fi
        
        if [[ -n "$seajay_nodes" && -n "$komodo_nodes" && "$komodo_nodes" -gt 0 ]]; then
            ratio_komodo=$(echo "scale=2; $seajay_nodes / $komodo_nodes" | bc)
        else
            ratio_komodo="N/A"
        fi
        
        if [[ -n "$seajay_nodes" && -n "$laser_nodes" && "$laser_nodes" -gt 0 ]]; then
            ratio_laser=$(echo "scale=2; $seajay_nodes / $laser_nodes" | bc)
        else
            ratio_laser="N/A"
        fi
        
        # Display results
        echo -e "    SeaJay:  ${RED}$seajay_nodes${NC} nodes"
        echo -e "    Stash:   ${GREEN}$stash_nodes${NC} nodes (ratio: ${YELLOW}${ratio_stash}x${NC})"
        echo -e "    Komodo:  ${GREEN}$komodo_nodes${NC} nodes (ratio: ${YELLOW}${ratio_komodo}x${NC})"
        echo -e "    Laser:   ${GREEN}$laser_nodes${NC} nodes (ratio: ${YELLOW}${ratio_laser}x${NC})"
        
        # Save to CSV
        echo "$name,$depth,$seajay_nodes,$stash_nodes,$komodo_nodes,$laser_nodes,$ratio_stash,$ratio_komodo,$ratio_laser" >> "$CSV_FILE"
        
        # Save to report
        {
            echo "  Depth $depth:"
            echo "    SeaJay:  $seajay_nodes nodes"
            echo "    Stash:   $stash_nodes nodes (ratio: ${ratio_stash}x)"
            echo "    Komodo:  $komodo_nodes nodes (ratio: ${ratio_komodo}x)"
            echo "    Laser:   $laser_nodes nodes (ratio: ${ratio_laser}x)"
        } >> "$REPORT_FILE"
    done
done

# Summary statistics
echo -e "\n${BOLD}==========================================
Summary Analysis
==========================================${NC}"

{
    echo
    echo "========================================"
    echo "SUMMARY ANALYSIS"
    echo "========================================"
} >> "$REPORT_FILE"

# Calculate average ratios
avg_stash=$(tail -n +2 "$CSV_FILE" | awk -F',' '$7 != "N/A" {sum+=$7; count++} END {if(count>0) printf "%.2f", sum/count; else print "N/A"}')
avg_komodo=$(tail -n +2 "$CSV_FILE" | awk -F',' '$8 != "N/A" {sum+=$8; count++} END {if(count>0) printf "%.2f", sum/count; else print "N/A"}')
avg_laser=$(tail -n +2 "$CSV_FILE" | awk -F',' '$9 != "N/A" {sum+=$9; count++} END {if(count>0) printf "%.2f", sum/count; else print "N/A"}')

echo -e "${BOLD}Average node explosion ratios:${NC}"
echo -e "  vs Stash:  ${RED}${avg_stash}x${NC}"
echo -e "  vs Komodo: ${RED}${avg_komodo}x${NC}"
echo -e "  vs Laser:  ${RED}${avg_laser}x${NC}"

{
    echo
    echo "Average node explosion ratios:"
    echo "  vs Stash:  ${avg_stash}x"
    echo "  vs Komodo: ${avg_komodo}x"
    echo "  vs Laser:  ${avg_laser}x"
} >> "$REPORT_FILE"

# Find worst cases
echo -e "\n${BOLD}Worst explosion cases:${NC}"
{
    echo
    echo "Worst explosion cases:"
} >> "$REPORT_FILE"

# Sort by ratios and show top 3
tail -n +2 "$CSV_FILE" | sort -t',' -k7 -rn | head -3 | while IFS=',' read -r pos depth sj st ko la r_st r_ko r_la; do
    echo -e "  $pos @ depth $depth: ${RED}${r_st}x${NC} vs Stash"
    echo "  $pos @ depth $depth: ${r_st}x vs Stash" >> "$REPORT_FILE"
done

echo -e "\n${GREEN}Report saved to: $REPORT_FILE${NC}"
echo -e "${GREEN}Data saved to: $CSV_FILE${NC}"

# Recommendations based on ratios
echo -e "\n${BOLD}Diagnostic Recommendations:${NC}"
{
    echo
    echo "========================================"
    echo "DIAGNOSTIC RECOMMENDATIONS"
    echo "========================================"
} >> "$REPORT_FILE"

if (( $(echo "$avg_stash > 5" | bc -l) )); then
    echo -e "  ${RED}⚠ CRITICAL:${NC} Extreme node explosion detected (>5x vs Stash)"
    echo "  • Priority: Check quiescence search and pruning conditions"
    echo "  • Likely culprits: SEE, delta pruning, or move ordering"
    {
        echo "  CRITICAL: Extreme node explosion detected (>5x vs Stash)"
        echo "  • Priority: Check quiescence search and pruning conditions"
        echo "  • Likely culprits: SEE, delta pruning, or move ordering"
    } >> "$REPORT_FILE"
elif (( $(echo "$avg_komodo > 3" | bc -l) )); then
    echo -e "  ${YELLOW}⚠ WARNING:${NC} Significant node explosion (>3x vs Komodo)"
    echo "  • Priority: Review LMR and futility pruning"
    echo "  • Check move ordering effectiveness"
    {
        echo "  WARNING: Significant node explosion (>3x vs Komodo)"
        echo "  • Priority: Review LMR and futility pruning"
        echo "  • Check move ordering effectiveness"
    } >> "$REPORT_FILE"
else
    echo -e "  ${GREEN}✓${NC} Node counts within reasonable range"
    echo "  ✓ Node counts within reasonable range" >> "$REPORT_FILE"
fi

echo -e "\n${BOLD}Next steps:${NC}"
echo "  1. Add instrumentation to measure pruning effectiveness"
echo "  2. Profile quiescence search specifically"
echo "  3. Check move ordering statistics"

{
    echo
    echo "Next steps:"
    echo "  1. Add instrumentation to measure pruning effectiveness"
    echo "  2. Profile quiescence search specifically"
    echo "  3. Check move ordering statistics"
} >> "$REPORT_FILE"