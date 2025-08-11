#!/bin/bash

# Stage 9b SPRT Setup Validation Script
# Verifies all prerequisites are met before running SPRT test

echo "=== Stage 9b SPRT Setup Validation ==="
echo

# Check 1: Binaries exist and work
echo "✓ Checking test binaries..."

if [ ! -f "/workspace/bin/seajay_stage9_base" ]; then
    echo "❌ Stage 9 binary missing: /workspace/bin/seajay_stage9_base"
    echo "   Run: ./prepare_stage9b_binaries.sh"
    exit 1
else
    echo "  ✓ Stage 9 binary found"
fi

if [ ! -f "/workspace/bin/seajay_stage9b_draws" ]; then
    echo "❌ Stage 9b binary missing: /workspace/bin/seajay_stage9b_draws"
    echo "   Run: ./prepare_stage9b_binaries.sh"
    exit 1
else
    echo "  ✓ Stage 9b binary found"
fi

# Check 2: UCI command response
echo
echo "✓ Testing UCI responses..."

echo "  Testing Stage 9 binary..."
if timeout 5s bash -c 'echo -e "uci\nquit" | /workspace/bin/seajay_stage9_base' >/dev/null 2>&1; then
    echo "  ✓ Stage 9 responds to UCI"
else
    echo "  ❌ Stage 9 UCI test failed"
    exit 1
fi

echo "  Testing Stage 9b binary..."
if timeout 5s bash -c 'echo -e "uci\nquit" | /workspace/bin/seajay_stage9b_draws' >/dev/null 2>&1; then
    echo "  ✓ Stage 9b responds to UCI"
else
    echo "  ❌ Stage 9b UCI test failed"
    exit 1
fi

# Check 3: Fast-chess availability
echo
echo "✓ Checking fast-chess..."

if [ ! -f "/workspace/external/testers/fast-chess/fastchess" ]; then
    echo "❌ Fast-chess not found: /workspace/external/testers/fast-chess/fastchess"
    echo "   Install with: ./tools/scripts/setup-external-tools.sh"
    exit 1
else
    echo "  ✓ Fast-chess found"
fi

# Check 4: Opening book (optional but recommended)
echo
echo "✓ Checking opening book..."

if [ ! -f "/workspace/external/books/4moves_test.pgn" ]; then
    echo "⚠️  Opening book not found: /workspace/external/books/4moves_test.pgn"
    echo "   SPRT will run without opening book (not recommended)"
    echo "   Consider installing with: ./tools/scripts/setup-external-tools.sh"
else
    echo "  ✓ Opening book found"
fi

# Check 5: Output directory
echo
echo "✓ Checking output directory..."

mkdir -p /workspace/sprt_results
if [ -d "/workspace/sprt_results" ]; then
    echo "  ✓ Output directory ready"
else
    echo "❌ Cannot create output directory: /workspace/sprt_results"
    exit 1
fi

# Check 6: Draw detection functionality
echo
echo "✓ Testing draw detection..."

echo "  Testing Stage 9 (should NOT detect draws)..."
STAGE9_TEST=$(echo -e "position fen 8/8/8/4k3/8/3K4/8/8 w - - 0 1\ngo depth 1\nquit" | /workspace/bin/seajay_stage9_base 2>&1)
if echo "$STAGE9_TEST" | grep -qi "insufficient"; then
    echo "  ⚠️  WARNING: Stage 9 appears to detect insufficient material"
    echo "      This might affect test results"
else
    echo "  ✓ Stage 9 does not report draws"
fi

echo "  Testing Stage 9b (should detect draws)..."
STAGE9B_TEST=$(echo -e "position fen 8/8/8/4k3/8/3K4/8/8 w - - 0 1\ngo depth 1\nquit" | /workspace/bin/seajay_stage9b_draws 2>&1)
if echo "$STAGE9B_TEST" | grep -qi "insufficient"; then
    echo "  ✓ Stage 9b correctly detects insufficient material"
else
    echo "  ❌ WARNING: Stage 9b not detecting insufficient material"
    echo "      Draw detection may not be working properly"
fi

# Check 7: Performance baseline
echo
echo "✓ Testing performance..."

echo "  Stage 9 performance test..."
time timeout 10s bash -c 'echo -e "position startpos\ngo depth 4\nquit" | /workspace/bin/seajay_stage9_base >/dev/null 2>&1' 2>/tmp/stage9_time
if [ $? -eq 0 ]; then
    echo "  ✓ Stage 9 search completes"
else
    echo "  ⚠️  Stage 9 search timeout (may be slow)"
fi

echo "  Stage 9b performance test..."
time timeout 10s bash -c 'echo -e "position startpos\ngo depth 4\nquit" | /workspace/bin/seajay_stage9b_draws >/dev/null 2>&1' 2>/tmp/stage9b_time
if [ $? -eq 0 ]; then
    echo "  ✓ Stage 9b search completes"
else
    echo "  ⚠️  Stage 9b search timeout (may be slow)"
fi

echo
echo "=== Validation Complete ==="
echo

echo "✅ Ready for SPRT testing!"
echo
echo "Next steps:"
echo "  1. Run: ./run_stage9b_sprt.sh"
echo "  2. Wait for completion (1-4 hours)"
echo "  3. Analyze results with: python3 analyze_stage9b_draws.py <results_dir>"
echo
echo "IMPORTANT: This test focuses on DRAW RATE REDUCTION, not traditional Elo improvement"
echo "Success criteria:"
echo "  • Stage 9b total draw rate < 35% (vs >40% for Stage 9)"
echo "  • Stage 9b repetition draws < 10% (vs >20% for Stage 9)"
echo "  • Elo within ±10 points (no significant regression)"