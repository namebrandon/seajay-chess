#!/bin/bash
# Helper to create individual test scripts

create_test_script() {
    local stage1_num=$1
    local stage2_num=$2
    local stage1_desc=$3
    local stage2_desc=$4
    local stage1_binary=$5
    local stage2_binary=$6
    local stage1_name=$7
    local stage2_name=$8
    
    local filename="/workspace/tools/scripts/sprt_tests/historical_progression/stage${stage1_num}_vs_stage${stage2_num}.sh"
    
    cat > "$filename" << SCRIPT
#!/bin/bash
# Stage ${stage1_num} vs Stage ${stage2_num} Progression Test
# 200 games with diverse openings at TC 10+0.1

echo "========================================================"
echo "Stage ${stage1_num} vs Stage ${stage2_num} Progression Test"
echo "========================================================"
echo ""
echo "Stage ${stage1_num}: ${stage1_desc}"
echo "Stage ${stage2_num}: ${stage2_desc}"
echo ""
echo "Using 8moves_v3.pgn for opening diversity"
echo "200 games (100 with each color)"
echo "Time control: 10+0.1"
echo ""

# Configuration
OUTPUT_DIR="/workspace/tools/scripts/sprt_tests/historical_progression/results"
OPENING_BOOK="/workspace/external/books/8moves_v3.pgn"
TC="10+0.1"
GAMES=200
ROUNDS=\$((GAMES / 2))

# Binaries
STAGE${stage1_num}="${stage1_binary}"
STAGE${stage2_num}="${stage2_binary}"

# Create output directory
mkdir -p "\$OUTPUT_DIR"

# Check for fast-chess
FASTCHESS="/workspace/external/testers/fast-chess/fastchess"
if [ ! -f "\$FASTCHESS" ]; then
    echo "ERROR: fast-chess not found. Run setup-external-tools.sh first"
    exit 1
fi

# Verify binaries
if [ ! -f "\$STAGE${stage1_num}" ]; then
    echo "ERROR: Stage ${stage1_num} binary not found: \$STAGE${stage1_num}"
    exit 1
fi
if [ ! -f "\$STAGE${stage2_num}" ]; then
    echo "ERROR: Stage ${stage2_num} binary not found: \$STAGE${stage2_num}"
    exit 1
fi

# Verify opening book
if [ ! -f "\$OPENING_BOOK" ]; then
    echo "ERROR: Opening book not found: \$OPENING_BOOK"
    exit 1
fi

# Display checksums
echo "Binary checksums:"
echo "  Stage ${stage1_num}: \$(md5sum \$STAGE${stage1_num} | awk '{print \$1}')"
echo "  Stage ${stage2_num}: \$(md5sum \$STAGE${stage2_num} | awk '{print \$1}')"
echo ""

# Output files
PGN_FILE="\$OUTPUT_DIR/stage${stage1_num}_vs_stage${stage2_num}.pgn"
LOG_FILE="\$OUTPUT_DIR/stage${stage1_num}_vs_stage${stage2_num}.log"

echo "Starting match..."
echo "Output: \$(basename \$PGN_FILE)"
echo ""

# Run the match
"\$FASTCHESS" \\
    -engine cmd="\$STAGE${stage1_num}" name="${stage1_name}" \\
    -engine cmd="\$STAGE${stage2_num}" name="${stage2_name}" \\
    -each tc="\$TC" \\
    -openings file="\$OPENING_BOOK" format=pgn order=sequential \\
    -games 2 -rounds \$ROUNDS -repeat \\
    -pgnout file="\$PGN_FILE" \\
    -log file="\$LOG_FILE" level=info \\
    -recover \\
    -event "Stage ${stage1_num} vs Stage ${stage2_num} Progression Test" \\
    -site "8moves_v3 Diverse Openings"

# Display results
echo ""
echo "========================================================"
echo "Match Complete!"
echo "========================================================"
echo ""

if [ -f "\$LOG_FILE" ]; then
    # Extract final score
    SCORE_LINE=\$(grep -E "Score of ${stage1_name} vs ${stage2_name}" "\$LOG_FILE" | tail -1)
    
    if [ -n "\$SCORE_LINE" ]; then
        echo "Final Result:"
        echo "\$SCORE_LINE"
        echo ""
        
        # Parse and display statistics
        if [[ "\$SCORE_LINE" =~ ([0-9]+)[[:space:]]-[[:space:]]([0-9]+)[[:space:]]-[[:space:]]([0-9]+) ]]; then
            S${stage1_num}_WINS="\${BASH_REMATCH[1]}"
            S${stage2_num}_WINS="\${BASH_REMATCH[2]}"
            DRAWS="\${BASH_REMATCH[3]}"
            
            TOTAL=\$((S${stage1_num}_WINS + S${stage2_num}_WINS + DRAWS))
            
            echo "Statistics:"
            echo "  Stage ${stage1_num} wins: \$S${stage1_num}_WINS (\$(( S${stage1_num}_WINS * 100 / TOTAL ))%)"
            echo "  Stage ${stage2_num} wins: \$S${stage2_num}_WINS (\$(( S${stage2_num}_WINS * 100 / TOTAL ))%)"
            echo "  Draws:         \$DRAWS (\$(( DRAWS * 100 / TOTAL ))%)"
            
            # Simple Elo estimate
            if [ \$((S${stage1_num}_WINS + S${stage2_num}_WINS)) -gt 0 ]; then
                WIN_RATE=\$(echo "scale=1; \$S${stage2_num}_WINS * 100 / (\$S${stage1_num}_WINS + \$S${stage2_num}_WINS)" | bc)
                echo ""
                echo "Stage ${stage2_num} win rate (excl. draws): \${WIN_RATE}%"
                
                if (( \$(echo "\$WIN_RATE > 55" | bc -l) )); then
                    echo "Stage ${stage2_num} appears stronger (${stage2_desc})"
                elif (( \$(echo "\$WIN_RATE < 45" | bc -l) )); then
                    echo "Stage ${stage1_num} appears stronger (unexpected result)"
                else
                    echo "Stages appear roughly equal in strength"
                fi
            fi
        fi
    fi
fi

echo ""
echo "Files saved:"
echo "  PGN: \$PGN_FILE"
echo "  Log: \$LOG_FILE"
echo ""
echo "To analyze games:"
echo "  less \$PGN_FILE"
echo "  grep \"Result\" \$PGN_FILE | sort | uniq -c"
SCRIPT

    chmod +x "$filename"
    echo "Created: $filename"
}

# Create all the test scripts
create_test_script "11" "12" "MVV-LVA move ordering" "Transposition tables" \
    "/workspace/binaries/seajay-stage11-f2ad9b5" \
    "/workspace/binaries/seajay-stage12-ffa0e44" \
    "Stage11-MVV" "Stage12-TT"

create_test_script "12" "13" "Transposition tables" "Iterative deepening" \
    "/workspace/binaries/seajay-stage12-ffa0e44" \
    "/workspace/binaries/seajay-stage13-869495e" \
    "Stage12-TT" "Stage13-ID"

create_test_script "13" "14" "Iterative deepening" "Quiescence search" \
    "/workspace/binaries/seajay-stage13-869495e" \
    "/workspace/binaries/seajay-stage14-final" \
    "Stage13-ID" "Stage14-QS"

create_test_script "14" "15" "Quiescence search" "SEE parameter tuning" \
    "/workspace/binaries/seajay-stage14-final" \
    "/workspace/binaries/seajay-stage15-properly-fixed" \
    "Stage14-QS" "Stage15-Tuned"

echo "All test scripts created!"
