#!/bin/bash

# Test a sample of WAC positions with both legacy and ranked move picker
ENGINE="./bin/seajay"
TIME_MS=2000

echo "Testing sample WAC positions..."
echo "================================"

# Create a small test file with 5 positions
cat > wac_sample.epd << EOF
2rr3k/pp3pp1/1nnqbN1p/3pN3/2pP4/2P3Q1/PPB4P/R4RK1 w - - bm Qg6; id "WAC.001";
8/7p/5k2/5p2/p1p2P2/Pr1pPK2/1P1R3P/8 b - - bm Rxb2; id "WAC.002";
5rk1/1ppb3p/p1pb4/6q1/3P1p1r/2P1R2P/PP1BQ1P1/5RKN w - - bm Rg3; id "WAC.003";
r1bq2rk/pp3pbp/2p1p1pQ/7P/3P4/2PB1N2/PP3PPR/2KR4 w - - bm Qxh7+; id "WAC.004";
5k2/6pp/p1qN4/1p1p4/3P4/2PKP2Q/PP3r2/3R4 b - - bm Qc4+; id "WAC.005";
EOF

echo "Testing with legacy move ordering (UseRankedMovePicker=false)..."
echo "------------------------------------------------------------------"
python3 tools/tactical_test.py "$ENGINE" wac_sample.epd $TIME_MS 2>&1 | grep -E "WAC\.|Success rate|Average"

echo ""
echo "Testing with RankedMovePicker (UseRankedMovePicker=true)..."
echo "------------------------------------------------------------------"

# Create a wrapper script that sets the option
cat > engine_wrapper.sh << 'EOF'
#!/bin/bash
# First send the option, then pass through to engine
(echo "setoption name UseRankedMovePicker value true"; cat) | ./bin/seajay
EOF
chmod +x engine_wrapper.sh

python3 tools/tactical_test.py ./engine_wrapper.sh wac_sample.epd $TIME_MS 2>&1 | grep -E "WAC\.|Success rate|Average"

# Clean up
rm -f wac_sample.epd engine_wrapper.sh

echo ""
echo "Test complete!"