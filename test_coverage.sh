#!/bin/bash

# Create a simple test that will run enough nodes to trigger coverage checks
echo "Testing RankedMovePicker coverage (this will generate ~2000 nodes):"
echo ""
echo -e "setoption name UseRankedMovePicker value true\nposition startpos\ngo depth 6" | ./bin/seajay 2>&1 | grep -E "COVERAGE|DUPLICATE|MISSING|nodes"
