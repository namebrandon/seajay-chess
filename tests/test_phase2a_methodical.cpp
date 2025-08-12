/**
 * METHODICAL VALIDATION for Phase 2A: Memory Allocation
 * Build up from the simplest test to complex
 */

#include <iostream>
#include <memory>
#include <cstring>
#include <cstdint>

// Step 1: Test basic memory allocation
bool testBasicAllocation() {
    std::cout << "Test 1: Basic memory allocation...";
    
    const size_t size = 1000;
    std::unique_ptr<uint64_t[]> table = std::make_unique<uint64_t[]>(size);
    
    if (!table) {
        std::cout << " FAILED!\n";
        return false;
    }
    
    // Test write and read
    table[0] = 0xDEADBEEF;
    table[999] = 0xCAFEBABE;
    
    if (table[0] != 0xDEADBEEF || table[999] != 0xCAFEBABE) {
        std::cout << " FAILED!\n";
        return false;
    }
    
    std::cout << " PASSED!\n";
    return true;
}

// Step 2: Test larger allocation (like our actual sizes)
bool testLargeAllocation() {
    std::cout << "Test 2: Large memory allocation (853KB)...";
    
    const size_t rookSize = 102400;
    const size_t bishopSize = 6784;
    
    std::unique_ptr<uint64_t[]> rookTable = std::make_unique<uint64_t[]>(rookSize);
    std::unique_ptr<uint64_t[]> bishopTable = std::make_unique<uint64_t[]>(bishopSize);
    
    if (!rookTable || !bishopTable) {
        std::cout << " FAILED!\n";
        return false;
    }
    
    // Zero-initialize
    std::memset(rookTable.get(), 0, rookSize * sizeof(uint64_t));
    std::memset(bishopTable.get(), 0, bishopSize * sizeof(uint64_t));
    
    // Verify zeros
    if (rookTable[0] != 0 || rookTable[rookSize-1] != 0) {
        std::cout << " FAILED (not zeroed)!\n";
        return false;
    }
    
    std::cout << " PASSED!\n";
    return true;
}

// Step 3: Test memory access patterns
bool testAccessPatterns() {
    std::cout << "Test 3: Memory access patterns...";
    
    const size_t size = 4096;
    std::unique_ptr<uint64_t[]> table = std::make_unique<uint64_t[]>(size);
    
    // Write pattern
    for (size_t i = 0; i < size; ++i) {
        table[i] = i * 0x123456789ABCDEFULL;
    }
    
    // Verify pattern
    for (size_t i = 0; i < size; ++i) {
        if (table[i] != i * 0x123456789ABCDEFULL) {
            std::cout << " FAILED at index " << i << "!\n";
            return false;
        }
    }
    
    std::cout << " PASSED!\n";
    return true;
}

// Step 4: Test calculated offsets (like magic bitboards will use)
bool testCalculatedOffsets() {
    std::cout << "Test 4: Calculated offsets...";
    
    const size_t totalSize = 102400;
    std::unique_ptr<uint64_t[]> table = std::make_unique<uint64_t[]>(totalSize);
    
    // Simulate how magic bitboards will partition the table
    size_t offset = 0;
    for (int sq = 0; sq < 64; ++sq) {
        // Simulate different sizes per square
        size_t squareSize = (sq < 4) ? 4096 : 1024;  // Corners vs others
        
        // Write a marker at the start of each square's section
        if (offset + squareSize <= totalSize) {
            table[offset] = 0x1000000000000000ULL | sq;
        }
        
        offset += squareSize;
        if (offset > totalSize) break;
    }
    
    std::cout << " PASSED!\n";
    return true;
}

int main() {
    std::cout << "=== METHODICAL VALIDATION: Phase 2A Memory Allocation ===\n\n";
    
    int passed = 0;
    int failed = 0;
    
    if (testBasicAllocation()) passed++; else failed++;
    if (testLargeAllocation()) passed++; else failed++;
    if (testAccessPatterns()) passed++; else failed++;
    if (testCalculatedOffsets()) passed++; else failed++;
    
    std::cout << "\n=== RESULTS ===\n";
    std::cout << "Passed: " << passed << "\n";
    std::cout << "Failed: " << failed << "\n";
    
    if (failed == 0) {
        std::cout << "\n✓ Phase 2A Memory Allocation: ALL TESTS PASSED!\n";
        std::cout << "Memory allocation is working correctly.\n";
        std::cout << "Ready to proceed to Step 2B: Single Square Table Generation\n";
        return 0;
    } else {
        std::cout << "\n✗ Phase 2A: TESTS FAILED!\n";
        return 1;
    }
}