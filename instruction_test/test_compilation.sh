#!/bin/bash

echo "Testing compilation with various optimization levels and instruction sets"
echo "========================================================================"
echo ""

# Test 1: Basic compilation with -O2
echo "Test 1: Basic -O2"
echo "-----------------"
if g++ -O2 instruction_test.cpp -o test_O2 2>/dev/null; then
    echo "Compilation: SUCCESS"
    ./test_O2
else
    echo "Compilation: FAILED"
fi
echo ""

# Test 2: -O3 optimization
echo "Test 2: -O3"
echo "-----------"
if g++ -O3 instruction_test.cpp -o test_O3 2>/dev/null; then
    echo "Compilation: SUCCESS"
    ./test_O3
else
    echo "Compilation: FAILED"
fi
echo ""

# Test 3: -O3 with SSE4.2 and POPCNT
echo "Test 3: -O3 -msse4.2 -mpopcnt"
echo "------------------------------"
if g++ -O3 -msse4.2 -mpopcnt instruction_test.cpp -o test_sse42 2>/dev/null; then
    echo "Compilation: SUCCESS"
    ./test_sse42
else
    echo "Compilation: FAILED"
fi
echo ""

# Test 4: march=native (auto-detect)
echo "Test 4: -O3 -march=native"
echo "-------------------------"
if g++ -O3 -march=native instruction_test.cpp -o test_native 2>/dev/null; then
    echo "Compilation: SUCCESS"
    ./test_native
else
    echo "Compilation: FAILED"
fi
echo ""

# Test 5: march=westmere (our CPU)
echo "Test 5: -O3 -march=westmere"
echo "---------------------------"
if g++ -O3 -march=westmere instruction_test.cpp -o test_westmere 2>/dev/null; then
    echo "Compilation: SUCCESS"
    ./test_westmere
else
    echo "Compilation: FAILED"
fi
echo ""

# Test 6: Link-time optimization
echo "Test 6: -O3 -flto -march=native"
echo "--------------------------------"
if g++ -O3 -flto -march=native instruction_test.cpp -o test_lto 2>/dev/null; then
    echo "Compilation: SUCCESS"
    ./test_lto
else
    echo "Compilation: FAILED"
fi
echo ""

# Test 7: Try with BMI (should fail)
echo "Test 7: -O3 -mbmi -mbmi2 (Expected to fail or not use BMI)"
echo "-----------------------------------------------------------"
if g++ -O3 -mbmi -mbmi2 instruction_test.cpp -o test_bmi 2>/dev/null; then
    echo "Compilation: SUCCESS (but may not actually use BMI)"
    ./test_bmi
else
    echo "Compilation: FAILED (as expected)"
fi
echo ""

# Test 8: Try with AVX (should fail)
echo "Test 8: -O3 -mavx -mavx2 (Expected to fail or not use AVX)"
echo "-----------------------------------------------------------"
if g++ -O3 -mavx -mavx2 instruction_test.cpp -o test_avx 2>/dev/null; then
    echo "Compilation: SUCCESS (but may not actually use AVX)"
    ./test_avx
else
    echo "Compilation: FAILED (as expected)"
fi
echo ""

# Clean up
rm -f test_O2 test_O3 test_sse42 test_native test_westmere test_lto test_bmi test_avx

echo "Testing complete!"