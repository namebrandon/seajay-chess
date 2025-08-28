#!/bin/bash

# Script to verify the performance fix

echo "===== Performance Fix Verification ====="
echo ""
echo "This script will help verify if the SearchData size reduction fixes the regression."
echo ""

# Check if we can measure cache misses
if command -v perf &> /dev/null; then
    echo "Using 'perf' to measure cache performance..."
    echo ""
    
    # Measure cache misses on a standard position
    echo "Measuring cache misses for standard benchmark position..."
    echo "position startpos
go depth 12
quit" | perf stat -e cache-misses,cache-references ./seajay 2>&1 | grep -E "(cache-misses|cache-references|nodes|nps)"
    
    echo ""
    echo "Key metrics to check:"
    echo "1. Cache miss rate should be < 5% after fix (likely > 15% before fix)"
    echo "2. NPS should increase by 10-14% after fix"
else
    echo "'perf' not available. Using basic benchmark instead..."
fi

echo ""
echo "Running standard benchmark..."
echo ""

# Run benchmark
echo "bench" | ./seajay | grep -E "(nodes|nps|time)"

echo ""
echo "===== Size Verification ====="
echo ""

# Create a simple test program to check sizes
cat > check_sizes.cpp << 'EOF'
#include <iostream>
#include <type_traits>
#include "src/search/types.h"
#include "src/search/iterative_search_data.h"

int main() {
    using namespace seajay::search;
    
    std::cout << "SearchData size: " << sizeof(SearchData) << " bytes" << std::endl;
    std::cout << "IterativeSearchData size: " << sizeof(IterativeSearchData) << " bytes" << std::endl;
    std::cout << "Is SearchData polymorphic: " << (std::is_polymorphic<SearchData>::value ? "YES" : "NO") << std::endl;
    
    // Calculate expected sizes
    size_t killer_size = sizeof(void*);  // Should be pointer (8 bytes)
    size_t history_size = sizeof(void*); // Should be pointer (8 bytes)
    size_t counter_size = sizeof(void*); // Should be pointer (8 bytes)
    
    std::cout << "\nExpected after fix:" << std::endl;
    std::cout << "  Killer pointer: " << killer_size << " bytes" << std::endl;
    std::cout << "  History pointer: " << history_size << " bytes" << std::endl;
    std::cout << "  Counter pointer: " << counter_size << " bytes" << std::endl;
    
    if (sizeof(SearchData) > 2000) {
        std::cout << "\n[WARNING] SearchData is still too large! The fix may not be applied correctly." << std::endl;
        return 1;
    } else {
        std::cout << "\n[SUCCESS] SearchData size is reasonable." << std::endl;
        return 0;
    }
}
EOF

# Compile and run the size check
g++ -std=c++20 -I. -o check_sizes check_sizes.cpp 2>/dev/null

if [ -f check_sizes ]; then
    ./check_sizes
    rm check_sizes check_sizes.cpp
else
    echo "Could not compile size check program."
fi

echo ""
echo "===== Expected Results After Fix ====="
echo "1. SearchData size: ~1KB (down from 42KB)"
echo "2. NPS increase: 10-14%"
echo "3. Cache miss rate: < 5%"
echo "4. ELO recovery: 30-40 points"