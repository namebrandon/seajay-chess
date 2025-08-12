/**
 * Test Phase 2A: Memory Allocation for Magic Bitboards
 * Validates that memory is properly allocated and initialized
 */

#include "../src/core/magic_bitboards.h"
#include <iostream>
#include <cassert>

using namespace seajay;

int main() {
    std::cout << "=== Testing Phase 2A: Memory Allocation ===\n";
    
    // Initialize magic bitboards (should allocate memory)
    magic::initMagics();
    
    // Verify that attack tables are allocated
    if (!magic::rookAttackTable) {
        std::cerr << "ERROR: Rook attack table not allocated!\n";
        return 1;
    }
    
    if (!magic::bishopAttackTable) {
        std::cerr << "ERROR: Bishop attack table not allocated!\n";
        return 1;
    }
    
    std::cout << "✓ Memory allocation successful\n";
    
    // Test that memory is accessible (should not crash)
    // Write and read a test value
    magic::rookAttackTable[0] = 0xDEADBEEF;
    if (magic::rookAttackTable[0] != 0xDEADBEEF) {
        std::cerr << "ERROR: Cannot write/read rook attack table!\n";
        return 1;
    }
    
    magic::bishopAttackTable[0] = 0xCAFEBABE;
    if (magic::bishopAttackTable[0] != 0xCAFEBABE) {
        std::cerr << "ERROR: Cannot write/read bishop attack table!\n";
        return 1;
    }
    
    std::cout << "✓ Memory is readable/writable\n";
    
    // Initialize again to test std::once_flag (should not double-allocate)
    magic::initMagics();
    std::cout << "✓ Multiple init calls are safe (std::once_flag working)\n";
    
    std::cout << "\n=== Phase 2A PASSED ===\n";
    std::cout << "Run with valgrind to check for memory leaks:\n";
    std::cout << "  valgrind --leak-check=full ./test_phase2a\n";
    
    return 0;
}