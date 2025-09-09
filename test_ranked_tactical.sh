#!/bin/bash
# Properly test RankedMovePicker with WAC positions

# Test 5 WAC positions with default settings (UseRankedMovePicker=false)
echo "Testing with default settings (UseRankedMovePicker=false):"
head -5 tests/positions/wacnew.epd > wac_5.epd
python3 tools/tactical_test.py ./bin/seajay wac_5.epd 2000 2>&1 | tail -15

echo ""
echo "Now testing with UseRankedMovePicker=true:"
# Need to modify the engine to set the option
cat > seajay_ranked.sh << 'INNER_EOF'
#!/bin/bash
(echo "uci"; echo "setoption name UseRankedMovePicker value true"; cat) | ./bin/seajay
INNER_EOF
chmod +x seajay_ranked.sh

python3 tools/tactical_test.py ./seajay_ranked.sh wac_5.epd 2000 2>&1 | tail -15

rm -f wac_5.epd seajay_ranked.sh
