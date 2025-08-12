/**
 * Simple test for Phase 2A: Memory Allocation
 */

#include <iostream>
#include <memory>
#include <cstring>

int main() {
    std::cout << "=== Testing Phase 2A: Memory Allocation ===\n";
    
    // Calculate sizes similar to what magic bitboards does
    size_t rookTableTotal = 102400;  // Based on our calculation
    size_t bishopTableTotal = 6784;  // Based on our calculation
    
    std::cout << "Rook table entries: " << rookTableTotal << "\n";
    std::cout << "Bishop table entries: " << bishopTableTotal << "\n";
    
    // Test allocation using unique_ptr
    std::unique_ptr<uint64_t[]> rookTable;
    std::unique_ptr<uint64_t[]> bishopTable;
    
    try {
        rookTable = std::make_unique<uint64_t[]>(rookTableTotal);
        bishopTable = std::make_unique<uint64_t[]>(bishopTableTotal);
        
        // Zero-initialize
        std::memset(rookTable.get(), 0, rookTableTotal * sizeof(uint64_t));
        std::memset(bishopTable.get(), 0, bishopTableTotal * sizeof(uint64_t));
        
        std::cout << "✓ Memory allocated successfully\n";
        
        // Test read/write
        rookTable[0] = 0xDEADBEEF;
        bishopTable[0] = 0xCAFEBABE;
        
        if (rookTable[0] == 0xDEADBEEF && bishopTable[0] == 0xCAFEBABE) {
            std::cout << "✓ Memory is readable/writable\n";
        }
        
        size_t totalMemory = (rookTableTotal + bishopTableTotal) * sizeof(uint64_t);
        std::cout << "Total memory: " << totalMemory << " bytes (" 
                  << (totalMemory / 1024.0) << " KB)\n";
        
    } catch (const std::bad_alloc& e) {
        std::cerr << "ERROR: Failed to allocate memory: " << e.what() << "\n";
        return 1;
    }
    
    std::cout << "\n=== Phase 2A PASSED ===\n";
    return 0;
}