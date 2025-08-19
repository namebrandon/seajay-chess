#!/bin/bash

echo "=========================================="
echo "Testing SeaJay Stage 14 Phase 2 Changes"
echo "=========================================="
echo

# Test 1: Verify compilation and basic functionality
echo "Test 1: Basic functionality"
echo "---------------------------"
echo -e "uci\nquit" | ./bin/seajay 2>&1 | grep "id name"
echo

# Test 2: Run bench with different SEE modes
echo "Test 2: SEE Pruning Modes"
echo "--------------------------"

echo "SEE OFF:"
echo -e "setoption name SEEPruning value off\nbench" | ./bin/seajay 2>&1 | grep "Total:"

echo "SEE CONSERVATIVE:"
echo -e "setoption name SEEPruning value conservative\nbench" | ./bin/seajay 2>&1 | grep "Total:"

echo "SEE AGGRESSIVE:"
echo -e "setoption name SEEPruning value aggressive\nbench" | ./bin/seajay 2>&1 | grep "Total:"
echo

# Test 3: Verify no crashes in tactical positions
echo "Test 3: Tactical position test (Kiwipete)"
echo "------------------------------------------"
KIWIPETE="r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1"
echo -e "position fen $KIWIPETE\ngo depth 6" | ./bin/seajay 2>&1 | tail -5
echo

# Test 4: Verify quiescence search is working
echo "Test 4: Quiescence search verification"
echo "---------------------------------------"
echo -e "setoption name UseQuiescence value true\nposition startpos\ngo depth 1" | ./bin/seajay 2>&1 | grep "info depth"
echo

# Test 5: Check for memory leaks with valgrind (if available)
if command -v valgrind &> /dev/null; then
    echo "Test 5: Memory leak check"
    echo "-------------------------"
    echo -e "bench\nquit" | valgrind --leak-check=summary --error-exitcode=1 ./bin/seajay 2>&1 | grep "ERROR SUMMARY"
else
    echo "Test 5: Valgrind not available, skipping memory check"
fi
echo

echo "=========================================="
echo "Phase 2 testing complete!"
echo "=========================================="